// TextProgressCtrl.cpp : implementation file
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
// computer or anything else vaguely within its vicinity.
//
// Expect bugs.
//
// Please use and enjoy. Please let me know of any bugs/mods/improvements
// that you have found/implemented and I will fix/incorporate them into this
// file.

////////////////////////////////////////////////////////////////////////////////
// Revision History		- by PJ Arends (pja@telus.net)
// July 1, 2001			- added functionality to set font to parent windows font
// July 15, 2001		- added SetTextColor() and GetTextColor() functions
// August 2, 2001		- Added functionality where the control now responds to
//						  PBM_* progress bar messages. Now the control can be
//						  controlled by sending messages to its HWND.
//						- added PBM_SETTEXTCOLOR and PBM_SETSHOWTEXT messages
//						- added the ability to call GetPos() via OnGetPos()
// January 3, 2005		- By Kriz: Added two basic methods some other changes to
//						  allow the user to switch between the three alignment
//						  styles LEFT, CENTER and RIGHT - even on the fly if that's
//						  needed. Methods: AlignText and AlignTextInvalidate
//						- By C Maunder: updated the code so it compiles in VC7
// March 6, 2006		- By A. Bommarito: changed to allow text in vertical
//						  progress bars; added method to change text background
//						  color; added capability to independently control display
//						  of text and percentage complete; added marquee mode;
//						  changed to exclusively use messaging interface
// February 22, 2007	- By A. Bommarito: changed to fix problem where progress bar
//						  border didn't display on XP and above when a manifest file
//						  specified use of Version 6 of the Common Controls; fixed
//						  problem with AppendFormat method being used without an
//						  _T() around the format string - caused compile error when
//						  using Unicode (message from dev75040); added complete turn
//						  off of Marquee style (message from Robert Pickford)
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <atltypes.h>
#include "TextProgressCtrlEx.h"

//#include "CMemDC.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////////////////////////////////////////////////////////
// CTextProgressCtrlEx

#define IDT_MARQUEE			1					// timer used to update marquee style progress bar
#define	EDGE_SPACE			2					// number of pixels to space text away from edge of progress bar

CTextProgressCtrlEx::CTextProgressCtrlEx() {
	// default range
	m_nMin = 0;
	m_nMax = 100;

	// default step size
	m_nStepSize = 10;

	// initial starting position
	m_nPos = 0;

	// default colors
	m_crBarClr = CLR_DEFAULT;
	m_crBarBkClr = CLR_DEFAULT;
	m_crTextClr = CLR_DEFAULT;
	m_crTextBkClr = CLR_DEFAULT;

	// default to show percent complete, all text centered
	m_bShowPercent = TRUE;
	m_dwTextStyle = DT_CENTER;

	// default marquee bar size of 20% of full progress bar
	m_nMarqueeSize = 20;
}

CTextProgressCtrlEx::~CTextProgressCtrlEx() {
	// delete vertical font if needed
	if (m_VerticalFont.m_hFont)
		m_VerticalFont.DeleteObject();
}

/*BEGIN_MESSAGE_MAP(CTextProgressCtrlEx, CProgressCtrl)
//{{AFX_MSG_MAP(CTextProgressCtrlEx)
ON_WM_ERASEBKGND()
ON_WM_PAINT()
ON_WM_TIMER()
//}}AFX_MSG_MAP
ON_MESSAGE(PBM_SETRANGE, OnSetRange)
ON_MESSAGE(PBM_SETPOS, OnSetPos)
ON_MESSAGE(PBM_DELTAPOS, OnOffsetPos)
ON_MESSAGE(PBM_SETSTEP, OnSetStep)
ON_MESSAGE(PBM_STEPIT, OnStepIt)
ON_MESSAGE(PBM_SETRANGE32, OnSetRange32)
ON_MESSAGE(PBM_GETRANGE, OnGetRange)
ON_MESSAGE(PBM_GETPOS, OnGetPos)
ON_MESSAGE(PBM_SETBARCOLOR, OnSetBarColor)
ON_MESSAGE(PBM_SETBKCOLOR, OnSetBarBkColor)

ON_MESSAGE(PBM_GETBARCOLOR, OnGetBarColor)
ON_MESSAGE(PBM_GETBKCOLOR, OnGetBarBkColor)
ON_MESSAGE(PBM_SETTEXTCOLOR, OnSetTextColor)
ON_MESSAGE(PBM_GETTEXTCOLOR, OnGetTextColor)
ON_MESSAGE(PBM_SETTEXTBKCOLOR, OnSetTextBkColor)
ON_MESSAGE(PBM_GETTEXTBKCOLOR, OnGetTextBkColor)
ON_MESSAGE(PBM_SETSHOWPERCENT, OnSetShowPercent)
ON_MESSAGE(PBM_ALIGNTEXT, OnAlignText)
ON_MESSAGE(PBM_SETMARQUEE, OnSetMarquee)
ON_MESSAGE(PBM_SETMARQUEEOPTIONS, OnSetMarqueeOptions)
END_MESSAGE_MAP()*/

////////////////////////////////////////////////////////////////////////////////
// CTextProgressCtrlEx message handlers

BOOL CTextProgressCtrlEx::OnEraseBkgnd(HDC /*pDC*/) {
	return TRUE;
}

void CTextProgressCtrlEx::OnPaint(HDC wParam) {
	if (m_nMin >= m_nMax)
		return;

	CPaintDC PaintDC(m_hWnd);					// device context for painting
	CRect rc;
	GetClientRect(rc);
	CMemoryDC dc(PaintDC, rc);						// memory device context
													//CDCHandle dc{ wParam };

													// get colors to use
	COLORREF crBarColor = (COLORREF)OnGetBarColor(0, 0, 0);
	COLORREF crBarBkColor = (COLORREF)OnGetBarBkColor(0, 0, 0);
	COLORREF crTextColor = (COLORREF)OnGetTextColor(0, 0, 0);
	COLORREF crTextBkColor = (COLORREF)OnGetTextBkColor(0, 0, 0);

	if (GetFont())
		dc.SelectFont(GetFont());
	else
		dc.SelectFont(GetParent().GetFont());

	// get bar dimensions and draw bar outline
	BOOL bLeft = true, bMiddle = true, bRight = true;
	CRect LeftRect, MiddleRect, RightRect, ClientRect;
	GetClientRect(&ClientRect);
	DrawEdge(dc, ClientRect, EDGE_SUNKEN, BF_ADJUST | BF_RECT | BF_FLAT);
	LeftRect = MiddleRect = RightRect = ClientRect;

	// compute the fractional position
	double dFraction = (double)(m_nPos - m_nMin) / ((double)(m_nMax - m_nMin));

	// get the control style
	DWORD dwStyle = GetStyle();

	// create a vertical font if needed
	if ((dwStyle & PBS_VERTICAL) && (!m_VerticalFont.m_hFont))
		CreateVerticalFont();

	if (dwStyle & PBS_MARQUEE) {
		if (dwStyle & PBS_VERTICAL) {
			MiddleRect.top = MiddleRect.bottom - (int)((MiddleRect.Height()) * dFraction);
			MiddleRect.bottom = MiddleRect.top + (((LeftRect.Height() * m_nMarqueeSize) + 50) / 100);
			if (MiddleRect.bottom >= LeftRect.bottom) {
				bLeft = false;
				MiddleRect.bottom = LeftRect.bottom;
			} else
				LeftRect.top = MiddleRect.bottom;
			if (MiddleRect.top <= RightRect.top) {
				bRight = false;
				MiddleRect.top = RightRect.top;
			} else
				RightRect.bottom = MiddleRect.top;
		} else {
			MiddleRect.right = MiddleRect.left + (int)((MiddleRect.Width()) * dFraction);
			MiddleRect.left = MiddleRect.right - (((LeftRect.Width() * m_nMarqueeSize) + 50) / 100);
			if (MiddleRect.left <= LeftRect.left) {
				bLeft = false;
				MiddleRect.left = LeftRect.left;
			} else
				LeftRect.right = MiddleRect.left;
			if (MiddleRect.right >= RightRect.right) {
				bRight = false;
				MiddleRect.right = RightRect.right;
			} else
				RightRect.left = MiddleRect.right;
		}
	} else {
		bLeft = false;
		if (dwStyle & PBS_VERTICAL) {
			MiddleRect.top = MiddleRect.bottom - (int)((MiddleRect.Height()) * dFraction);
			RightRect.bottom = MiddleRect.top;
		} else {
			MiddleRect.right = MiddleRect.left + (int)((MiddleRect.Width()) * dFraction);
			RightRect.left = MiddleRect.right;
		}
	}
	if (bLeft)
		dc.FillSolidRect(LeftRect, crBarBkColor);
	if (bMiddle)
		dc.FillSolidRect(MiddleRect, crBarColor);
	if (bRight)
		dc.FillSolidRect(RightRect, crBarBkColor);

	// draw text if needed
	CString str;
	GetWindowText(str);
	if (m_bShowPercent)
		str.AppendFormat(_T("%d%%"), (int)((dFraction * 100.0) + 0.5));
	if (str.GetLength()) {
		HFONT h_old_font = NULL;
		CPoint ptStart;
		CSize szText;

		if (dwStyle & PBS_VERTICAL) {
			h_old_font = dc.SelectFont(m_VerticalFont);

			dc.GetTextExtent(str, str.GetLength(), &szText);
			switch (m_dwTextStyle) {
			case DT_LEFT:
				ptStart.y = ClientRect.bottom - 1 - EDGE_SPACE;
				break;
			case DT_RIGHT:
				ptStart.y = ClientRect.top + szText.cx + EDGE_SPACE;
				break;
			case DT_CENTER:
			default:
				ptStart.y = ClientRect.top + szText.cx + (ClientRect.Height() - szText.cx) / 2;
				break;
			}
			ptStart.x = ClientRect.left + ((ClientRect.Width() - szText.cy) / 2);
		} else {
			dc.GetTextExtent(str, str.GetLength(), &szText);
			switch (m_dwTextStyle) {
			case DT_LEFT:
				ptStart.x = ClientRect.left + EDGE_SPACE;
				break;
			case DT_RIGHT:
				ptStart.x = ClientRect.right - szText.cx - 1 - EDGE_SPACE;
				break;
			case DT_CENTER:
			default:
				ptStart.x = ClientRect.left + ((ClientRect.Width() - szText.cx) / 2);
				break;
			}
			ptStart.y = ClientRect.top + ((ClientRect.Height() - szText.cy) / 2);
		}

		dc.SetBkMode(TRANSPARENT);

		CRgn rgn;

		if (bLeft) {
			rgn.CreateRectRgn(LeftRect.left, LeftRect.top, LeftRect.right, LeftRect.bottom);
			dc.SelectClipRgn(rgn.m_hRgn);
			dc.SetTextColor(crTextColor);
			dc.ExtTextOut(ptStart.x, ptStart.y, ETO_CLIPPED, &ClientRect, str);
			rgn.DeleteObject();
		}

		if (bMiddle) {
			rgn.CreateRectRgn(MiddleRect.left, MiddleRect.top, MiddleRect.right, MiddleRect.bottom);
			dc.SelectClipRgn(rgn.m_hRgn);
			dc.SetTextColor(crTextBkColor);
			dc.ExtTextOut(ptStart.x, ptStart.y, ETO_CLIPPED, &ClientRect, str);
			rgn.DeleteObject();
		}

		if (bRight) {
			rgn.CreateRectRgn(RightRect.left, RightRect.top, RightRect.right, RightRect.bottom);
			dc.SelectClipRgn(rgn.m_hRgn);
			dc.SetTextColor(crTextColor);
			dc.ExtTextOut(ptStart.x, ptStart.y, ETO_CLIPPED, &ClientRect, str);
			rgn.DeleteObject();
		}

		if (h_old_font)
			dc.SelectFont(h_old_font);
	}
}

void CTextProgressCtrlEx::OnTimer(UINT_PTR nIDEvent) {
	static int nDirection = +1;

	if (nIDEvent == IDT_MARQUEE) {
		int nPosition = GetPos();

		int nMax = (int)((((double)(100 + m_nMarqueeSize) * (double)m_nMax) + 0.5) / 100.0);
		if ((nDirection == 1) && (nPosition >= nMax))
			nDirection = -1;
		else if ((nDirection == -1) && (nPosition <= m_nMin))
			nDirection = +1;

		SetPos(nPosition + nDirection);
	} else {
		SetMsgHandled(false);
	}
}

LRESULT CTextProgressCtrlEx::OnSetRange(UINT uMsg, WPARAM, LPARAM lparamRange) {
	// set new 16-bit range, returning old one
	return (OnSetRange32(uMsg, LOWORD(lparamRange), HIWORD(lparamRange)));
}

LRESULT CTextProgressCtrlEx::OnSetPos(UINT, WPARAM nNewPos, LPARAM) {
	// save old position for return
	int nOldPos = m_nPos;

	// set new position
	m_nPos = (int)nNewPos;

	// limit position to inside of range
	DWORD dwStyle = GetStyle();
	if (m_nPos < m_nMin)
		m_nPos = m_nMin;
	if (dwStyle & PBS_MARQUEE) {
		int nMax = (int)((((double)(100 + m_nMarqueeSize) * (double)m_nMax) + 0.5) / 100.0);
		if (m_nPos > nMax)
			m_nPos = nMax;
	} else if (m_nPos > m_nMax)
		m_nPos = m_nMax;

	// call a common routine to redraw window
	CommonPaint();

	// return old position
	return ((LRESULT)nOldPos);
}

LRESULT CTextProgressCtrlEx::OnOffsetPos(UINT uMsg, WPARAM nIncrement, LPARAM) {
	// set new position, returning old one
	return (OnSetPos(uMsg, (WPARAM)(m_nPos + (int)nIncrement), 0));
}

LRESULT CTextProgressCtrlEx::OnSetStep(UINT, WPARAM nStepInc, LPARAM) {
	// save old step size for return
	int nOldStep = m_nStepSize;

	// set new step size
	m_nStepSize = (int)nStepInc;

	// return old step size
	return ((LRESULT)nOldStep);
}

LRESULT CTextProgressCtrlEx::OnStepIt(UINT uMsg, WPARAM, LPARAM) {
	// compute new position, wrapping if needed
	int nPos = m_nPos + m_nStepSize;
	if (nPos < m_nMin)
		nPos = m_nMax - abs(m_nMin - nPos);
	else if (nPos > m_nMax)
		nPos = m_nMin + abs(nPos - m_nMax);

	// set new position, returning old one
	return ((LRESULT)OnSetPos(uMsg, (WPARAM)nPos, 0));
}

LRESULT CTextProgressCtrlEx::OnSetRange32(UINT, WPARAM nLowLim, LPARAM nHighLim) {
	// save old range for return
	int nOldMin = m_nMin;
	int nOldMax = m_nMax;

	// set new range
	m_nMin = (int)nLowLim;
	m_nMax = (int)nHighLim;

	// return old range (16 bit only)
	return (MAKELRESULT(LOWORD(nOldMin), LOWORD(nOldMax)));
}

LRESULT CTextProgressCtrlEx::OnGetRange(UINT, WPARAM bWhichLimit, LPARAM pPBRange) {
	// get arguments into proper types
	BOOL bType = (BOOL)bWhichLimit;
	PPBRANGE pRange = (PPBRANGE)pPBRange;

	// return both range limits if a pointer was provided
	if (pRange) {
		pRange->iLow = m_nMin;
		pRange->iHigh = m_nMax;
	}

	// return selected range limit
	return ((LRESULT)(bType ? m_nMin : m_nMax));
}

LRESULT CTextProgressCtrlEx::OnGetPos(UINT, WPARAM, LPARAM) {
	// return current position
	return ((LRESULT)m_nPos);
}

LRESULT CTextProgressCtrlEx::OnSetBarColor(UINT, WPARAM, LPARAM crBar) {
	// call a common routine to redraw window
	CommonPaint();

	// set new bar color, returning old one
	COLORREF crOldBarClr = m_crBarClr;
	m_crBarClr = (COLORREF)crBar;
	return ((LRESULT)crOldBarClr);
}

LRESULT CTextProgressCtrlEx::OnSetBarBkColor(UINT, WPARAM, LPARAM crBarBk) {
	// call a common routine to redraw window
	CommonPaint();

	// set new bar background color, returning old one
	COLORREF crOldBarBkClr = m_crBarBkClr;
	m_crBarBkClr = (COLORREF)crBarBk;
	return ((LRESULT)crOldBarBkClr);
}

LRESULT CTextProgressCtrlEx::OnGetBarColor(UINT, WPARAM, LPARAM) {
	// return current bar color
	return ((LRESULT)((m_crBarClr == CLR_DEFAULT) ? ::GetSysColor(COLOR_HIGHLIGHT) : m_crBarClr));
}

LRESULT CTextProgressCtrlEx::OnGetBarBkColor(UINT, WPARAM, LPARAM) {
	// return current bar background color
	return ((LRESULT)((m_crBarBkClr == CLR_DEFAULT) ? ::GetSysColor(COLOR_BTNFACE) : m_crBarBkClr));
}

LRESULT CTextProgressCtrlEx::OnSetTextColor(UINT, WPARAM, LPARAM crText) {
	// call a common routine to redraw window
	CommonPaint();

	// set new text color, returning old one
	COLORREF crOldTextClr = m_crTextClr;
	m_crTextClr = (COLORREF)crText;
	return ((LRESULT)crOldTextClr);
}

LRESULT CTextProgressCtrlEx::OnGetTextColor(UINT, WPARAM, LPARAM) {
	// return current text color
	return ((LRESULT)((m_crTextClr == CLR_DEFAULT ? ::GetSysColor(COLOR_WINDOWTEXT) : m_crTextClr)));
}

LRESULT CTextProgressCtrlEx::OnSetTextBkColor(UINT, WPARAM, LPARAM crTextBkClr) {
	// call a common routine to redraw window
	CommonPaint();

	// set new text background color, returning old one
	COLORREF crOldTextBkClr = m_crTextBkClr;
	m_crTextBkClr = (COLORREF)crTextBkClr;
	return ((LRESULT)crOldTextBkClr);
}

LRESULT CTextProgressCtrlEx::OnGetTextBkColor(UINT, WPARAM, LPARAM) {
	// return current text background color
	return ((LRESULT)((m_crTextBkClr == CLR_DEFAULT ? ::GetSysColor(COLOR_WINDOW) : m_crTextBkClr)));
}

LRESULT CTextProgressCtrlEx::OnSetShowPercent(UINT, WPARAM bShow, LPARAM) {
	// call a common routine to redraw window
	CommonPaint();

	// set new percent completed display state, returning old one
	BOOL bOldShow = m_bShowPercent;
	m_bShowPercent = (BOOL)bShow;
	return ((LRESULT)bOldShow);
}

LRESULT CTextProgressCtrlEx::OnAlignText(UINT, WPARAM dwAlignment, LPARAM) {
	// get argument into proper type
	DWORD dwAlign = (DWORD)dwAlignment;

	// check input
	ATLASSERT((dwAlign == DT_LEFT) || (dwAlign == DT_RIGHT) || (dwAlign == DT_CENTER));

	// call a common routine to redraw window
	CommonPaint();

	// set new text alignment mode, returning old one
	DWORD dwOldTextStyle = m_dwTextStyle;
	m_dwTextStyle = dwAlign;
	return ((LRESULT)dwOldTextStyle);
}

LRESULT CTextProgressCtrlEx::OnSetMarquee(UINT, WPARAM bShow, LPARAM uMsecBetweenUpdate) {
	// call a common routine to redraw window
	CommonPaint();

	// can't have percent complete for this style
	m_bShowPercent = false;

	// turn marquee mode on or off
	if ((BOOL)bShow) {
		ModifyStyle(0, PBS_MARQUEE);
		SetTimer(IDT_MARQUEE, (UINT)uMsecBetweenUpdate, NULL);
	} else {
		KillTimer(IDT_MARQUEE);
		ModifyStyle(PBS_MARQUEE, 0);
	}

	// return current marquee mode
	return ((LRESULT)bShow);
}

LRESULT CTextProgressCtrlEx::OnSetMarqueeOptions(UINT, WPARAM nBarSize, LPARAM) {
	// get argument into proper type
	int nBar = (int)nBarSize;

	// call a common routine to redraw window
	CommonPaint();

	// set new marquee bar size
	m_nMarqueeSize = nBar;

	// return bar size (have to return something from a message)
	return ((LRESULT)nBar);
}

////////////////////////////////////////////////////////////////////////////////
// CTextProgressCtrlEx helper functions

void CTextProgressCtrlEx::CreateVerticalFont() {
	// delete old font if it exists
	if (m_VerticalFont.m_hFont)
		m_VerticalFont.DeleteObject();
	// create a new vertical font based on the current font
	LOGFONT lf;
	CFontHandle current_font(GetFont());
	current_font.GetLogFont(lf);
	lf.lfEscapement = lf.lfOrientation = 900;
	m_VerticalFont.CreateFontIndirect(&lf);
}

void CTextProgressCtrlEx::CommonPaint() {
	//
	// common place to mess with redraw options
	//
	// note that Invalidate() by itself won't cause
	// a repaint when this control is used in a modeless
	// dialog box
	//
	// also note that the marquee timer won't work in a modeless
	// dialog - to use marquee mode in a modeless dialog the code
	// that is running will have to pump Windows messages
	//

	RedrawWindow();
}
