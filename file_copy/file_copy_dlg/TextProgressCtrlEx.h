#pragma once


//
// TextProgressCtrl.h : header file for CTextProgressCtrlEx class
//
// Written by Chris Maunder (chris@codeproject.com)
// Copyright 1998-2005.
//
// TextProgressCtrl is a drop-in replacement for the standard
// CProgressCtrl that displays text in a progress control.
//
// Homepage: http://www.codeproject.com/miscctrl/text_progressctrl.asp
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed by any means PROVIDING it is not sold for
// profit without the author's written consent, and providing that this
// notice and the author's name is included. If the source code in
// this file is used in any commercial application then an email to
// me would be nice.
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to your
// computer, causes your pet cat to fall ill, increases baldness or
// makes you car start emitting strange noises when you start it up.
//
// Expect bugs.
//
// Please use and enjoy. Please let me know of any bugs/mods/improvements
// that you have found/implemented and I will fix/incorporate them into this
// file.
//
// See file TextProgressCtrl.cpp for modification history.
//


////////////////////////////////////////////////////////////////////////////////
//
// To set the range (16 bit):
//		call	SetRange(WORD wLower, WORD wUpper)
//		or		SendMessage(PBM_SETRANGE, 0, MAKELPARAM(wLower, wUpper))
//		or		::SendMessage(m_hWnd, PBM_SETRANGE, 0, MAKELPARAM(wLower, wUpper))
//
// To set the position:
//		call	SetPos(int nNewPos)
//		or		SendMessage(PBM_SETPOS, (WPARAM)nNewPos, 0)
//		or		::SendMessage(m_hWnd, PBM_SETPOS, (WPARAM)nNewPos, 0)
//
// To set the position using a delta value:
//		call	OffsetPos(int nIncrement)
//		or		SendMessage(PBM_DELTAPOS, (WPARAM)nIncrement, 0)
//		or		::SendMessage(m_hWnd, PBM_DELTAPOS, (WPARAM)nIncrement, 0)
//
// To set the step increment size:
//		call	SetStep(int nStepInc)
//		or		SendMessage(PBM_SETSTEP, (WPARAM)nStepInc, 0)
//		or		::SendMessage(m_hWnd, PBM_SETSTEP, (WPARAM)nStepInc, 0)
//
// To step the position by the step increment size:
//		call	StepIt()
//		or		SendMessage(PBM_STEPIT, 0, 0)
//		or		::SendMessage(m_hWnd, PBM_STEPIT, 0, 0)
//
// To set the range (32 bit):
//		call	SetRange32(int nLowLim, int nHighLim)
//		or		SendMessage(PBM_SETRANGE32, (WPARAM)nLowLim, (LPARAM)nHighLim)
//		or		::SendMessage(m_hWnd, PBM_SETRANGE32, (WPARAM)nLowLim, (LPARAM)nHighLim)
//
// To get the current range:
//		call	GetRange(BOOL bWhichLimit, PBRANGE* pPBRange)
//		or		SendMessage(PBM_GETRANGE, (WPARAM)bWhichLimit, (LPARAM)pBBRange)
//		or		::SendMessage(m_hWnd, PBM_GETRANGE, (WPARAM)bWhichLimit, (LPARAM)pBBRange)
//
// To get the current position:
//		call	GetPos()
//		or		SendMessage(PBM_GETPOS, 0, 0)
//		or		::SendMessage(m_hWnd, PBM_GETPOS, 0, 0)
//
// To set the bar color:
//		call	SetBarColor(COLORREF crBar)
//		or		SendMessage (PBM_SETBARCOLOR, 0, (LPARAM)crBar)
//		or		::SendMessage (m_hwnd, PBM_SETBARCOLOR, 0, (LPARAM)crBar)
//
// To get the bar color:
//		call	GetBarColor()
//		or		SendMessage (PBM_GETBARCOLOR, 0, 0)
//		or		::SendMessage (m_hwnd, PBM_GETBARCOLOR, 0, 0)
//
// To set the bar background color:
//		call	SetBkColor(COLORREF crBarBk)
//		or		SetBarBkColor(COLORREF crBarBk)
//		or		SendMessage (PBM_SETBARBKCOLOR, 0, (LPARAM)crBarBk)
//		or		::SendMessage (m_hwnd, PBM_SETBARBKCOLOR, 0, (LPARAM)crBarBk)
//
// To get the bar background color:
//		call	GetBarBkColor()
//		or		SendMessage (PBM_GETBARBKCOLOR, 0, 0)
//		or		::SendMessage (m_hwnd, PBM_GETBARBKCOLOR, 0, 0)
//
// To operate on the bar text:
//		use any of the normal CWnd window text functions, such as
//		SetWindowText, GetWindowText, GetWindowTextLength, SetFont and GetFont
//
// To set the text color:
//		call	SetTextColor(COLORREF crTextClr)
//		or		SendMessage (PBM_SETTEXTCOLOR, 0, (LPARAM)crTextClr)
//		or		::SendMessage (m_hwnd, PBM_SETTEXTCOLOR, 0, (LPARAM)crTextClr)

// To get the text color:
//		call	GetTextColor()
//		or		SendMessage (PBM_GETTEXTCOLOR, 0, 0)
//		or		::SendMessage (m_hwnd, PBM_GETTEXTCOLOR, 0, 0)
//
// To set the text background color:
//		call	SetTextBkColor(COLORREF crTextBkClr)
//		or		SendMessage (PBM_SETTEXTBKCOLOR, 0, (LPARAM)crTextBkClr)
//		or		::SendMessage (m_hwnd, PBM_SETTEXTBKCOLOR, 0, (LPARAM)crTextBkClr)
//
// To get the text background color:
//		call	GetTextBkColor()
//		or		SendMessage (PBM_GETTEXTBKCOLOR, 0, 0)
//		or		::SendMessage (m_hwnd, PBM_GETTEXTBKCOLOR, 0, 0)
//
// To show or hide percentage complete:
//		call	SetShowPercent(BOOL bShow)
//		or		SendMessage(PBM_SETSHOWPERCENT, (WPARAM)bShow, 0)
//		or		::SendMessage(m_hWnd, PBM_SETSHOWPERCENT, (WPARAM)bShow, 0)
//
// To set the text alignment mode:
//		call	AlignText(DWORD dwAlignment), where dwAlignment = DT_LEFT, DT_RIGHT or DT_CENTER
//		or		SendMessage(PBM_ALIGNTEXT, (WPARAM)dwAlignment, 0)
//		or		SendMessage(m_hWnd, PBM_ALIGNTEXT, (WPARAM)dwAlignment, 0)
//
// To turn on/off marquee mode
//		call	SetMarquee(BOOL bOn, int nMsecBetweenUpdate)
//		or		SendMessage(PBM_SETMARQUEE, (WPARAM)bOn, (LPARAM)nMsecBetweenUpdate)
//		or		::SendMessage(m_hWnd, PBM_SETMARQUEE, (WPARAM)bOn, (LPARAM)nMsecBetweenUpdate)
//
// To set marquee options
//		call	SetMarqueeOptions(int nBarSize)
//		or		SendMessage(PBM_SETMARQUEEOPTIONS, (WPARAM)nBarSize, 0)
//		or		::SendMessage(m_hWnd, PBM_SETMARQUEEOPTIONS, (WPARAM)nBarSize, 0)
//
////////////////////////////////////////////////////////////////////////////////

#include <atlcrack.h>
#include <atlctrls.h>
#include <atlgdi.h>
#include <Windows.h>
#include <WinBase.h>
#include <atlframe.h>


// setup aliases using "Colour" instead of "Color"
#define SetBarColour		SetBarColor
#define GetBarColour		GetBarColor
#define SetBgColour			SetBarBkColor
#define GetBgColour			GetBarBkColor
#define SetTextColour		SetTextColor
#define GetTextColour		GetTextColor

// define progress bar stuff that may not be elsewhere defined (if needed)
#ifndef PBS_SMOOTH
#define PBS_SMOOTH			0x01
#endif
#ifndef PBS_VERTICAL
#define PBS_VERTICAL		0x04
#endif
#ifndef PBS_MARQUEE
#define PBS_MARQUEE			0x08
#endif
#ifndef PBM_SETRANGE32
#define PBM_SETRANGE32		(WM_USER+6)
typedef struct {
	int iLow;
	int iHigh;
} PBRANGE, *PPBRANGE;
#endif
#ifndef PBM_GETRANGE
#define PBM_GETRANGE		(WM_USER+7)
#endif
#ifndef PBM_GETPOS
#define PBM_GETPOS			(WM_USER+8)
#endif
#ifndef PBM_SETBARCOLOR
#define PBM_SETBARCOLOR		(WM_USER+9)
#endif
#ifndef PBM_SETBKCOLOR
#define PBM_SETBKCOLOR		CCM_SETBKCOLOR
#endif
#ifndef PBM_SETMARQUEE
#define PBM_SETMARQUEE		(WM_USER+10)
#endif

// setup message codes for new messages
// already defined in CommCtrl #define PBM_GETBARCOLOR			(WM_USER+100)
// already defined in CommCtrl #define PBM_GETBKCOLOR			(WM_USER+101)
#define PBM_SETTEXTCOLOR		(WM_USER+102)
#define PBM_GETTEXTCOLOR		(WM_USER+103)
#define PBM_SETTEXTBKCOLOR		(WM_USER+104)
#define PBM_GETTEXTBKCOLOR		(WM_USER+105)
#define PBM_SETSHOWPERCENT		(WM_USER+106)
#define PBM_ALIGNTEXT			(WM_USER+107)
#define PBM_SETMARQUEEOPTIONS	(WM_USER+108)


////////////////////////////////////////////////////////////////////////////////
// CTextProgressCtrlEx class

class CTextProgressCtrlEx : public CWindowImpl<CTextProgressCtrlEx, CProgressBarCtrl>,
							public CUpdateUI<CTextProgressCtrlEx>,
							public CMessageFilter,
							public CIdleHandler {
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

	virtual BOOL OnIdle() {
		UIUpdateChildWindows();
		return FALSE;
	}

	DECLARE_WND_SUPERCLASS(_T("TextProgressCtrlEx"), CProgressBarCtrl::GetWndClassName())

	BEGIN_UPDATE_UI_MAP(CTextProgressCtrlEx)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP_EX(CTextProgressCtrlEx)
		MSG_WM_ERASEBKGND(OnEraseBkgnd)
		MSG_WM_PAINT(OnPaint)
		MSG_WM_TIMER(OnTimer)

		MESSAGE_HANDLER_EX(PBM_SETRANGE, OnSetRange)
		MESSAGE_HANDLER_EX(PBM_SETPOS, OnSetPos)
		MESSAGE_HANDLER_EX(PBM_DELTAPOS, OnOffsetPos)
		MESSAGE_HANDLER_EX(PBM_SETSTEP, OnSetStep)
		MESSAGE_HANDLER_EX(PBM_STEPIT, OnStepIt)
		MESSAGE_HANDLER_EX(PBM_SETRANGE32, OnSetRange32)
		MESSAGE_HANDLER_EX(PBM_GETRANGE, OnGetRange)
		MESSAGE_HANDLER_EX(PBM_GETPOS, OnGetPos)
		MESSAGE_HANDLER_EX(PBM_SETBARCOLOR, OnSetBarColor)
		MESSAGE_HANDLER_EX(PBM_SETBKCOLOR, OnSetBarBkColor)

		MESSAGE_HANDLER_EX(PBM_GETBARCOLOR, OnGetBarColor)
		MESSAGE_HANDLER_EX(PBM_GETBKCOLOR, OnGetBarBkColor)
		MESSAGE_HANDLER_EX(PBM_SETTEXTCOLOR, OnSetTextColor)
		MESSAGE_HANDLER_EX(PBM_GETTEXTCOLOR, OnGetTextColor)
		MESSAGE_HANDLER_EX(PBM_SETTEXTBKCOLOR, OnSetTextBkColor)
		MESSAGE_HANDLER_EX(PBM_GETTEXTBKCOLOR, OnGetTextBkColor)
		MESSAGE_HANDLER_EX(PBM_SETSHOWPERCENT, OnSetShowPercent)
		MESSAGE_HANDLER_EX(PBM_ALIGNTEXT, OnAlignText)
		MESSAGE_HANDLER_EX(PBM_SETMARQUEE, OnSetMarquee)
		MESSAGE_HANDLER_EX(PBM_SETMARQUEEOPTIONS, OnSetMarqueeOptions)
		END_MSG_MAP()

	// Constructor / Destructor
	CTextProgressCtrlEx();

	virtual ~CTextProgressCtrlEx();

	// Operations
public:
	inline COLORREF SetBarColor(COLORREF crBarClr = CLR_DEFAULT) {
		ATLASSERT(::IsWindow(m_hWnd)); return ((COLORREF) ::SendMessage(m_hWnd, PBM_SETBARCOLOR, 0, (LPARAM)crBarClr));
	}
	inline COLORREF GetBarColor() const {
		ATLASSERT(::IsWindow(m_hWnd)); return ((COLORREF) ::SendMessage(m_hWnd, PBM_GETBARCOLOR, 0, 0));
	}
	inline COLORREF SetBarBkColor(COLORREF crBarBkClr = CLR_DEFAULT) {
		ATLASSERT(::IsWindow(m_hWnd)); return ((COLORREF) ::SendMessage(m_hWnd, PBM_SETBKCOLOR, 0, (LPARAM)crBarBkClr));
	}
	inline COLORREF GetBarBkColor() const {
		ATLASSERT(::IsWindow(m_hWnd)); return ((COLORREF) ::SendMessage(m_hWnd, PBM_GETBKCOLOR, 0, 0));
	}
	inline COLORREF SetTextColor(COLORREF crTextClr = CLR_DEFAULT) {
		ATLASSERT(::IsWindow(m_hWnd)); return ((COLORREF) ::SendMessage(m_hWnd, PBM_SETTEXTCOLOR, 0, (LPARAM)crTextClr));
	}
	inline COLORREF GetTextColor() const {
		ATLASSERT(::IsWindow(m_hWnd)); return ((COLORREF) ::SendMessage(m_hWnd, PBM_GETTEXTCOLOR, 0, 0));
	}
	inline COLORREF SetTextBkColor(COLORREF crTextBkClr = CLR_DEFAULT) {
		ATLASSERT(::IsWindow(m_hWnd)); return ((COLORREF) ::SendMessage(m_hWnd, PBM_SETTEXTBKCOLOR, 0, (LPARAM)crTextBkClr));
	}
	inline COLORREF GetTextBkColor() const {
		ATLASSERT(::IsWindow(m_hWnd)); return ((COLORREF) ::SendMessage(m_hWnd, PBM_GETTEXTBKCOLOR, 0, 0));
	}
	inline BOOL SetShowPercent(BOOL bShow) {
		ATLASSERT(::IsWindow(m_hWnd)); return ((BOOL) ::SendMessage(m_hWnd, PBM_SETSHOWPERCENT, (WPARAM)bShow, 0));
	}
	inline DWORD AlignText(DWORD dwAlignment = DT_CENTER) {
		ATLASSERT(::IsWindow(m_hWnd)); return ((DWORD) ::SendMessage(m_hWnd, PBM_ALIGNTEXT, (WPARAM)dwAlignment, 0));
	}
	inline BOOL SetMarquee(BOOL bOn, UINT uMsecBetweenUpdate) {
		ATLASSERT(::IsWindow(m_hWnd)); return ((BOOL) ::SendMessage(m_hWnd, PBM_SETMARQUEE, (WPARAM)bOn, (LPARAM)uMsecBetweenUpdate));
	}
	inline int SetMarqueeOptions(int nBarSize) {
		ATLASSERT(::IsWindow(m_hWnd)); return ((BOOL) ::SendMessage(m_hWnd, PBM_SETMARQUEEOPTIONS, (WPARAM)nBarSize, 0));
	}

	// Generated message map functions
protected:
	//{{AFX_MSG(CTextProgressCtrlEx)
	BOOL OnEraseBkgnd(HDC);
	void OnPaint(HDC wParam);
	void OnTimer(UINT_PTR nIDEvent);
	//}}AFX_MSG

	// handlers for shell progress control standard messages
	LRESULT OnSetRange(UINT, WPARAM, LPARAM lparamRange);
	LRESULT OnSetPos(UINT, WPARAM nNewPos, LPARAM);
	LRESULT OnOffsetPos(UINT, WPARAM nIncrement, LPARAM);
	LRESULT OnSetStep(UINT, WPARAM nStepInc, LPARAM);
	LRESULT OnStepIt(UINT, WPARAM, LPARAM);
	LRESULT OnSetRange32(UINT, WPARAM nLowLim, LPARAM nHighLim);
	LRESULT OnGetRange(UINT, WPARAM bWhichLimit, LPARAM pPBRange);
	LRESULT OnGetPos(UINT, WPARAM, LPARAM);
	LRESULT OnSetBarColor(UINT, WPARAM, LPARAM crBar);
	LRESULT OnSetBarBkColor(UINT, WPARAM, LPARAM crBarBk);

	// new handlers added by this class
	LRESULT OnGetBarColor(UINT, WPARAM, LPARAM);
	LRESULT OnGetBarBkColor(UINT, WPARAM, LPARAM);
	LRESULT OnSetTextColor(UINT, WPARAM, LPARAM crText);
	LRESULT OnGetTextColor(UINT, WPARAM, LPARAM);
	LRESULT OnSetTextBkColor(UINT, WPARAM, LPARAM crTextBk);
	LRESULT OnGetTextBkColor(UINT, WPARAM, LPARAM);
	LRESULT OnSetShowPercent(UINT, WPARAM bShow, LPARAM);
	LRESULT OnAlignText(UINT, WPARAM dwAlignment, LPARAM);
	LRESULT OnSetMarquee(UINT, WPARAM bOn, LPARAM nMsecBetweenUpdate);
	LRESULT OnSetMarqueeOptions(UINT, WPARAM nBarSize, LPARAM);

	// helper functions
	void CreateVerticalFont();
	void CommonPaint();

	//DECLARE_MESSAGE_MAP()

	// variables for class
	BOOL		m_bShowPercent;				// true to show percent complete as text
	CFont		m_VerticalFont;				// font for vertical progress bars
	COLORREF	m_crBarClr, m_crBarBkClr;	// bar colors
	COLORREF	m_crTextClr, m_crTextBkClr;	// text colors
	DWORD		m_dwTextStyle;				// alignment style for text
	int			m_nPos;						// current position within range
	int			m_nStepSize;				// current step size
	int			m_nMin, m_nMax;				// minimum and maximum values of range
	int			m_nMarqueeSize;				// size of sliding marquee bar in percent (0 - 100)
};

////////////////////////////////////////////////////////////////////////////////