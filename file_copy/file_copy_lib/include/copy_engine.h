#pragma once
#include <stdio.h>
#include <string>
#include <vector>
#include <cassert>
#include <sstream>
#include <utility>
#include <future>

#include "file.h"
#include "file_part_task.h"
#include "folder_task.h"
#include "concurrent_queue.h"
#include "task_sink.h"


namespace file_copy {
	class files_to_process {
		friend class copy_engine;
	public:

		enum class files_to_process_status {
			idle,
			source_access_denied,
			dest_access_denied,
			started,
			finished,
			source_file_error,
			dest_file_error,
		};

		~files_to_process() {}

		// Thread safe
		// Returns the status of the files_to_process object
		//    Returns files_to_process_status.
		inline files_to_process_status get_status_ts() {
			auto source_status = m_source->status_ts();
			auto dest_status = m_dest->status_ts();

			switch (dest_status) {
			case file::file_status::failed_open:
				return files_to_process_status::dest_access_denied;
			case file::file_status::failed:
				return files_to_process_status::dest_file_error;
			case file::file_status::closed_write:
				return files_to_process_status::finished;
			}

			switch (source_status) {
			case file::file_status::idle:
				return files_to_process_status::idle;
			case file::file_status::failed_open:
				return files_to_process_status::source_access_denied;
			case file::file_status::failed:
				return files_to_process_status::source_file_error;
			default:
				return files_to_process_status::started;
			}
		}

		inline file_ptr source() const {
			return m_source;
		}

		inline file_ptr dest() const {
			return m_dest;
		}
	
	private:
		files_to_process(const file_ptr& source, const file_ptr& dest)
			: m_source{ source }, m_dest{ dest } {}

		file_ptr m_source;
		file_ptr m_dest;
	};

	class async_crc32 {
	public:
		async_crc32(const char* data, size_t count, uint32_t crc32 = 0) : m_count{ count }, m_crc32{ crc32 } {
			m_data = new char[count];
			memcpy_s(m_data, count, data, count);
		}
		~async_crc32() {
			if (m_data)
				delete[] m_data;
		}

		async_crc32(const async_crc32& v) {
			m_count = v.m_count;
			m_crc32 = v.m_crc32;
			m_data = new char[m_count];
			memcpy_s(m_data, m_count, v.m_data, m_count);
		}

		async_crc32& operator=(const async_crc32& v) {
			if (&v != this) {
				if (m_data)
					delete[] m_data;
				m_count = v.m_count;
				m_crc32 = v.m_crc32;
				m_data = new char[m_count];
				memcpy_s(m_data, m_count, v.m_data, m_count);
			}
			return *this;
		}

		async_crc32(async_crc32&& v) {// = delete;
			m_data = v.m_data;
			v.m_data = nullptr;
			m_count = v.m_count;
			v.m_count = 0;
			m_crc32 = v.m_crc32;
			v.m_crc32 = 0;
		}

		async_crc32& operator=(async_crc32&& v) {// = delete;
			if (&v != this) {
				m_data = v.m_data;
				v.m_data = nullptr;
				m_count = v.m_count;
				v.m_count = 0;
				m_crc32 = v.m_crc32;
				v.m_crc32 = 0;
			}
			return *this;
		}

		uint32_t operator()() {
			return crc32::crc32_16bytes_prefetch(m_data, m_count, m_crc32);
		}
	private:
		char* m_data{ nullptr };
		size_t m_count{ 0 };
		uint32_t m_crc32{ 0 };
	};

	//using files_to_process = std::pair<file_ptr, file_ptr>; // usage files_to_process{file_ptr source, file_ptr dest}
	using file_to_process_vector = std::vector<files_to_process>;

	class copy_engine {
		friend class file_part_task;
	public:
		enum class async_mode {
			automatic,
			sync,
			async
		};

		~copy_engine() {
			//async(false); // kill the async thread if existing
		}

		static copy_engine& get_instance() {
			static copy_engine instance{};
			return instance;
		}

		// Prepares the copy of the file/folder into the destination folder.
		// Flags if the copy will be done sync or async (multithreaded)
		// Parameters: 
		//    const std::string& source: [in] source file / folder
		//    const std::string& dest_folder: [in] destination folder
		void copy_prepare(const std::wstring& source, const std::wstring& dest_folder) {
			file_ptr _source{ new file{ source } };
			file_ptr _dest{ new file{ dest_folder, std::wstring{} } };
			/*if (_source->folder() == _dest->path()) {
				_dest->rename_to_non_existing();
			}*/

			async(async_decision(_source, _dest));
			build_files_to_process(_source, _dest, _source->folder() == _dest->path());
			uint64_t remove;
				
			DWORD err= get_disk_free_space(_dest->root_full(), remove);
		}

		// Starts the copy process (call copy_prepare(...) first)
		// Parameters: 
		//    const async_mode& force_mode = async_mode::automatic: 
		//       Forces to sync or async. 
		//       Default is automatic, leaving the decision to the application
		void copy_start(const async_mode& force_mode = async_mode::automatic) {
			
			switch (force_mode) {
			case async_mode::automatic: // do nothing as the decision was made in copy_prepare
				break;
			case async_mode::async:
				async(true);
				break;
			case async_mode::sync:
				async(false);
				break;
			default: // not possible to reach
				assert(0);
			}

			if (!m_task_sink)
				m_task_sink.reset(new task_sink{ m_task_queue });

			if (async() && !m_sink_thread) {
				m_sink_thread = m_task_sink->run();
			}

			for (auto x : m_files_to_process) {
				copy_file(x.m_source, x.m_dest);
			}

			if (!m_async.load()) {
				commit();
			}

			if (m_sink_thread) {
				if (m_task_sink)
					m_task_sink->die();
				m_sink_thread = nullptr;
			}
			if (m_task_sink)
				m_task_sink = nullptr;
		}

		// initialized the copy engine
		void init(const unsigned int& task_queue_size = 3000) {
			m_task_queue.reset(new task_queue{ task_queue_size });
		}

		// Is the current mode assynchronous?
		//
		// Returns bool: true = yes, false = no
		bool async() const {
			return m_async.load();
		}

		// Thread Safe: Returns the total size of files to be processed (after a call to build_files_to_process)
		uint64_t files_to_process_total_size_bytes_ts() {
			return m_files_to_process_total_size.load();
		}

		// Thread Safe: Returns the total number of files to be processed (after a call to build_files_to_process)
		uint64_t num_files_to_process_ts() {
			return m_num_files_to_process.load();
		}

		// Thread Safe: Returns the total number of folders to be processed (after a call to build_files_to_process)
		uint64_t num_folders_to_process_ts() {
			uint64_t ret = m_num_folders_to_process.load();
			return ret ? ret - 1 : 0U;
		}

		const file_to_process_vector& get_files_to_process() const {
			return m_files_to_process;
		}

		file_ptr current_read_ts() {
			//double checked locking pattern
			if (m_mutex_current_read_is_dirty.load()) {
				std::lock_guard<std::mutex> l(m_mutex_current_read);
				if (m_mutex_current_read_is_dirty.load()) {
					m_current_read_local = m_current_read;
					m_mutex_current_read_is_dirty.store(false);
				}
			}
			return m_current_read_local;
		}

		file_ptr current_write_ts() {
			//double checked locking pattern
			if (m_mutex_current_write_is_dirty.load()) {
				std::lock_guard<std::mutex> l(m_mutex_current_write);
				if (m_mutex_current_write_is_dirty.load()) {
					m_current_write_local = m_current_write;
					m_mutex_current_write_is_dirty.store(false);
				}
			}
			return m_current_write_local;
		}

	protected:
		copy_engine() {}

		// Returns true if the files should be copied asynchronously (eg. distinct physical drives)
		bool async_decision(const file_ptr& source, const file_ptr& dest) const {
			bool ret = true;

			VOLUME_DISK_EXTENTS source_extents;
			VOLUME_DISK_EXTENTS dest_extents;

			DWORD res;
			res = get_disk_extents(source->root_full(), source_extents);
			if (res) {
				std::wostringstream os;
				os << "Async decision failed : root: " << source->root_full().c_str()
					<< std::showbase << std::setfill(_T('0')) << std::setw(4) << std::hex << res;
				TRACE(_T("%s\n"), os.str().c_str());
				throw std::runtime_error(wstring_to_string(os.str()));
			}

			res = get_disk_extents(dest->root_full(), dest_extents);
			if (res) {
				std::wostringstream os;
				os << "Async decision failed : root: " << source->root_full().c_str()
					<< std::showbase << std::setfill(_T('0')) << std::setw(4) << std::hex << res;
				TRACE(_T("%s\n"), os.str().c_str());
				throw std::runtime_error(wstring_to_string(os.str()));
			}

			// same physical disk identification
			if ((source_extents.NumberOfDiskExtents == 1) && (dest_extents.NumberOfDiskExtents == 1)
				&& (source_extents.Extents[0].DiskNumber == dest_extents.Extents[0].DiskNumber)) {
				ret = false;
			}

			TRACE(_T("Source : NumberOfDiskExtents : %d\nDestination : NumberOfDiskExtents : %d\nasync decision is that the file(s) will be copied %s from \"%s\" device[%d] to \"%s\" device[%d]\n"),
				source_extents.NumberOfDiskExtents,
				dest_extents.NumberOfDiskExtents,
				ret ? _T("asynchronously") : _T("synchronously"),
				source->path_full().c_str(),
				source_extents.Extents[0].DiskNumber,
				dest->path_full().c_str(),
				dest_extents.Extents[0].DiskNumber);

			return ret;
		}

		// Flags the copy into async (multithreaded) mode, good for distinct devices. Only used internally by the class.
		// Parameters: 
		//    const bool& v: [in] true = async, false = not async
		void async(const bool& v) {
			m_async.store(v);
		}

		struct build_files_to_process_res {
			uint64_t files{ 0U };
			uint64_t folders{ 0U };
			uint64_t size{ 0U };
		};

		// Appends files and folders in the m_files_to_process structure.
		build_files_to_process_res build_files_to_process(file_ptr source, file_ptr dest, bool rename_existing = false) {
			//TRACE(_T("Adding file to process: %s attributes: %x system: %s\n"), f->path().c_str(), f->win32_attributes()->dwFileAttributes, f->win32_attributes()->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ? _T("true") : _T("false"));
			//TRACE(_T("%s "), f->path().c_str());

			build_files_to_process_res res;

			/*if(!dest->file_name().size() && source->file_name().size())
				dest->file_name(source->file_name());
			m_files_to_process.push_back(files_to_process{ source, dest });*/
			if (!dest->file_name().size() && source->file_name().size())
				dest->file_name(source->file_name());

			if (rename_existing) {
				dest->rename_to_non_existing();
			}

			if (source->is_directory()) {
				++m_num_folders_to_process;
				++res.folders;
				WIN32_FIND_DATA find_file_data;
				HANDLE h_find;
				// root doesn't have a file name
				h_find = FindFirstFileW((source->path_full() + _T("\\*")).c_str(), &find_file_data);
				if (h_find == INVALID_HANDLE_VALUE) {
					TRACE(_T("Folder Access Denied: %s"), source->path_full().c_str());
					source->status_ts(file::file_status::failed_open);
				}
				while (h_find != INVALID_HANDLE_VALUE) {
					if (!(find_file_data.cFileName[0] == _T('.') && find_file_data.cFileName[1] == _T('\0'))
						&& !(find_file_data.cFileName[0] == _T('.') && find_file_data.cFileName[1] == _T('.') && find_file_data.cFileName[2] == _T('\0'))) {
						///*&& /*AVOID SYSTEM FOLDERS*/!((find_file_data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) && (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))) /*!(f->is_root() && (wcscmp(find_file_data.cFileName, _T("$RECYCLE.BIN"))==0))) /*(wcscmp(find_file_data.cFileName, _T(".")) != 0) && (wcscmp(find_file_data.cFileName, _T("..")) != 0)*/ {
						win32_attributes_ptr attributes{ new WIN32_FILE_ATTRIBUTE_DATA };
						attributes->dwFileAttributes = find_file_data.dwFileAttributes;
						attributes->ftCreationTime = find_file_data.ftCreationTime;
						attributes->ftLastAccessTime = find_file_data.ftLastAccessTime;
						attributes->ftLastWriteTime = find_file_data.ftLastWriteTime;
						attributes->nFileSizeHigh = find_file_data.nFileSizeHigh;
						attributes->nFileSizeLow = find_file_data.nFileSizeLow;

						file_ptr file_to_add_source{ new file{ source->path(), find_file_data.cFileName, source } };
						file_to_add_source->win32_attributes(attributes);
						file_ptr file_to_add_dest{ new file{ dest->path(), find_file_data.cFileName, dest } };

						build_files_to_process_res res_aux = build_files_to_process(file_to_add_source, file_to_add_dest);
						res.size += res_aux.size;
						res.files += res_aux.files;
						res.folders += res_aux.folders;
					}

					if (!FindNextFileW(h_find, &find_file_data)) {
						FindClose(h_find);
						h_find = INVALID_HANDLE_VALUE;
					}
				} 
				source->size_ts(res.size); // sets the directory size
				TRACE(_T("Folder: \"%s\"\nFiles: %d\nSubfolders: %d Size: %d\n"), source->path_full().c_str(), res.files, res.folders-1 /*subtract one to exclude the current folder*/, res.size);
			} else {
				uint64_t size = source->size_ts();
				assert(size != _UI64_MAX);
				m_files_to_process_total_size += size;
				res.size = size;

				++m_num_files_to_process;
				++res.files;
			}
			
			// last thing to do is to push the file because the directory properties must be set for last
			m_files_to_process.push_back(files_to_process{ source, dest });
			
			return res;
		}

		// Copies the file from source into destination
		// Parameters: 
		//    void* buffer: [out] memory buffer where it will read into
		//    size_t count: [in,out] number of bytes to be read, and returns the number of bytes successfully read
		//
		// Returns int64_t: File size
		// Throws std::exception in case of serious issues.
		void copy_file(file_ptr source, file_ptr dest) {
			errno_t res;
			size_t count = READ_SIZE;

			uint32_t crc32 = 0;

			if (!source->is_directory()) {
				{ // update copy engine's monitoring variable
					static file* prev_file = nullptr;
					if (prev_file != source.get()) {
						prev_file = source.get();
						current_read_ts(source);
					}
				}
				res = source->open_read();
			}
			
			bool success = false;
			dest->win32_attributes(source->win32_attributes());
			bool first_run = true;
			std::future<uint32_t> fut_crc;

			//fut_crc = std::async(test_async,1);

			do {
				task_ptr task;
				if (dest->is_directory()) {
					folder_task_ptr folder{ new folder_task{dest} };
					task = folder;
					success = true;
				} else {
					file_part_task_ptr dest_part{ new file_part_task{ dest } };
					success = source->read(m_buff, count);
					if (!first_run) {
						crc32 = fut_crc.get();
					} else {
						first_run = false;
					}

					if (success) {
						async_crc32 async_task(m_buff, count, crc32);
						fut_crc = std::async(/*std::launch::async,*/ async_task);
					} else {
						source->status_ts(file::file_status::failed_open);
					}

					/*if (success) {
						crc32 = crc32::crc32_16bytes_prefetch(m_buff, count, crc32);
					} else {
						source->failed(true);
					}*/
					dest_part->write_buff_store(m_buff, count, source->is_eof());
					task = dest_part;
				}
				if (!m_async.load()) {
					if (m_task_queue->size() == m_task_queue->max_size())
						commit();
				}
				m_task_queue->push(task);

			} while (success && (source->is_directory() ? false : !source->is_eof()));

			/*if (!m_async.load()) {
				commit();
			}*/

			if (success && (source->is_directory() ? false : source->is_eof())) {
				crc32 = fut_crc.get();
				source->crc32_ts(crc32);
			}
			source->close(); // dest will be closed automatically during the last write.
		};

		void current_read_ts(file_ptr v) {
			std::lock_guard<std::mutex> l(m_mutex_current_read);
			m_mutex_current_read_is_dirty.store(true);
			m_current_read = v;
		}

		void current_write_ts(file_ptr v) {
			std::lock_guard<std::mutex> l(m_mutex_current_write);
			m_mutex_current_write_is_dirty.store(true);
			m_current_write = v;
		}

		void commit() {
			m_task_sink->commit();
		}

		void stop_and_wait_sink_thread() {
			if (m_sink_thread) {
				if (m_task_sink)
					m_task_sink->die();
				m_sink_thread = nullptr;
			}
		}

	protected:
		
		task_queue_ptr m_task_queue;
		task_sink_ptr m_task_sink;
		std::shared_ptr<std::thread> m_sink_thread;

		std::atomic<uint64_t> m_files_to_process_total_size{ 0 };
		std::atomic<uint64_t> m_num_files_to_process{ 0 };
		std::atomic<uint64_t> m_num_folders_to_process{ 0 };

		char m_buff[READ_SIZE];

		file_to_process_vector m_files_to_process;

		std::atomic<bool> m_async{ false };

		std::mutex m_mutex_current_read;
		file_ptr m_current_read;
		file_ptr m_current_read_local;
		std::atomic<bool> m_mutex_current_read_is_dirty{ false };

		std::mutex m_mutex_current_write;
		file_ptr m_current_write;
		file_ptr m_current_write_local;
		std::atomic<bool> m_mutex_current_write_is_dirty{ false };
	};
}