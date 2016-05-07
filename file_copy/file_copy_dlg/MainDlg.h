// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////
#pragma once

#include <atlctrls.h>
#include <atlcrack.h>
#include <atlddx.h>
#include "resource.h"
#include "aboutdlg.h"

#include "copy_engine.h"
#include "file_copy_thread.h"
//#include "CProgressBarCtrlEx.h"
#include "TextProgressCtrlEx.h"


#define APP_TITLE "Copy Commando"
#define BAR1_COLOR RGB(0, 255, 0)
#define MAINDLG_TIMER_ID 2

class CMainDlg : public CDialogImpl<CMainDlg>, public CUpdateUI<CMainDlg>,// public CWinDataExchange<CMainDlg>,
	public CMessageFilter, public CIdleHandler {
public:
	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

	virtual BOOL OnIdle() {
		UIUpdateChildWindows();
		return FALSE;
	}

	/*BEGIN_DDX_MAP(CMainDlg)
		DDX_CONTROL(IDC_MODE, m_mode)
	END_DDX_MAP()*/

	BEGIN_UPDATE_UI_MAP(CMainDlg)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP_EX(CMainDlg)
		//MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_TIMER(OnTimer)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		//CHAIN_MSG_MAP_MEMBER(m_progress_bar)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(HWND /**/, LPARAM /*lParam*/);

	void OnTimer(UINT_PTR uTimerID);

	void OnDestroy();

	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		CAboutDlg dlg;
		dlg.DoModal();
		return 0;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		// TODO: Add validation code 
		CloseDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		CloseDialog(wID);
		return 0;
	}

	void CloseDialog(int nVal) {
		DestroyWindow();
		::PostQuitMessage(nVal);
	}

public:
	CStatic m_static_current_read;
	CTextProgressCtrlEx m_progress_bar_current_read;
	CStatic m_static_total_read;
	CTextProgressCtrlEx m_progress_bar_total_read;
	CStatic m_static_current_write;
	CTextProgressCtrlEx m_progress_bar_current_write;
	CStatic m_static_total_write;
	CTextProgressCtrlEx m_progress_bar_total_write;
	CStatic m_mode;

protected:
	std::wstring status_to_text(const file_copy_thread::file_copy_status& v) const {
		switch (v) {
		case file_copy_thread::file_copy_status::idle:
			return _T("idle");
		case file_copy_thread::file_copy_status::preparing:
			return _T("counting");
		case file_copy_thread::file_copy_status::copying:
			return _T("processing");
		case file_copy_thread::file_copy_status::completed:
			return _T("completed");
		case file_copy_thread::file_copy_status::error:
			return _T("error");
		default:
			assert(0);
			return _T("unexpected");
		}
	}

	void update_stats();

protected:

	file_copy_thread m_file_copy_thread;
};
