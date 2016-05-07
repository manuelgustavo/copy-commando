#pragma once

#include <Windows.h>
#include <WinBase.h>

#include <string>
#include <stdio.h>
#include <fileapi.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <memory>
#include <filesystem>

#include "tools.h"
#include "task.h"
#include "file.h"

namespace file_copy {
	class folder_task : public task {
		friend class file;
	public:
		// Constructor
		// Parameters: 
		//    const file_ptr f: [in] file_ptr
		folder_task(const file_ptr& f) {
			m_fp = f;
		}

		// Destructor
		// doesn't close the file if still open
		~folder_task() {
		}
		// commits the file when the last write has been performed.
		virtual bool operator()() override {
			DWORD attr = m_fp->check_exists();
			if (attr == INVALID_FILE_ATTRIBUTES) { // it doesn't exist, create it!
				bool ret = create_dir(m_fp->path_full());
				if (ret)
					m_fp->commit_file_basic_info();
				return ret;
			}

			if (!(attr & FILE_ATTRIBUTE_READONLY) && (attr & FILE_ATTRIBUTE_DIRECTORY)) { // directory exists
				m_fp->commit_file_basic_info();
				return true;
			} else
				return false; // failed for another reason
		}

		inline file_ptr get_fp() {
			return m_fp;
		}

		inline void file_attributes(win32_attributes_ptr p) {
			m_attributes = p;
		}

	protected:
		win32_attributes_ptr m_attributes;

		file_ptr m_fp;
		std::shared_ptr<char> m_write_buff;
		std::size_t m_write_buff_count;
	};

	using folder_task_ptr = std::shared_ptr<folder_task>;
}