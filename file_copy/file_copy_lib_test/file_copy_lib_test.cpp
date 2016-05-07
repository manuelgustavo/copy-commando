// file_copy_lib_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <vector>
#include <iostream>
#include <memory>

#include "copy_engine.h"

using namespace std;
using namespace file_copy;

class scoped_stdout_mode {
public:
	scoped_stdout_mode(const int& mode) {
		wcout.flush();
		m_prev_mode = _setmode(_fileno(stdout), _O_U8TEXT);
	}

	~scoped_stdout_mode() {
		wcout.flush();
		_setmode(_fileno(stdout), m_prev_mode);
	}

protected:
	int m_prev_mode;
};


void dump_files_to_process(const copy_engine& _copy) {
	//auto prev_mode = _setmode(_fileno(stdout), _O_U8TEXT);
	wcout << _T("\n\n### Dumping files to process STARTED ###\n\n");
	const file_to_process_vector& v = _copy.get_files_to_process();
	wcout << _T("count\tsource()->root()\tsource()->file_name()\tsource()->is_directory()\tsource()->size_ts()\tsource()->path()\tdest()->path()\tget_status_ts()\n");
	uint64_t count = 0ui64;
	for (auto x : v) {
		++count;
		wcout
			<< count << _T("\t")
			<< x.source()->root() << _T("\t")
			<< x.source()->file_name() << _T("\t")
			<< (x.source()->is_directory() ? _T("dir") : _T("file")) << _T("\t")
			<< x.source()->size_ts() << _T("\t")
			<< x.source()->path() << _T("\t")
			<< x.dest()->path() << _T("\t");

		/*idle,
		source_access_denied,
		dest_access_denied,
		started,
		finished,
		source_file_error,
		dest_file_error,*/

		switch (x.get_status_ts()) {
		case files_to_process::files_to_process_status::idle:
			wcout << _T("files_to_process::files_to_process_status::idle");
			break;
		case files_to_process::files_to_process_status::source_access_denied:
			wcout << _T("files_to_process::files_to_process_status::source_access_denied");
			break;
		case files_to_process::files_to_process_status::dest_access_denied:
			wcout << _T("files_to_process::files_to_process_status::dest_access_denied");
			break;
		case files_to_process::files_to_process_status::started:
			wcout << _T("files_to_process::files_to_process_status::started");
			break;
		case files_to_process::files_to_process_status::finished:
			wcout << _T("files_to_process::files_to_process_status::finished");
			break;
		case files_to_process::files_to_process_status::source_file_error:
			wcout << _T("files_to_process::files_to_process_status::source_file_error");
			break;
		case files_to_process::files_to_process_status::dest_file_error:
			wcout << _T("files_to_process::files_to_process_status::dest_file_error");
			break;
		default:
			wcout << _T("unknown");
			break;
		}
		wcout << endl;
	}

	//_setmode(_fileno(stdout), prev_mode);

	wcout << _T("\n\n### Dumping files to process FINISHED ###\n\n");
}

void dump_files_to_process_folders_only(const copy_engine& _copy) {
	//auto prev_mode = _setmode(_fileno(stdout), _O_U8TEXT);
	wcout << _T("\n\n### Dumping folders to process STARTED ###\n\n");
	const file_to_process_vector& v = _copy.get_files_to_process();
	wcout << _T("count\tsource()->root()\tsource()->file_name()\tsource()->is_directory()\tsource()->size_ts()\tsource()->path()\tdest()->path()\tget_status_ts()\n");
	uint64_t count = 0ui64;
	for (auto x : v) {
		if (!x.source()->is_directory())
			continue;
		++count;
		wcout
			<< count << _T("\t")
			<< x.source()->root() << _T("\t")
			<< x.source()->file_name() << _T("\t")
			<< (x.source()->is_directory() ? _T("dir") : _T("file")) << _T("\t")
			<< x.source()->size_ts() << _T("\t")
			<< x.source()->path() << _T("\t")
			<< x.dest()->path() << _T("\t");

		/*idle,
		source_access_denied,
		dest_access_denied,
		started,
		finished,
		source_file_error,
		dest_file_error,*/

		switch (x.get_status_ts()) {
		case files_to_process::files_to_process_status::idle:
			wcout << _T("files_to_process::files_to_process_status::idle");
			break;
		case files_to_process::files_to_process_status::source_access_denied:
			wcout << _T("files_to_process::files_to_process_status::source_access_denied");
			break;
		case files_to_process::files_to_process_status::dest_access_denied:
			wcout << _T("files_to_process::files_to_process_status::dest_access_denied");
			break;
		case files_to_process::files_to_process_status::started:
			wcout << _T("files_to_process::files_to_process_status::started");
			break;
		case files_to_process::files_to_process_status::finished:
			wcout << _T("files_to_process::files_to_process_status::finished");
			break;
		case files_to_process::files_to_process_status::source_file_error:
			wcout << _T("files_to_process::files_to_process_status::source_file_error");
			break;
		case files_to_process::files_to_process_status::dest_file_error:
			wcout << _T("files_to_process::files_to_process_status::dest_file_error");
			break;
		default:
			wcout << _T("unknown");
			break;
		}
		wcout << endl;
	}

	//_setmode(_fileno(stdout), prev_mode);

	wcout << _T("\n\n### Dumping folders to process FINISHED ###\n\n");
}

void tester(const wstring& s, const wstring& d, bool just_prepare = false, bool dump_prepare = false, copy_engine::async_mode mode = copy_engine::async_mode::automatic) {
	wcout << _T("\n\n### Copying files STARTED ###\n\n");
	wcout << _T("source: ") << s << endl << _T("dest: ") << d << endl;
	auto start = std::chrono::steady_clock::now();
	copy_engine& _copy = copy_engine::get_instance();
	_copy.init();
	auto start_prepare = std::chrono::steady_clock::now();

	_copy.copy_prepare(s, d);

	auto end_prepare = std::chrono::steady_clock::now();
	auto duration_prepare = std::chrono::duration_cast<std::chrono::milliseconds>(end_prepare - start_prepare);
	wcout << _T("listing files/folders took: ") << duration_prepare.count() << " milliseconds.\n";
	wcout << _T("files: ") << _copy.num_files_to_process_ts() << endl;
	wcout << _T("folders: ") << _copy.num_folders_to_process_ts() << endl;
	wcout << _T("size: ") << _copy.files_to_process_total_size_bytes_ts() << endl;
	wcout << _T("async decision: ") << (_copy.async() ? _T("asynchronous") : _T("synchronous")) << endl;
	if (mode != copy_engine::async_mode::automatic)
		wcout << _T("overwriting decision with async_mode")
		<< (mode == copy_engine::async_mode::sync ? _T("copy_engine::async_mode::sync") : _T("copy_engine::async_mode::async"))
		<< endl;

	if (dump_prepare)
		dump_files_to_process_folders_only(_copy);

	if (dump_prepare)
		dump_files_to_process(_copy);

	if (!just_prepare) {
		auto start_copy = std::chrono::steady_clock::now();
		_copy.copy_start(mode);
		auto end_copy = std::chrono::steady_clock::now();;
		auto duration_copy(std::chrono::duration_cast<std::chrono::milliseconds>(end_copy - start_copy));
		wcout << _T("copying files took: ") << duration_copy.count() << _T(" milliseconds.\n");
	} else {
		wcout << _T("skipping copy step!") << endl;
	}
	auto end = std::chrono::steady_clock::now();
	auto duration(std::chrono::duration_cast<std::chrono::milliseconds>(end - start));
	wcout << _T("whole process took: ") << duration.count() << _T(" milliseconds.\n");
	wcout << _T("\n\n### Copying files ENDED ###\n\n");
}



int main()
{
	scoped_stdout_mode stdout_mode{ _O_U8TEXT };
	try {
		/*wcout << _T("testing assynchronous\n");
		tester(_T("f:\\t1\\filecopy"), _T("f:\\t1"), false, false, copy_engine::async_mode::async);*/

		wcout << _T("testing auto async\n");
		tester(_T("f:\\t1\\filecopy"), _T("h:\\t1"), false, false);

		/*wcout << _T("testing synchronous\n");
		tester(_T("e:\\t1\\filecopy"), _T("e:\\t1"), false, false, copy_engine::async_mode::sync);*/

		

	} catch (exception& e) {
		TRACE("exception in filecopy: %s\n", e.what());
		wcout << "exception in filecopy: " << e.what();
		assert(0);
	}

	return 0;
}

