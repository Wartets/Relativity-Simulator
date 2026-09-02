#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <algorithm>
#include <span>
#include <concepts>

namespace Relativistic::Core {

class ThreadPool {
private:
	std::vector<std::jthread> workers_;
	std::queue<std::function<void()>> tasks_;
	std::mutex queue_mutex_;
	std::condition_variable cv_task_;
	std::condition_variable cv_finished_;
	std::atomic<size_t> active_tasks_{0};
	std::atomic<bool> stop_{false};

	void worker_loop(std::stop_token st) noexcept {
		while (!st.stop_requested()) {
			std::function<void()> task;
			{
				std::unique_lock<std::mutex> lock(queue_mutex_);
				cv_task_.wait(lock, [&]() {
					return st.stop_requested() || stop_.load(std::memory_order_relaxed) || !tasks_.empty();
				});

				if ((st.stop_requested() || stop_.load(std::memory_order_relaxed)) && tasks_.empty()) {
					return;
				}

				if (!tasks_.empty()) {
					task = std::move(tasks_.front());
					tasks_.pop();
				}
			}

			if (task) {
				task();
				if (active_tasks_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
					std::lock_guard<std::mutex> lock(queue_mutex_);
					cv_finished_.notify_all();
				}
			}
		}
	}

public:
	explicit ThreadPool(size_t thread_count = 0) {
		const size_t count = (thread_count > 0) ? thread_count : std::max(size_t{1}, static_cast<size_t>(std::thread::hardware_concurrency()));
		workers_.reserve(count);
		for (size_t i = 0; i < count; ++i) {
			workers_.emplace_back([this](std::stop_token st) {
				worker_loop(st);
			});
		}
	}

	~ThreadPool() noexcept {
		shutdown();
	}

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

	[[nodiscard]] size_t thread_count() const noexcept {
		return workers_.size();
	}

	void enqueue(std::function<void()> task) {
		{
			std::lock_guard<std::mutex> lock(queue_mutex_);
			if (stop_.load(std::memory_order_relaxed)) return;
			active_tasks_.fetch_add(1, std::memory_order_relaxed);
			tasks_.push(std::move(task));
		}
		cv_task_.notify_one();
	}

	void wait_idle() noexcept {
		std::unique_lock<std::mutex> lock(queue_mutex_);
		cv_finished_.wait(lock, [this]() {
			return tasks_.empty() && (active_tasks_.load(std::memory_order_acquire) == 0);
		});
	}

	template <typename Func>
		requires std::invocable<Func, size_t, size_t>
	void parallel_for(size_t total_items, Func&& func, size_t min_chunk = 1) {
		if (total_items == 0) return;
		const size_t count = workers_.size();
		const size_t num_chunks = std::clamp(count * 2, size_t{1}, total_items);
		const size_t chunk_size = std::max(min_chunk, (total_items + num_chunks - 1) / num_chunks);

		{
			std::lock_guard<std::mutex> lock(queue_mutex_);
			if (stop_.load(std::memory_order_relaxed)) return;
			const size_t planned_tasks = (total_items + chunk_size - 1) / chunk_size;
			active_tasks_.fetch_add(planned_tasks, std::memory_order_relaxed);
			for (size_t start = 0; start < total_items; start += chunk_size) {
				const size_t end = std::min(start + chunk_size, total_items);
				tasks_.push([&func, start, end]() {
					func(start, end);
				});
			}
		}
		cv_task_.notify_all();
		wait_idle();
	}

	void shutdown() noexcept {
		stop_.store(true, std::memory_order_release);
		cv_task_.notify_all();
		for (auto& w : workers_) {
			w.request_stop();
		}
		workers_.clear();
	}
};

}
