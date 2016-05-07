#pragma once

#include <thread>
#include <cassert>
#include <condition_variable>
#include <atomic>
#include "thread_tools.h"
#include "copy_engine.h"

class copy_queue_item {
public:
	copy_queue_item(const std::wstring& _source, const std::wstring& _dest) :
		source{ _source }, dest{ _dest } {
	}
	std::wstring source;
	std::wstring dest;
};

using copy_queue_item_ptr = std::shared_ptr<copy_queue_item>;
using copy_queue = thread_tools::concurrent_queue<copy_queue_item_ptr>;

class file_copy_thread : public thread_tools::thread_wrapper {
public:
	file_copy_thread() : m_copy{ file_copy::copy_engine::get_instance() } {}
	virtual ~file_copy_thread() {
		die();
	}
	enum class file_copy_status {
		idle,
		preparing,
		copying,
		completed,
		error,
		undefined
	};
	virtual void operator ()() {
		TRACE("file_copy_thread started! id: 0x%x\n", std::this_thread::get_id() );
		notify_started();
		
		m_copy.init();
		try {
			copy_queue_item_ptr item;

			while (m_queue.try_pop(item)) {
				status(file_copy_status::preparing);
				m_copy.copy_prepare(item->source, item->dest);
			}
			status(file_copy_status::copying);
			m_copy.copy_start();
			status(file_copy_status::completed);
		} catch (...) {
			assert(0);
			status(file_copy_status::error);
		}
		TRACE("file_copy_thread finished! id: 0x%x\n", std::this_thread::get_id());
	}

	virtual void add(const std::wstring& source, const std::wstring& dest) {
		copy_queue_item_ptr item{ new copy_queue_item{source, dest} };
		m_queue.push(item);
	}

	file_copy_status status() {
		return m_status.load();
	}

	bool async() {
		return m_copy.async();
	}

	const file_copy::file_ptr current_read_file() {
		return  m_copy.current_read_ts();
	}

	const file_copy::file_ptr current_write_file() {
		return  m_copy.current_write_ts();
	}

	uint64_t num_to_process() {
		return m_copy.num_files_to_process_ts() + m_copy.num_folders_to_process_ts();
	}

private:
	void status(const file_copy_status& v) {
		m_status.store(v);
	}

private:
	std::atomic<file_copy_status> m_status{ file_copy_status::idle };

	file_copy::copy_engine& m_copy;
	copy_queue m_queue{ MAXINT32 };
};




