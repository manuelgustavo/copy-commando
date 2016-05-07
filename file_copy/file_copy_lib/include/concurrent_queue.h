#pragma once

#include <queue>
#include <condition_variable>
#include <mutex>
#include "file_part_task.h"
#include <atomic>


namespace thread_tools {
	template<typename DATA>
	class concurrent_queue
	{
	private:
		std::queue<DATA> the_queue;
		mutable std::mutex the_mutex;
		std::condition_variable the_condition_variable_pop;
		std::condition_variable the_condition_variable_push;
	public:
		concurrent_queue(unsigned int max) : m_push(0), m_pop(0) {
			m_max.store(max);
		}
		void push(DATA const& data) {
			{
				std::unique_lock<std::mutex> lock(the_mutex);
				the_condition_variable_push.wait(lock, [this] {return !(the_queue.size() > (m_max + 1)); });

				the_queue.push(data);
				++m_push;
			}
			the_condition_variable_pop.notify_all();
		}

		bool empty() const
		{
			std::lock_guard<std::mutex> lock(the_mutex);
			return the_queue.empty();
		}

		bool try_pop(DATA& popped_value) {
			{
				std::lock_guard<std::mutex> lock(the_mutex);
				if (the_queue.empty()) {
					return false;
				}

				popped_value = the_queue.front();
				the_queue.pop();
				++m_pop;
			}
			the_condition_variable_push.notify_all();
			return true;
		}

		void wait_and_pop(DATA& popped_value) {
			{
				std::unique_lock<std::mutex> lock(the_mutex);
				the_condition_variable_pop.wait(lock, [this] {return !the_queue.empty(); });

				popped_value = the_queue.front();
				the_queue.pop();
				++m_pop;
			}
			the_condition_variable_push.notify_all();
		}

		bool timed_wait_and_pop(DATA& popped_value, int wait_time) {
			{
				std::unique_lock<std::mutex> lock(the_mutex);
				if (!the_condition_variable_pop.wait_for(lock, std::chrono::milliseconds(wait_time), [this] {return !the_queue.empty(); })) {
					return false;
				}

				popped_value = the_queue.front();
				the_queue.pop();
				++m_pop;
			}
			the_condition_variable_push.notify_all();
			return true;
		}

		size_t size() const {
			std::unique_lock<std::mutex> lock(the_mutex);
			return the_queue.size();
		}

		unsigned int max_size() const {
			return m_max;
		}

		int push_count() const {
			std::unique_lock<std::mutex> lock(the_mutex);
			return m_push;
		}

		int pop_count() const {
			std::unique_lock<std::mutex> lock(the_mutex);
			return m_pop;
		}

	private:
		std::atomic<unsigned int> m_max;
		int m_push;
		int m_pop;
	};
} // /file_copy
