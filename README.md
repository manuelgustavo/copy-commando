# copy-commando

Manuel Saraiva (2016.05.07):
TeraCopy is a great tool (http://codesector.com/teracopy), the problem is that it lacks performance , the release is buggy (crashes when trying to copy >100K files or so, and doesn't copy anything with a path longer than 256 characters) and the last release was years ago.
Besides that, it's a paid tool.

In order to solve these problems, I've decided to create this alternative (in c++ with WTL), which I've made open-source due to lack for continuing on it.

The application features synchronous and asynchronous copy decision based on the source and destination;
Attributes preservation;
CRC32 checking.

To compile use Visual Studio 2015.

Pre-requisites: WTL9 (https://sourceforge.net/p/wtl/discussion/374433/thread/152f76ef/)


The WTL 9 include directory must be set in the Properties/VC++ Directories if it's different than "C:\WTL\WTL9\Include"

There are 3 projects inside the solution:
file_copy_lib: Library which contains all the code behind the scenes. The idea is that it could be used for other projects as well.
file_copy_lib_test: Unit testing of the library.
file_copy_dlg: Main dialog for the application.

Beware that this is an incomplete code (early alpha), optimizations weren't applied yet.

** TESTING the existing code **
using file_copy_lib_test:
In file_copy_lib_test.cpp, change the following line to the source and destination folders as needed:
tester(_T("c:\\a"), _T("c:\\b"), false, false);


using file_copy_dlg:
In MainDlg.cpp, change the following line to the source and destination folders as needed:
m_file_copy_thread.add(_T("c:\\a"), _T("c:\\b"));

Note that you need access rights to the folders being used.