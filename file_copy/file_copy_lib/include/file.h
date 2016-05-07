#pragma once

#include <Windows.h>
#include <WinBase.h>
#include <stdio.h>
#include <string>
#include <fileapi.h>
#include <memory>
#include <sstream>
#include <iomanip>
#include <io.h>
#include <fcntl.h>
#include <atomic>
#include "tools.h"
#include "crc32.h"


namespace file_copy {
	using FILE_ptr = std::shared_ptr<FILE*>;
	class file;
	using file_ptr = std::shared_ptr<file>;
	class file {
		friend class file_part_task;
		friend class copy_engine;

	public:

		enum class file_status {
			idle,
			open_read,
			open_write,
			closed_read,
			closed_write,
			failed_open,
			skipped,
			failed
		};

		enum class exist_decision {
			ask,
			skip,
			overwrite,
			rename
		};

		// Constructor
		// Parameters: 
		//    const std::string& path: [in] path of the file
		file(const std::wstring& path, file_ptr parent = nullptr, bool no_write_syscache = true) : m_parent{ parent } {
			std::size_t pos = path.find_last_of(_T('\\'));
			if (pos != std::string::npos) {
				if(!m_parent)
					folder(path.substr(0, pos));
				file_name(path.substr(pos + 1));
			} else {
				if (!m_parent)
					folder(path);
				m_is_root.store(true);
			}
		}

		// Constructor
		// Parameters: 
		//    const std::string& folder: [in] folder of the file (without trailing "\")
		//    const std::string& file_name: [in] file name
		file(const std::wstring& folder, const std::wstring& file_name, file_ptr parent = nullptr, bool no_write_syscache = true) : m_file_name(file_name), m_parent{ parent }, m_no_write_syscache{ no_write_syscache } {
			if (!m_parent)
				m_folder = folder;
			std::size_t pos = path().find_last_of(_T('\\'));
			if (pos == std::string::npos) {
				m_is_root.store(true);
			}
		}

		// TODO
		file(const file&) = delete;

		// TODO
		file& operator=(const file&) = delete;

		// TODO
		file(file&&) = delete;

		// TODO
		file& operator=(file&&) = delete;

		virtual ~file() {
			close();
		}

		// Returns the full file path (including initial "\\\\?\\")
		// Returns: std::stringw
		inline std::wstring path_full() {
			return _T("\\\\?\\") + path();
		}

		// Called when a decision about the file must be made.
		// Returns: exist_decision 
		inline exist_decision on_existing() {
			if (m_exist_decision == exist_decision::ask) {
				m_exist_decision.store(on_existing_ask());
			}
			return m_exist_decision.load();
		}

		// Makes the decision (TODO) on what to do if the file needs to be asked for overwrite or rename.
		// Returns: exist_decision
		inline exist_decision on_existing_ask() {
			return exist_decision::rename;
		}

		// Thread safe
		// returns the current decision if a file exists
		// Returns: exist_decision
		inline exist_decision exist_choice_ts() const{
			return m_exist_decision.load();
		}

		// Thread safe
		// Flags the file as failed.
		// Parameters: 
		//    const bool& b: true = failed, false = not failed
		inline void exist_choice_ts(const exist_decision& v) {
			m_exist_decision.store(v);
		}

		

		// Returns the root of the file path (including initial "\\\\?\\")
		// Returns: std::stringw
		inline std::wstring root() const {
			assert(path().size());
			std::wstring aux = path();

			if (aux[1] == _T(':'))
				return aux.substr(0, 2);
			else
				return aux.substr(0, aux.substr(2).find_first_of(_T("\\")));
		}

		// Returns the root of the file path (including initial "\\\\?\\")
		// Returns: std::stringw
		inline std::wstring root_full() const {
			return _T("\\\\?\\") + root();
		}
		
		/*// Returns the full file path in wide characters (including initial "\\\\?\\")
		// Returns: std::stringw
		inline std::wstring path_full_w() {
			return string_to_wstring(path_full());
		}*/

		// Thread safe
		// Flags the file as failed.
		// Parameters: 
		//    const bool& b: true = failed, false = not failed
		/*inline void failed_ts(const bool& b) {
			m_failed.store(b);
		}

		// Thread safe
		// Returns if the file has been flagged as failed.
		// Returns: bool: true = failed, false = not failed
		inline bool failed_ts() {
			return m_failed.load();
		}*/
		
		// Gets the file attributes. Reads them if they weren't read before
		// Returns: filled win32_attributes_ptr, otherwise empty win32_attributes_ptr
		// Throws std::exception in case of serious issues.
		inline win32_attributes_ptr win32_attributes() {
			if (!m_attributes) {
				DWORD err = read_file_attributes();
				if (err) {
					std::wostringstream os;
					os << "Error when getting file attributes : System Error Code: "
						<< std::showbase << std::setfill(_T('0')) << std::setw(4) << std::hex << err;

					throw std::runtime_error(wstring_to_string(os.str()));
				}
			}
			return m_attributes;
		}
		
		// Sets the file attributes. 
		// Parameters: 
		//    win32_attributes_ptr p: attributes to be set
		inline void win32_attributes(const win32_attributes_ptr& p) {
			assert(p);

			m_attributes = p;
		}

		// Returns the size of the file/directory (for folders, the size must be set with size_ts(...).
		// Returns int64_t: File size. _UI64_MAX in case it's a folder and the value hasn't been set yet 
		// Throws std::exception in case of serious issues.
		inline int64_t size_ts() {
			if (!m_attributes) {
				DWORD err = read_file_attributes();
				if (err) {
					std::wostringstream os;
					os << "Error when getting file size : " << path_full() << " : System Error Code: "
						<< std::showbase << std::setfill(_T('0')) << std::setw(4) << std::hex << err;

					throw std::runtime_error(wstring_to_string(os.str()));
				}
			}
			int64_t ret = 0;
			if (!is_directory()) {
				ret = static_cast<int64_t>(m_attributes->nFileSizeHigh) << 32 | m_attributes->nFileSizeLow;
			} else {
				ret = m_size.load();
			}
			return ret;
		}

		// Only valide for directories. Sets the size of the directory (for folders, the size must be set with size_ts(...).
		// Parameters: 
		//    const uint64_t& v: attributes to be set
		inline void size_ts(const uint64_t& v) {
			m_size.store(v);
		}

		// Sets the file/folder base folder.
		//    const std::wstring& v: file name.
		inline void folder(const std::wstring& v) {
			m_parent = nullptr;
			m_folder = v;
		}

		// Returns the file/folder base folder.
		// Returns: std::string
		inline std::wstring folder() const {
			if (m_parent)
				return m_parent->path();
			else
				return m_folder;
		}

		// Sets the file/folder name.
		//    const std::wstring& v: file name.
		inline void file_name(const std::wstring& v) {
			m_file_name = v;
		}

		// Returns the file/folder name.
		// Returns: std::string
		inline std::wstring file_name() const {
			return m_file_name;
		}

		// Opens the file for reading.
		// Returns bool: errno_t
		inline errno_t open_read() {
			m_read = true;
			const wchar_t fopen_flags[] = _T("rb");
			if (m_FILE) {
				TRACE(_T("file already open: %s\n"), path_full().c_str());
				return 0;
			}

			m_FILE.reset(new FILE*);
			TRACE(_T("Opening file: %s\n"), path_full().c_str());
			errno_t res = _wfopen_s(m_FILE.get(), path_full().c_str(), fopen_flags);
			if (res)
				m_FILE = nullptr;
			else
				status_ts(file_status::open_read);

			TRACE(_T("Opening file: %s return result: %s\n"), path_full().c_str(), get_errno_desc(res).c_str());

			return res;
		}

		// Opens the file for writing.
		// Returns bool: errno_t
		inline errno_t open_write() {
			const wchar_t fopen_flags[] = _T("wb");
			if (m_FILE) {
				TRACE(_T("file already open: %s\n"), path_full().c_str());
				return 0;
			}

			m_FILE.reset(new FILE*);
			TRACE(_T("Opening file: %s\n"), path_full().c_str());
			errno_t res = _wfopen_s(m_FILE.get(), path_full().c_str(), fopen_flags);
			if (res) {
				m_FILE = nullptr;
				status_ts(file_status::failed_open);
			}
			else
				status_ts(file_status::open_write);

			TRACE(_T("Opening file: %s return result: %s\n"), path_full().c_str(), get_errno_desc(res).c_str());

			return res;
		}

		// Preallocates and opens the file for writing
		//
		// Returns bool: errno_t
		inline errno_t open_write_preallocate() {
			if (m_FILE) {
				TRACE(_T("file already open: %s\n"), path_full().c_str());
				return EACCES;
			}

			errno_t res = 0;

			TRACE(_T("Opening file: %s\n"), path_full().c_str());

			HANDLE h_file = preallocate();

			if (h_file == INVALID_HANDLE_VALUE) {
				res = EACCES;
				status_ts(file_status::failed_open);
			} else {
				int fd = _open_osfhandle((intptr_t)h_file, _O_RDWR);
				m_FILE.reset(new FILE*);
				*m_FILE = _fdopen(fd, "rb+");
				fseek(*m_FILE, 0, SEEK_SET);
				if (!*m_FILE) {
					int _errno;
					res = _get_errno(&_errno);
					status_ts(file_status::failed_open);
				} else {
					status_ts(file_status::open_write);
				}
			}
	
			TRACE(_T("Opening file: %s return result: %s\n"), path_full().c_str(), get_errno_desc(res).c_str());

			if (res)
				status_ts(file_status::failed_open);

			return res;
		}

		// Is the file open?
		// Returns: true = yes, false = no.
		inline bool is_open() {
			return m_FILE ? true : false;
		}

		// Returns the raw file pointer.
		// Returns: FILE*
		inline FILE_ptr get_FILE() {
			return m_FILE;
		}

		// Thread Safe
		// Gets the file path.
		//
		// Returns std::string: the path of the file.
		inline std::wstring path() const {
			return file_name().size() ? folder() + _T("\\") + file_name() : folder();
			//if (m_file_name.size())
			//std::wstring _file_name = file_name();
			//std::wstring _folder = folder();
			

			//std::wstring parent_path = m_parent->path();
			
			/*return _file_name.size() ? parent_path + _T("\\") + _file_name;
			if (m_parent) {
				
				return _file_name.size() ? parent_path + _T("\\") + _file_name : m_parent->path();
			} else {
				return 	_file_name.size() ? _folder + _T("\\") + _file_name : _folder;
			}*/

			//else
				//return m_folder;
		}

		
		// Check if the file_part_task reached the EOF
		// Returns bool: is EOF true, not EOF false
		inline bool is_eof() {
			if (m_FILE)
				return feof(*m_FILE) ? true : false;
			else {
				std::wostringstream os;
				os << "End Of file failed : file not open : file name: " << path_full();
				TRACE(_T("%s\n"), os.str().c_str());
				throw std::runtime_error(wstring_to_string(os.str()));
			}

		}

		// Closes the file if open
		inline void close(bool failed = false) {
			if (m_FILE) {
				if (!m_read && !m_no_write_syscache) {
					fflush(*m_FILE);
					if (_commit(_fileno(*m_FILE))) {
						std::wostringstream os;
						os << "Closing failed : could not close : file name: " << path_full();
						TRACE(_T("%s\n"), os.str().c_str());
						throw std::runtime_error(wstring_to_string(os.str()));
					}
					TRACE(_T("Closing file: %s"), path_full().c_str());
				}
				status_ts(failed ? file_status::failed : m_read ? file_status::closed_read : file_status::closed_write);
				fclose(*m_FILE);
				m_FILE = nullptr;
			}
		}

		// Reads the file.
		// Parameters: 
		//    void* buffer: [out] memory buffer where it will read into
		//    size_t count: [in,out] number of bytes to be read, and returns the number of bytes successfully read
		//
		// Returns bool: Success true, Failure false
		// Throws std::exception in case of serious issues.
		inline bool read(void* buffer, size_t& count) {
			assert(buffer != nullptr);

			bool ret{ true };
			size_t num_read = count;

			TRACE(_T("Reading %d bytes of file: %s\n"), count, path_full().c_str());
			if (m_FILE && *m_FILE) {
				num_read = fread_s(buffer, count, sizeof(char), count, *m_FILE);
				//fflush(*m_fp.get());
				if (num_read != count) {
					if (!feof(*m_FILE)) {
						if (ferror(*m_FILE)) {
							ret = false;
							close(true);
						} else {
							std::wostringstream os;
							os << "Reading failed! : Unknown error : file name: " << path_full()
								<< " read: " << ret;
							TRACE(_T("%s\n"), os.str().c_str());
							throw std::runtime_error(wstring_to_string(os.str()));
						}
					}
				}
				TRACE(_T("Read %d bytes of file: %s\n"), num_read, path_full().c_str());
			} else {
				std::wostringstream os;
				os << "Reading failed : file not open : file name: " << path_full()
					<< " count: " << count;
				TRACE(_T("%s\n"), os.str().c_str());
				throw std::runtime_error(wstring_to_string(os.str()));
			}
			count = num_read;
			return ret;
		}

		// Writes the file.
		// Parameters: 
		//    void* buffer: [out] memory buffer where it will write into
		//    size_t count: [in,out] number of bytes to be read, and returns the number of bytes successfully read
		//
		// Returns bool: Success true, Failure false
		// Throws std::exception in case of serious issues.
		inline bool write(void* buffer, size_t& count) {
			assert(buffer != nullptr);

			bool ret{ true };
			size_t num_written = count;

			TRACE(_T("Writing %d bytes of file: %s\n"), count, path_full().c_str());
			if (m_FILE && *m_FILE) {
				num_written = fwrite(buffer, sizeof(char), count, *m_FILE);

				if (num_written != count) {
					if (!ferror(*m_FILE)) {
						ret = false;
						close(true);
					} else {
						std::wostringstream os;
						os << "Writing failed! : Unknown error : file name: " << path_full()
							<< " written: " << ret;
						TRACE(_T("%s\n"), os.str().c_str());
						throw std::runtime_error(wstring_to_string(os.str()));
					}
				}
				TRACE(_T("Wrote %d bytes of file: %s\n"), num_written, path_full().c_str());
			} else {
				std::wostringstream os;
				os << "Writing failed : file not open : file name: " << path_full()
					<< " count: " << count;
				TRACE(_T("%s\n"), os.str().c_str());
				throw std::runtime_error(wstring_to_string(os.str()));
			}

			/*if (m_no_write_syscache) {
				fflush(*m_FILE);
				if (_commit(_fileno(*m_FILE))) {
					std::wostringstream os;
					os << "Writing failed : could not commit : file name: " << path_full();
					TRACE(_T("%s\n"), os.str().c_str());
					throw std::runtime_error(wstring_to_string(os.str()));
				}
			}*/
			count = num_written;
			return ret;
		}

		// Stores the file timestamps based on a WIN32_FILE_ATTRIBUTE_DATA (doesn't ommits yet)
		// throws std::exception if anything goes wrong
		inline file_basic_info_ptr file_basic_info() {
			TRACE(_T("Getting file basic info (times and basic attributes) for: %s\n"), path_full().c_str());

			if (!m_attributes)
				read_file_attributes();

			file_basic_info_ptr basic_info(new FILE_BASIC_INFO);

			basic_info->CreationTime.HighPart = m_attributes->ftCreationTime.dwHighDateTime;
			basic_info->CreationTime.LowPart = m_attributes->ftCreationTime.dwLowDateTime;
			basic_info->ChangeTime.HighPart = m_attributes->ftLastWriteTime.dwHighDateTime;
			basic_info->ChangeTime.LowPart = m_attributes->ftLastWriteTime.dwLowDateTime;
			basic_info->LastAccessTime.HighPart = m_attributes->ftLastAccessTime.dwHighDateTime;
			basic_info->LastAccessTime.LowPart = m_attributes->ftLastAccessTime.dwLowDateTime;
			basic_info->LastWriteTime.HighPart = m_attributes->ftLastWriteTime.dwHighDateTime;
			basic_info->LastWriteTime.LowPart = m_attributes->ftLastWriteTime.dwLowDateTime;
			basic_info->FileAttributes = m_attributes->dwFileAttributes;

			return basic_info;
		}

		// Saves (writes into the file) the file timestamps based on a WIN32_FILE_ATTRIBUTE_DATA
		// throws std::exception if anything goes wrong
		inline void commit_file_basic_info() {
			TRACE(_T("Setting file basic info (times and basic attributes) for: %s\n : is_directory \"%s\""), path_full().c_str(), is_directory() ? _T("true") : _T("false"));
			HANDLE h_file = INVALID_HANDLE_VALUE;
			if (m_FILE) { // use the FILE* in case it exists.
				//throw std::runtime_error("file handle is already open! It must be closed before usage");
				h_file = (HANDLE)_get_osfhandle(_fileno(*m_FILE));
			} else {

				h_file = CreateFileW(path_full().c_str(), GENERIC_WRITE,//GENERIC_READ | GENERIC_WRITE | DELETE,
					FILE_SHARE_WRITE | FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					is_directory() ? FILE_FLAG_BACKUP_SEMANTICS : NULL,
					NULL);
			}

			if (h_file != INVALID_HANDLE_VALUE) {
				if (!SetFileInformationByHandle(h_file, FileBasicInfo, file_basic_info().get(), sizeof(FILE_BASIC_INFO))) {
					if (!m_FILE)
						CloseHandle(h_file);

					DWORD last_error = GetLastError();

					std::wostringstream os;
					os << "Error when setting file attributes : file name: " << path() << " Error Code: "
						<< std::showbase << std::setfill(_T('0')) << std::setw(4) << std::hex << last_error;

					throw std::runtime_error(wstring_to_string(os.str()));
				}
				if (!m_FILE)
					CloseHandle(h_file);
			} else {
				DWORD last_error = GetLastError();

				std::wostringstream os;
				os << "Error when setting file attributes : file name: " << path() << " INVALID_HANDLE_VALUE :  System Error Code: "
					<< std::showbase << std::setfill(_T('0')) << std::setw(4) << std::hex << last_error;

				throw std::runtime_error(wstring_to_string(os.str()));
			}
		}

		// Is the file a directory?
		// Returns: bool: true = yes, false = no
		// Throws std::exception in case of serious issues.
		inline bool is_directory() {
			if (!m_attributes) {
				DWORD res = read_file_attributes();

				if (res) {
					std::wostringstream os;
					os << "Verifying directory failed! : file name: " << path_full()
						<< std::showbase << std::setfill(_T('0')) << std::setw(4) << std::hex << res;
					TRACE(_T("%s\n"), os.str().c_str());
					throw std::runtime_error(wstring_to_string(os.str()));
				}
			}
			return m_attributes->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? true : false;
		}

		// Does the file exist?
		// Returns: DWORD: exists !=0, doesn't exist == INVALID_FILE_ATTRIBUTES
		inline DWORD check_exists()	{
			TRACE(_T("check_exists: %s result: %s\n"), path_full().c_str(), GetFileAttributes(path_full().c_str()) ? _T("true") : _T("false"));
			return GetFileAttributes(path_full().c_str());
		}

		// Is the file a root folder? (like c:)
		// Returns: bool: true = yes, false = no
		inline bool is_root() const {
			return m_is_root.load();
		}

		// Thread safe
		// Stores the CRC32 information in the file
		// Parameters: 
		//    const uint32_t& v: value to be stored
		inline void crc32_ts(const uint32_t& v) {
			m_crc32.store(v);
		}

		// Thread safe
		// Retrieves the CRC32 information stored in the file
		// Returns: bool: true = yes, false = no
		inline uint32_t crc32_ts() const {
			return m_crc32.load();
		}

		inline file_ptr parent() const {
			return m_parent;
		}

		// Thread safe
		// Retrieves the file status information
		// Returns: file_status
		inline file_status status_ts() const {
			return m_status.load();
		}

		// Changes the file name to another one that doesn't exist yet (adds _# before the file extension)
		inline void rename_to_non_existing() {
			if (file_name().size()) {
				std::wstring original_file_name = file_name();

				std::wstring name_without_extension;
				std::wstring extension;
				std::size_t pos = original_file_name.find_last_of(_T('.'));
				if (pos != std::string::npos) {
					name_without_extension = original_file_name.substr(0, pos);
					extension = original_file_name.substr(pos);
				}

				unsigned int index = 0;
				do {
					file_name(name_without_extension.size() ? name_without_extension + _T("_") + std::to_wstring(++index) + extension :
						original_file_name + _T("_") + std::to_wstring(++index));
				} while (check_exists() != INVALID_FILE_ATTRIBUTES);
			} else {
				unsigned int index = 0;
				std::wstring original_folder = folder();
				do {
					folder(original_folder + _T("_") + std::to_wstring(++index));
				} while (check_exists() != INVALID_FILE_ATTRIBUTES);
			}
		}

	protected:

		// Thread safe
		// Sets the file status information
		inline void status_ts(const file_status& v) {
			m_status.store(v);
		}

		// Populates file attributes into the object. Necessary before file_attributes
		// Returns DWORD: Success = 0 Error = Value of GetLastError()
		inline DWORD read_file_attributes() {
			TRACE(_T("Loading attributes for: %s\n"), path_full().c_str());
			m_attributes.reset(new WIN32_FILE_ATTRIBUTE_DATA);
			bool test = m_is_root.load();
			if (!GetFileAttributesExW(m_is_root.load() ? (path_full() + L"\\").c_str() : path_full().c_str(), GetFileExInfoStandard, m_attributes.get()))
				return GetLastError();
			return 0;
		}

		inline HANDLE preallocate() {
			assert(!m_is_root.load());
			HANDLE h_file = CreateFileW(path_full().c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
			if (h_file != INVALID_HANDLE_VALUE) {
				LARGE_INTEGER _size;
				_size.QuadPart = size_ts();
				::SetFilePointerEx(h_file, _size, 0, FILE_BEGIN);
				::SetEndOfFile(h_file);
			}
			return h_file;
		}

	protected:

		FILE_ptr m_FILE;
		win32_attributes_ptr m_attributes;

		file_ptr m_parent;

		std::wstring m_folder;
		std::wstring m_file_name;

		std::atomic<bool> m_failed{ false };
		std::atomic<bool> m_is_root{ false };

		std::atomic<file_status> m_status{ file_status::idle };
		std::atomic<exist_decision> m_exist_decision{ exist_decision::ask };

		std::atomic<uint64_t> m_size{ _UI64_MAX }; // used for folders

		bool m_read{ false };
		bool m_no_write_syscache{ false };

		std::atomic<uint32_t> m_crc32{ 0U };
	};
}