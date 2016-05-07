#include "stdafx.h"
#include "MainDlg.h"
#include "file.h"

LRESULT CMainDlg::OnInitDialog(HWND /**/, LPARAM /*lParam*/) {
	// initialize controls
	m_progress_bar_current_read.SubclassWindow(GetDlgItem(IDC_PROGRESS_CURRENT_READ));
	m_static_current_read.Attach(GetDlgItem(IDC_STATIC_CURRENT_READ));
	m_static_current_read.SetWindowText(_T(""));

	m_progress_bar_current_write.SubclassWindow(GetDlgItem(IDC_PROGRESS_CURRENT_WRITE));
	m_static_current_write.Attach(GetDlgItem(IDC_STATIC_CURRENT_WRITE));
	m_static_current_write.SetWindowText(_T(""));

	m_progress_bar_total_read.SubclassWindow(GetDlgItem(IDC_PROGRESS_TOTAL_READ));
	m_static_total_read.Attach(GetDlgItem(IDC_STATIC_TOTAL_READ));
	m_static_total_read.SetWindowText(_T(""));

	m_progress_bar_total_write.SubclassWindow(GetDlgItem(IDC_PROGRESS_TOTAL_WRITE));
	m_static_total_write.Attach(GetDlgItem(IDC_STATIC_TOTAL_WRITE));
	m_static_total_write.SetWindowText(_T(""));

	m_mode.Attach(GetDlgItem(IDC_MODE));
	// /initialize controls



	//m_progress_bar.SetScrollRange(0, 100);
	//m_progress_bar.SetScrollPos(50);
	//m_progress_bar.SetRange(0, 100);
	//m_progress_bar.SetPos(50);
	//m_progress_bar.SetWindowText(_T("teste 123456"));
	//m_progress_bar.ModifyStyle(0, PBS_MARQUEE | WS_CAPTION);
	//m_progress_bar.SetWindowText(_T("teste 123456"));
	//m_progress_bar.SetMarqueeOptions(30);
	//m_progress_bar.SetMarquee(true, 30);
	//SetDlgItemTextW(IDC_PROGRESS,_T("test"));
	//m_progress_bar.set
	//m_progress_bar.Invalidate();
	//m_progress_bar.SetPos(60);
	// sets the statistict timer
	SetTimer(MAINDLG_TIMER_ID, 30);

	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	// remove
	/*file_copy::copy_engine _copy{ 3000 };
	_copy.copy_prepare(_T("e:\\t1\\filecopy"), _T("e:\\t1"));
	_copy.copy_start();*/
	m_file_copy_thread.add(_T("c:\\a"), _T("c:\\b"));
	m_file_copy_thread.run();

	//m_file_copy_thread

	// /remove
	return TRUE;
}

void CMainDlg::OnTimer(UINT_PTR uTimerID) {
	if (MAINDLG_TIMER_ID != uTimerID)
		SetMsgHandled(false);
	else
		update_stats();
	//RedrawWindow();
}

void CMainDlg::OnDestroy() {
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);
}

void CMainDlg::update_stats() {
	static bool first_run = true;

	std::wostringstream os;
	auto status = m_file_copy_thread.status();
	os << _T(APP_TITLE " - ") << status_to_text(status)
		<< _T(" - ") << m_file_copy_thread.num_to_process() << _T(" items");
	SetWindowText(os.str().c_str());

	auto mode = m_file_copy_thread.async();
	static auto prev_mode = false;

	if ((prev_mode != mode) || first_run) {
		m_mode.SetWindowText(m_file_copy_thread.async() ? _T("Mode: asynchronous") : _T("Mode: synchronous"));
		prev_mode = mode;
	}

	//m_mode.Invalidate();
	static auto prev_status = file_copy_thread::file_copy_status::undefined;

	if ((status != prev_status) || (status == file_copy_thread::file_copy_status::copying)) {
		prev_status = status;
		switch (status) {
		case file_copy_thread::file_copy_status::idle:
			//m_progress_bar.SetMarquee(false, 0);
			break;
		case file_copy_thread::file_copy_status::preparing:
			m_progress_bar_current_read.SetMarquee(true, 30);
			m_progress_bar_current_read.SetWindowText(_T("Preparing..."));
			m_progress_bar_current_write.SetMarquee(true, 30);
			m_progress_bar_current_write.SetWindowText(_T("Preparing..."));
			m_progress_bar_total_read.SetMarquee(true, 30);
			m_progress_bar_total_read.SetWindowText(_T("Preparing..."));
			m_progress_bar_total_write.SetMarquee(true, 30);
			m_progress_bar_total_write.SetWindowText(_T("Preparing..."));
			break;
		case file_copy_thread::file_copy_status::copying: {
			file_copy::file_ptr file_read = m_file_copy_thread.current_read_file();
			if (file_read) {
				m_static_current_read.SetWindowText(file_read->file_name().c_str());
			}

			file_copy::file_ptr file_write = m_file_copy_thread.current_write_file();
			if (file_write) {
				m_static_current_write.SetWindowText(file_write->file_name().c_str());
			}
		} break;
/*			m_progress_bar.SetMarquee(false, 0);
			
			m_progress_bar_total.SetMarquee(false, 0);*/
		case file_copy_thread::file_copy_status::completed:
			m_progress_bar_current_read.SetBarColor(BAR1_COLOR);
			m_progress_bar_current_read.SetMarquee(false, 0);
			m_progress_bar_current_write.SetBarColor(BAR1_COLOR);
			m_progress_bar_current_write.SetMarquee(false, 0);
			m_progress_bar_total_read.SetBarColor(BAR1_COLOR);
			m_progress_bar_total_read.SetMarquee(false, 0);
			m_progress_bar_total_write.SetBarColor(BAR1_COLOR);
			m_progress_bar_total_write.SetMarquee(false, 0);
			break;
		case file_copy_thread::file_copy_status::error:
			break;
		}
	}
	first_run = false;
}