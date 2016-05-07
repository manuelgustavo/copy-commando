#pragma once

#include <thread>
#include <cassert>
#include <condition_variable>
#include <atomic>
#include "concurrent_queue.h"
#include "thread_tools.h"
#include "tools.h"
#include "task.h"

namespace file_copy {
	using task_queue = thread_tools::concurrent_queue<task_ptr>;
	using task_queue_ptr = std::shared_ptr<task_queue>;

	class task_sink : public thread_tools::thread_wrapper {
	public:
		task_sink(task_queue_ptr cq) :
			m_cq(cq) {
		}

		void commit();

		virtual void operator ()();

	private:
		task_queue_ptr m_cq;
		std::atomic<unsigned int> m_read_count{ 0 };
		std::atomic<unsigned int> m_write_count{ 0 };
		bool processNext();
	};

	using task_sink_ptr = std::shared_ptr<task_sink>;

} //End of namespace safestore
