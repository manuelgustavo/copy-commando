#pragma once

#include "tools.h"
#include "task.h"
#include "file.h"

namespace file_copy {
	class copy_engine;
	class file_part_task : public task {
		friend class file;
	public:
		// Constructor
		// Parameters: 
		//    const file_ptr f: [in] file_ptr
		file_part_task(const file_ptr& f){
			m_fp = f;
		}

		// Destructor
		// doesn't close the file if still open
		~file_part_task() { 
		}
		// commits the file when the last write has been performed.
		virtual bool operator()() override;

		inline file_ptr get_fp() {
			return m_fp;
		}

		// Stores the buffer to be written. (Doesn't commits the data yet. To be used later by write())
		// Parameters: 
		//    void* buffer: [out] memory buffer where it will write into
		//    size_t count: [in,out] number of bytes to be read, and returns the number of bytes successfully read
		//
		// Returns bool: Success true, Failure false
		// Throws std::exception in case of serious issues.
		inline void write_buff_store(void* buffer, const size_t& count, bool last_write) {
			assert(buffer != nullptr);
			TRACE("Writing to buffer %d bytes\n");

			m_write_buff.reset(new char[count]);

			memcpy_s(m_write_buff.get(), count, buffer, count);
			m_write_buff_count = count;
			m_last_write = last_write;
		}

		inline bool is_last_write() {
			return m_last_write;
		}

		// Writes the buffer into the disk
		//
		// Returns bool: Success true, Failure false
		inline bool write_buff_commit() {
			if (m_write_buff && m_write_buff_count) {
				return m_fp->write(m_write_buff.get(), m_write_buff_count);
			} else {
				return false;
			}
		}

		// Sets the file attributes (not committing yet)
		inline void file_attributes(win32_attributes_ptr p) {
			m_attributes = p;
		}

	protected:
		bool m_last_write{ false };

		win32_attributes_ptr m_attributes;

		file_ptr m_fp;
		std::shared_ptr<char> m_write_buff;
		std::size_t m_write_buff_count;
	};

	using file_part_task_ptr = std::shared_ptr<file_part_task>;
}