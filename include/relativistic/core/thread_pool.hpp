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
#include <cstdio>
#include <exception>

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
				try {
					task();
				} catch (const std::exception& ex) {
					std::fprintf(stderr, "[ThreadPool] worker task threw exception: %s\n", ex.what());
				} catch (...) {
					std::fprintf(stderr, "[ThreadPool] worker task threw an unknown exception\n");
				}
				if (active_tasks_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
					std::lock_guard<std::mutex> lock(queue_mutex_);
					cv_finished_.notify_all();
				}
			}
		}
	}

public:
	explicit ThreadPool(size_t thread_count = 0) {
		const size_t count = std::clamp((thread_count > 0) ? thread_count : static_cast<size_t>(std::thread::hardware_concurrency()), size_t{1}, size_t{32});
		workers_.reserve(count);
		for (size_t i = 0; i < count; ++i) {
			try {
				workers_.emplace_back([this](std::stop_token st) {
					worker_loop(st);
				});
			} catch (const std::exception& ex) {
				std::fprintf(stderr, "[ThreadPool] failed to start worker thread %zu of %zu: %s\n", i, count, ex.what());
				break;
			} catch (...) {
				std::fprintf(stderr, "[ThreadPool] failed to start worker thread %zu of %zu due to an unknown error\n", i, count);
				break;
			}
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
		while (true) {
			std::function<void()> task;
			{
				std::unique_lock<std::mutex> lock(queue_mutex_);
				if (tasks_.empty()) {
					if (active_tasks_.load(std::memory_order_acquire) == 0) {
						return;
					}
					cv_finished_.wait(lock, [this]() {
						return tasks_.empty() && (active_tasks_.load(std::memory_order_acquire) == 0);
					});
					return;
				}
				task = std::move(tasks_.front());
				tasks_.pop();
			}
			if (task) {
				try {
					task();
				} catch (const std::exception& ex) {
					std::fprintf(stderr, "[ThreadPool] wait_idle task threw exception: %s\n", ex.what());
				} catch (...) {
					std::fprintf(stderr, "[ThreadPool] wait_idle task threw an unknown exception\n");
				}
				if (active_tasks_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
					std::lock_guard<std::mutex> lock(queue_mutex_);
					cv_finished_.notify_all();
				}
			}
		}
	}

	template <typename Func>
		requires std::invocable<Func, size_t, size_t>
	void parallel_for(size_t total_items, Func&& func, size_t min_chunk = 1) {
		if (total_items == 0) return;
		const size_t count = workers_.size();
		const size_t chunk_size = std::max(min_chunk, std::max(size_t{1}, total_items / (std::max(count, size_t{1}) * 4)));
		std::atomic<size_t> current_index{0};
		const size_t tasks_needed = std::min(count, (total_items + chunk_size - 1) / chunk_size);

		std::mutex completion_mutex;
		std::condition_variable completion_cv;
		std::atomic<size_t> remaining_tasks{std::max(tasks_needed, size_t{1})};

		auto worker_task = [&]() {
			while (true) {
				const size_t start = current_index.fetch_add(chunk_size, std::memory_order_relaxed);
				if (start >= total_items) break;
				const size_t end = std::min(start + chunk_size, total_items);
				try {
					func(start, end);
				} catch (const std::exception& ex) {
					std::fprintf(stderr, "[ThreadPool] parallel_for chunk threw exception: %s\n", ex.what());
				} catch (...) {
					std::fprintf(stderr, "[ThreadPool] parallel_for chunk threw an unknown exception\n");
				}
			}
			if (remaining_tasks.fetch_sub(1, std::memory_order_acq_rel) == 1) {
				std::lock_guard<std::mutex> lock(completion_mutex);
				completion_cv.notify_all();
			}
		};

		if (tasks_needed > 1) {
			bool queued = false;
			{
				std::lock_guard<std::mutex> lock(queue_mutex_);
				if (!stop_.load(std::memory_order_relaxed)) {
					active_tasks_.fetch_add(tasks_needed - 1, std::memory_order_relaxed);
					for (size_t i = 1; i < tasks_needed; ++i) {
						tasks_.push(worker_task);
					}
					queued = true;
				}
			}
			if (queued) {
				cv_task_.notify_all();
			} else {
				remaining_tasks.fetch_sub(tasks_needed - 1, std::memory_order_acq_rel);
			}
		}

		worker_task();

		std::unique_lock<std::mutex> completion_lock(completion_mutex);
		completion_cv.wait(completion_lock, [&]() { return remaining_tasks.load(std::memory_order_acquire) == 0; });
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

[[nodiscard]] inline ThreadPool& global_render_thread_pool() {
	static ThreadPool instance;
	return instance;
}

}
