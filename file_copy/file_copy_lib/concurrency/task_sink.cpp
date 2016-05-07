#include "StdAfx.h"
#include "task_sink.h"
#include "tools.h"
#include "task.h"
//#include "file_part_task.h"

namespace file_copy {
	using namespace std;
	using namespace thread_tools;

	bool task_sink::processNext() {
		std::string msg;

		task_ptr task;

		bool processed = false;
		if (m_cq->timed_wait_and_pop(task, 100)) {
			processed = true;
			++m_read_count;
			try {
				if (task.get()->operator()()) {
					++m_write_count;
				}
			} catch (exception& e) {
				TRACE("exception when processing task! %s\n", e.what());
				assert(0);
			}
		}
		return processed;
	}

	void task_sink::operator()() {
		TRACE("task sink thread started\n");
		notify_started();

		do {
			processNext();
		} while (!m_stop_now.load(memory_order_acquire));

		commit();
		TRACE("task sink thread finished\n");
	}

	void task_sink::commit() {
		TRACE("committing task queue\n");
		while (m_cq->size()) {
			processNext();
		}
	}
} // /file_copy