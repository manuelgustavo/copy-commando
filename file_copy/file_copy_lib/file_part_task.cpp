#include "stdafx.h"

#include "file_part_task.h"
#include "copy_engine.h"

using namespace file_copy;

bool file_part_task::operator()() {
	bool ret = true;
	try {
		if (!m_fp->get_FILE()) {
			if (m_fp->check_exists() != INVALID_FILE_ATTRIBUTES) { // file already exists!
				TRACE(_T("File already exists : file path : %s\n"), m_fp->path_full());
				// decide what to do
				auto choice = m_fp->on_existing();

				switch (choice) {
				case file::exist_decision::skip:
					m_fp->status_ts(file::file_status::skipped);
					TRACE(_T("Skipping file : file path : %s\n"), m_fp->path_full().c_str());
					return true;
				case file::exist_decision::overwrite:
					TRACE(_T("Overwriting file : file path : %s\n"), m_fp->path_full().c_str());
					break;
				case file::exist_decision::rename: {// TODO rename the file
					TRACE(_T("Renaming file : file path : %s\n"), m_fp->path_full().c_str());
#ifdef _DEBUG
					std::wstring original_file_name = m_fp->path_full().c_str();
#endif
					m_fp->rename_to_non_existing();
#ifdef _DEBUG
					TRACE(_T("Renamed file : %s to file : %s\n"), original_file_name, m_fp->file_name());
#endif
				} break;
				case file::exist_decision::ask: {
					std::wostringstream os;
					os << "Writing failed! : \"exist_decision::ask\" shouldn't be set : file path: " << m_fp->path_full();
					TRACE(_T("%s\n"), os.str().c_str());
					throw std::runtime_error(wstring_to_string(os.str()));
				} break;
				default: {
					std::wostringstream os;
					os << "Writing failed! : unknown decision : file path: " << m_fp->path_full();
					TRACE(_T("%s\n"), os.str().c_str());
					throw std::runtime_error(wstring_to_string(os.str()));
				}
				}
			}
			
			{ // update copy engine's monitoring variable
				static file* prev_file = nullptr;
				if (prev_file != m_fp.get()) {
					prev_file = m_fp.get();
					copy_engine::get_instance().current_write_ts(m_fp);
				}
			}

			auto res = m_fp->open_write_preallocate();
			if (res) {
				if (!create_dir(m_fp->folder()))
					ret = false;
				else {
					res = m_fp->open_write_preallocate();
				}
			}
			if (!ret) {
				if (res) {
					std::wostringstream os;
					os << "Writing failed! : Couldn't open and preallocate file : file path: " << m_fp->path_full()
						<< " : " << std::showbase << std::setfill(_T('0')) << std::setw(4) << std::hex << res;
					TRACE(_T("%s\n"), os.str().c_str());
					throw std::runtime_error(wstring_to_string(os.str()));
				} else {
					std::wostringstream os;
					os << "Writing failed! : Couldn't open file : file path: " << m_fp->path_full();
					TRACE(_T("%s\n"), os.str().c_str());
					throw std::runtime_error(wstring_to_string(os.str()));
				}
			}
		}
		if (!write_buff_commit()) {
			m_fp->status_ts(file::file_status::failed);
		}
		if (is_last_write()) {
			m_fp->commit_file_basic_info();
			m_fp->close();
		}
	} catch (std::exception& e) {
		TRACE("exception when processing write! %s\n", e.what());
		ret = false;
	}
	return ret;
}
