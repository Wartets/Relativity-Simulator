#pragma once

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace Relativistic::Interferometry {

class FitsKeywordList {
public:
	void add_logical(std::string_view key, bool value, std::string_view comment = {}) {
		cards_.push_back(numeric_card(key, value ? "T" : "F", comment));
	}

	void add_integer(std::string_view key, int64_t value, std::string_view comment = {}) {
		cards_.push_back(numeric_card(key, std::to_string(value), comment));
	}

	void add_real(std::string_view key, double value, std::string_view comment = {}) {
		char buffer[48];
		std::snprintf(buffer, sizeof(buffer), "%.12E", std::isfinite(value) ? value : 0.0);
		cards_.push_back(numeric_card(key, buffer, comment));
	}

	void add_string(std::string_view key, std::string_view value, std::string_view comment = {}) {
		std::string escaped;
		escaped.reserve(value.size() + 8);
		for (const char character : value) {
			escaped.push_back(character);
			if (character == '\'') {
				escaped.push_back('\'');
			}
		}
		if (escaped.size() < 8) {
			escaped.resize(8, ' ');
		}
		std::string card = pad_key(key) + "= '" + escaped + "'";
		if (!comment.empty()) {
			card += " / ";
			card += comment;
		}
		card.resize(kCardSize, ' ');
		cards_.push_back(std::move(card));
	}

	[[nodiscard]] const std::vector<std::string>& cards() const noexcept {
		return cards_;
	}

	static constexpr size_t kCardSize = 80;

private:
	[[nodiscard]] static std::string pad_key(std::string_view key) {
		std::string padded(key.substr(0, 8));
		padded.resize(8, ' ');
		return padded;
	}

	[[nodiscard]] static std::string numeric_card(std::string_view key, const std::string& value, std::string_view comment) {
		std::string card = pad_key(key) + "= ";
		if (value.size() < 20) {
			card.append(20 - value.size(), ' ');
		}
		card += value;
		if (!comment.empty()) {
			card += " / ";
			card += comment;
		}
		card.resize(kCardSize, ' ');
		return card;
	}

	std::vector<std::string> cards_{};
};

namespace FitsDetail {

inline constexpr size_t kBlockSize = 2880;

inline void append_header(std::vector<char>& out, const std::vector<std::string>& cards) {
	for (const auto& card : cards) {
		out.insert(out.end(), card.begin(), card.end());
	}
	std::string end_card("END");
	end_card.resize(FitsKeywordList::kCardSize, ' ');
	out.insert(out.end(), end_card.begin(), end_card.end());
	const size_t remainder = out.size() % kBlockSize;
	if (remainder != 0) {
		out.insert(out.end(), kBlockSize - remainder, ' ');
	}
}

inline void pad_data(std::vector<char>& out) {
	const size_t remainder = out.size() % kBlockSize;
	if (remainder != 0) {
		out.insert(out.end(), kBlockSize - remainder, '\0');
	}
}

[[nodiscard]] inline size_t format_width(const std::string& format) {
	size_t repeat = 0;
	size_t index = 0;
	while (index < format.size() && format[index] >= '0' && format[index] <= '9') {
		repeat = repeat * 10 + static_cast<size_t>(format[index] - '0');
		++index;
	}
	if (index == 0) {
		repeat = 1;
	}
	const char type = (index < format.size()) ? format[index] : 'A';
	size_t element = 1;
	switch (type) {
		case 'I': element = 2; break;
		case 'J':
		case 'E': element = 4; break;
		case 'D': element = 8; break;
		case 'K': element = 8; break;
		default: element = 1; break;
	}
	return repeat * element;
}

}

class FitsBinaryTable {
public:
	explicit FitsBinaryTable(std::string extension_name) : extension_name_(std::move(extension_name)) {
		keywords_.add_string("EXTNAME", extension_name_);
	}

	[[nodiscard]] FitsKeywordList& keywords() noexcept {
		return keywords_;
	}

	void add_column(std::string name, std::string format, std::string unit = {}) {
		const size_t width = FitsDetail::format_width(format);
		columns_.push_back(Column{std::move(name), std::move(format), std::move(unit)});
		row_width_ += width;
	}

	void put_int16(int16_t value) {
		const auto bits = static_cast<uint16_t>(value);
		data_.push_back(static_cast<char>(bits >> 8));
		data_.push_back(static_cast<char>(bits & 0xFFU));
	}

	void put_int32(int32_t value) {
		put_unsigned(static_cast<uint32_t>(value), 4);
	}

	void put_float32(float value) {
		put_unsigned(std::bit_cast<uint32_t>(value), 4);
	}

	void put_float64(double value) {
		put_unsigned(std::bit_cast<uint64_t>(std::isfinite(value) ? value : 0.0), 8);
	}

	void put_logical(bool value) {
		data_.push_back(value ? 'T' : 'F');
	}

	void put_text(std::string_view text, size_t width) {
		std::string padded(text.substr(0, width));
		padded.resize(width, ' ');
		data_.insert(data_.end(), padded.begin(), padded.end());
	}

	void end_row() noexcept {
		++row_count_;
	}

	[[nodiscard]] size_t row_count() const noexcept {
		return row_count_;
	}

	void serialize(std::vector<char>& out) const {
		FitsKeywordList header;
		header.add_string("XTENSION", "BINTABLE");
		header.add_integer("BITPIX", 8);
		header.add_integer("NAXIS", 2);
		header.add_integer("NAXIS1", static_cast<int64_t>(row_width_));
		header.add_integer("NAXIS2", static_cast<int64_t>(row_count_));
		header.add_integer("PCOUNT", 0);
		header.add_integer("GCOUNT", 1);
		header.add_integer("TFIELDS", static_cast<int64_t>(columns_.size()));
		for (size_t i = 0; i < columns_.size(); ++i) {
			const std::string suffix = std::to_string(i + 1);
			header.add_string("TTYPE" + suffix, columns_[i].name);
			header.add_string("TFORM" + suffix, columns_[i].format);
			if (!columns_[i].unit.empty()) {
				header.add_string("TUNIT" + suffix, columns_[i].unit);
			}
		}
		std::vector<std::string> cards = header.cards();
		cards.insert(cards.end(), keywords_.cards().begin(), keywords_.cards().end());
		FitsDetail::append_header(out, cards);
		out.insert(out.end(), data_.begin(), data_.end());
		FitsDetail::pad_data(out);
	}

private:
	struct Column {
		std::string name;
		std::string format;
		std::string unit;
	};

	void put_unsigned(uint64_t bits, size_t bytes) {
		for (size_t i = 0; i < bytes; ++i) {
			const size_t shift = (bytes - 1 - i) * 8;
			data_.push_back(static_cast<char>((bits >> shift) & 0xFFU));
		}
	}

	std::string extension_name_;
	FitsKeywordList keywords_{};
	std::vector<Column> columns_{};
	std::vector<char> data_{};
	size_t row_width_{0};
	size_t row_count_{0};
};

class FitsDocument {
public:
	[[nodiscard]] FitsKeywordList& primary_keywords() noexcept {
		return primary_;
	}

	[[nodiscard]] FitsBinaryTable& add_table(std::string extension_name) {
		return tables_.emplace_back(std::move(extension_name));
	}

	[[nodiscard]] bool write(const std::filesystem::path& path, std::string& error) const {
		std::vector<char> bytes;
		FitsKeywordList header;
		header.add_logical("SIMPLE", true);
		header.add_integer("BITPIX", 8);
		header.add_integer("NAXIS", 0);
		header.add_logical("EXTEND", true);
		std::vector<std::string> cards = header.cards();
		cards.insert(cards.end(), primary_.cards().begin(), primary_.cards().end());
		FitsDetail::append_header(bytes, cards);
		for (const auto& table : tables_) {
			table.serialize(bytes);
		}

		std::error_code ec;
		if (path.has_parent_path()) {
			std::filesystem::create_directories(path.parent_path(), ec);
		}
		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		if (!file.is_open()) {
			error = "Unable to open the output file for writing.";
			return false;
		}
		file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
		if (!file) {
			error = "Writing the output file failed.";
			return false;
		}
		return true;
	}

private:
	FitsKeywordList primary_{};
	std::deque<FitsBinaryTable> tables_{};
};

}
