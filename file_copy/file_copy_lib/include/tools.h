#pragma once

#include <Windows.h>
#include <WinBase.h>

#include <stdio.h>
#include <string>
#include <algorithm>
#include <cassert>
#include <memory>
#include <fileapi.h>
#include <vector>
#include "trace.h"
#include <tchar.h>

//#include <winioctl.h>



namespace file_copy {
	//const int READ_SIZE = 65536;
	//constexpr int READ_SIZE = 65536;
	using win32_attributes_ptr = std::shared_ptr<WIN32_FILE_ATTRIBUTE_DATA>;
	using file_basic_info_ptr = std::shared_ptr<FILE_BASIC_INFO>;

	// converts a std::string into a std::wstring
	// Parameters:
	//    const std::string& s: [in] string to be converted
	// Returns: std::wstring: value converted into std::wstring
	/*inline std::wstring string_to_wstring(const std::string& s) {
		std::wstring ws(s.begin(), s.end());
		//std::copy(s.begin(), s.end(), ws.begin());
		return ws;
	}*/
	inline std::string wstring_to_string(const std::wstring& ws) {
		std::unique_ptr<char> buff(new char[ws.size() * 2]);

		//char t1[10];
		//wchar_t t2[10];
		//wcstombs_s(t1, ws.size() * 2,t2, ws.size() * 2);
		size_t num_converted;
		wcstombs_s(&num_converted, buff.get(), ws.size() * 2, ws.c_str(), ws.size() * 2);

		return std::string{ buff.get() };
	}

	inline std::wstring string_to_wstring(const std::string& s) { // TODO: NEEDS TO BE CHANGED AS THE ONE ABOVE
		std::wstring ws(s.begin(), s.end());
		//std::copy(s.begin(), s.end(), ws.begin());
		return ws;
	}

	// creates a dirtectory, regardless if it's recursive or not
	inline bool create_dir(const std::wstring& dir) {
		if (dir.size() < 7)
			return false;
		if (!CreateDirectory(dir.c_str(), nullptr)) {
			auto err = GetLastError();
			if (err != ERROR_ALREADY_EXISTS) {
				std::size_t pos = dir.find_last_of(_T('\\'));

				std::wstring upper_dir = dir.substr(0, pos);
				bool res = create_dir(upper_dir);
				return res && CreateDirectory(dir.c_str(), nullptr);
			} else {
				return true;
			}
		} else {
			return true;
		}
	}

	//using volume_disk_extents_ptr = std::shared_ptr<VOLUME_DISK_EXTENTS>;
	// Get the Disk Extents (used for physical disk id)
	// Parameters:
	//    const std::wstring& root: [in] root. (eg _T("E:"))
	//    VOLUME_DISK_EXTENTS& extents: [out] VOLUME_DISK_EXTENTS to hold the result
	// Returns: DWORD: success = 0, otherwise the value of GetLastError
	inline DWORD get_disk_extents(const std::wstring& root, VOLUME_DISK_EXTENTS& extents) {
		DWORD ret = 0;
		VOLUME_DISK_EXTENTS extents_out;
		//TRACE(_T("Setting file basic info (times and basic attributes) for: %s\n : is_directory \"%s\""), path_full().c_str(), is_directory() ? _T("true") : _T("false"));
		HANDLE h_file = INVALID_HANDLE_VALUE;
		//std::wstring remove = source->root_full();
		h_file = ::CreateFileW(root.c_str(), 0,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);

		if (h_file != INVALID_HANDLE_VALUE) {
			DWORD dwSize;
			if (!::DeviceIoControl(
				h_file,
				IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
				NULL,
				0,
				&extents_out,
				static_cast<DWORD>(sizeof(VOLUME_DISK_EXTENTS)),
				&dwSize,
				NULL)) {
				ret = ::GetLastError();
				if (ret == ERROR_MORE_DATA) {
					DWORD size = static_cast<DWORD>(offsetof(VOLUME_DISK_EXTENTS, Extents[extents_out.NumberOfDiskExtents]));

					if (!::DeviceIoControl(h_file, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0,
						(void*)&extents_out, size, &dwSize, NULL)) {
						ret = ::GetLastError();
					} else {
						ret = 0;
					}
				}
			}
			::CloseHandle(h_file);
		}

		if (!ret) {
			memcpy_s(&extents, sizeof(VOLUME_DISK_EXTENTS), &extents_out, sizeof(VOLUME_DISK_EXTENTS));
		}
		return ret;
	}

	// Get the Disk Extents (used for physical disk id)
	// Parameters:
	//    const std::wstring& root: [in] root. (eg _T("E:\"))
	//    int64_t& space: [out] place where to store the disk space.
	// Returns: DWORD: success = 0, otherwise the value of GetLastError
	inline DWORD get_disk_free_space(const std::wstring& root, uint64_t& space) {

		DWORD ret = 0;
		ULARGE_INTEGER free_bytes_to_caller;
		
		if (!GetDiskFreeSpaceExW(root.c_str(), &free_bytes_to_caller, NULL, NULL)) {
			ret = GetLastError();
		} else {
			space = free_bytes_to_caller.QuadPart;
		}

		return ret;
	}

	// converts a std::wstring into a std::string
	// Parameters:
	//    const std::string& s: [in] wstring to be converted
	// Returns: std::wstring: value converted into std::string
	/*inline std::string wstring_to_string(const std::wstring& ws) {
		//std::string s(ws.length(), ' ');
		//std::copy(ws.begin(), ws.end(), s.begin());
		std::string s(ws.begin(), ws.end());
		return s;
	}*/


	constexpr int READ_SIZE = 32768;
	//constexpr int READ_SIZE = 131072;
	//constexpr int READ_SIZE = 16384;
	inline std::wstring get_errno_desc(const errno_t& error) {

		switch (error) {
		case 0:
			return _T("SUCCESS");
		case EPERM:
			return _T("Operation not permitted");
		case ENOENT:
			return _T("No such file_part_task or directory");
		case ESRCH:
			return _T("No such process");
		case EINTR:
			return _T("Interrupted function");
		case EIO:
			return _T("I/O error");
		case ENXIO:
			return _T("No such device or address");
		case E2BIG:
			return _T("Argument list too long");
		case ENOEXEC:
			return _T("Exec format error");
		case EBADF:
			return _T("Bad file_part_task number");
		case ECHILD:
			return _T("No spawned processes");
		case EAGAIN:
			return _T("No more processes or not enough memory or maximum nesting level reached");
		case ENOMEM:
			return _T("Not enough memory");
		case EACCES:
			return _T("Permission denied");
		case EFAULT:
			return _T("Bad address");
		case EBUSY:
			return _T("Device or resource busy");
		case EEXIST:
			return _T("file_part_task exists");
		case EXDEV:
			return _T("Cross-device link");
		case ENODEV:
			return _T("No such device");
		case ENOTDIR:
			return _T("Not a directory");
		case EISDIR:
			return _T("Is a directory");
		case EINVAL:
			return _T("Invalid argument");
		case ENFILE:
			return _T("Too many files open in system");
		case EMFILE:
			return _T("Too many open files");
		case ENOTTY:
			return _T("Inappropriate I/O control operation");
		case EFBIG:
			return _T("file_part_task too large");
		case ENOSPC:
			return _T("No space left on device");
		case ESPIPE:
			return _T("Invalid seek");
		case EROFS:
			return _T("Read-only file_part_task system");
		case EMLINK:
			return _T("Too many links");
		case EPIPE:
			return _T("Broken pipe");
		case EDOM:
			return _T("Math argument");
		case ERANGE:
			return _T("Result too large");
		case EDEADLK:
			return _T("Resource deadlock would occur");
		case ENAMETOOLONG:
			return _T("Filename too long");
		case ENOLCK:
			return _T("No locks available");
		case ENOSYS:
			return _T("Function not supported");
		case ENOTEMPTY:
			return _T("Directory not empty");
		case EILSEQ:
			return _T("Directory not empty");
		case STRUNCATE:
			return _T("String was truncated");
		default:
			assert(0);
			return _T("Unknown");
		}

	}

	// will notify an event once when notify() is called and when the class is destroyed.
	/*class event_notifier {
	public:
		event_notifier(std::mutex* pMutex, std::condition_variable* cv) : m_p_mutex{ pMutex }, m_p_cv{ cv } {
			assert(pMutex && cv);
		}

		~event_notifier() {
			notify();
		}

		void notify() {
			if (!m_notified && m_p_mutex && m_p_cv) {
				{
					std::lock_guard<std::mutex> lk(*m_p_mutex); // guarantees the requestor will be waiting for a response
					m_notified = true;
				}
				m_p_cv->notify_all(); // notify request
			}
		}

		void reset() {
			std::lock_guard<std::mutex> lk(*m_p_mutex);
			m_notified = false;
		}

	private:
		std::mutex* m_p_mutex;
		std::condition_variable* m_p_cv;
		bool m_notified{ false };
	};*/
}