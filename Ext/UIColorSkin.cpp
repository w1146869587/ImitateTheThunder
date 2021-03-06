#pragma once
#include "StdAfx.h"
#include "UIColorSkin.h"

#define MAX_VALUE			230		//颜色值最大值
#define ORG_VALUE			125		//基准主颜色值
#define ASS_VALUE			100		//辅助颜色值
#define RATE_SEL_VALUE		0.6		//选中时，亮度变化率
#define RATE_OVER_VALUE		0.8		//鼠标移上时，亮度变化率

//基础颜色表，可扩展
static const DWORD g_Palette[]=
{
	RGB(ORG_VALUE, 0, 0), RGB(ORG_VALUE, 0, ASS_VALUE), RGB(ORG_VALUE, ASS_VALUE, 0), RGB(ORG_VALUE, ASS_VALUE, ASS_VALUE), 	
	RGB(0, ORG_VALUE, 0), RGB(0, ORG_VALUE, ASS_VALUE), RGB(ASS_VALUE, ORG_VALUE, 0), RGB(ASS_VALUE, ORG_VALUE, ASS_VALUE), 
	RGB(0, 0, ORG_VALUE), RGB(0, ASS_VALUE, ORG_VALUE), RGB(ASS_VALUE, 0, ORG_VALUE), RGB(ASS_VALUE, ASS_VALUE, ORG_VALUE), 
	RGB(ORG_VALUE, ORG_VALUE, 0), RGB(ORG_VALUE, ORG_VALUE, ASS_VALUE), 
	RGB(ORG_VALUE, 0, ORG_VALUE), RGB(ORG_VALUE, ASS_VALUE, ORG_VALUE), 
	RGB(ASS_VALUE, ORG_VALUE, ORG_VALUE), RGB(0, ORG_VALUE, ORG_VALUE),
	RGB(ORG_VALUE, ORG_VALUE, ORG_VALUE),
};
const int g_nMaxColumn = sizeof(g_Palette)/sizeof(DWORD);

CColorSkinUI::CColorSkinUI()
:m_nSideLen(0)
,m_nLine(0)
,m_nColumn(0)
,m_hMemDC(NULL)
,m_hMemBmp(NULL)
,m_pBuffer(NULL)
,m_nPreWidth(-1)
,m_nPreHeight(-1)
,m_nCurSelLine(0)
,m_nCurSelCol(0)
,m_nPreCurSelLine(-1)
,m_nPreCurSelCol(-1)
,m_pSelectCallback(NULL)
{
	ZeroMemory(&m_bitmap, sizeof(BITMAP));
}

CColorSkinUI::~CColorSkinUI()
{

	if ( m_hMemDC )
	{
		DeleteDC(m_hMemDC);
		m_hMemDC = NULL;
	}
	if ( m_hMemBmp )
	{
		DeleteObject(m_hMemBmp);
		m_hMemBmp = NULL;
	}
	if ( m_pBuffer )
	{
		free(m_pBuffer);
		m_pBuffer = NULL;
	}
}

void CColorSkinUI::SetPos( RECT rc )
{
	CControlUI::SetPos(rc);
	int nWidth	= GetWidth();
	int nHeight = GetHeight();
	m_nColumn	= (nWidth+1)/(m_nSideLen+1);
	if ( m_nColumn>g_nMaxColumn )
	{
		m_nColumn = g_nMaxColumn;
		m_nSideLen = (nWidth+1)/m_nColumn-1;
	}
	m_nLine		= (nHeight+1)/(m_nSideLen+1);
	m_arrColor.Resize(m_nLine, m_nColumn);
	InitBitmap(nWidth, nHeight);
}

void CColorSkinUI::DoInit()
{
	CControlUI::DoInit();
}

void CColorSkinUI::SetAttribute( LPCTSTR pstrName, LPCTSTR pstrValue )
{
	if ( _tcscmp(pstrName, _T("sidelen")) == 0 )
		m_nSideLen = _ttoi(pstrValue);
	else
		CControlUI::SetAttribute(pstrName, pstrValue);
}

void CColorSkinUI::DoPaint( HDC hDC, const RECT& rcPaint )
{
	if ( NULL == m_hMemDC )
		m_hMemDC = CreateCompatibleDC(hDC);
	RECT rcInsert;
	IntersectRect(&rcInsert, &m_rcItem, &rcPaint);
	HBITMAP hOldBmp = (HBITMAP)SelectObject(m_hMemDC, m_hMemBmp);
	::BitBlt(hDC, rcInsert.left, rcInsert.top, rcInsert.right-rcInsert.left, rcInsert.bottom-rcInsert.top, \
		m_hMemDC, rcInsert.left-m_rcItem.left, rcInsert.top-m_rcItem.top, SRCCOPY);
	m_hMemBmp = (HBITMAP)::SelectObject(m_hMemDC, hOldBmp);
}

void CColorSkinUI::InitBitmap(int nWidth, int nHeight)
{
	m_bitmap.bmWidth	=nWidth;
	m_bitmap.bmHeight	=nHeight;
	m_bitmap.bmBitsPixel=32;
	m_bitmap.bmPlanes	=1;
	m_bitmap.bmWidthBytes=nWidth*4;
	m_bitmap.bmType		=0;
	int nBytes = (((nWidth*32+31)>>5)<<2)*nHeight;
	if ( nWidth != m_nPreWidth || nHeight != m_nPreHeight )
	{
		if ( m_pBuffer )
			free(m_pBuffer);
		m_pBuffer	=malloc(nBytes);
		m_nPreWidth	=nWidth;
		m_nPreHeight=nHeight;
	}
	memset(m_pBuffer, 0xff, nBytes);
	m_bitmap.bmBits		=m_pBuffer;
	PaintPalette();
}

void CColorSkinUI::DoEvent( TEventUI& event )
{
	switch( event.Type )
	{
	case UIEVENT_MOUSEMOVE:
		{//鼠标移动时绘制当前焦点下颜色选中状态
			int xPos = (event.ptMouse.x-m_rcItem.left)/(m_nSideLen+1);
			int yPos = (event.ptMouse.y-m_rcItem.top)/(m_nSideLen+1);
			PaintPalette(false, yPos, xPos);//部分绘制，提高绘图效率
			Invalidate();
			break;
		}
	case UIEVENT_BUTTONDOWN:
		{
			int xPos = (event.ptMouse.x-m_rcItem.left)/(m_nSideLen+1);
			int yPos = (event.ptMouse.y-m_rcItem.top)/(m_nSideLen+1);
			if ( yPos>=m_nLine || xPos>=m_nColumn )
				break;
			m_nCurSelCol  = xPos;
			m_nCurSelLine = yPos;
			//绘制选中状态区域
			PaintPalette(true);
			if ( m_pSelectCallback )
			{//重载回调后，颜色值改编后就能进行响应了（更换界面颜色背景……）
				DWORD dwColor = m_arrColor.GetAt(m_nCurSelLine, m_nCurSelCol);
				dwColor = 0xff000000 + RGB( GetBValue(dwColor), GetGValue(dwColor), GetRValue(dwColor) );
				m_pSelectCallback->ColorTest(dwColor);
			}
			Invalidate();
		}
		break;
	case UIEVENT_MOUSEHOVER:
		break;
	case UIEVENT_MOUSELEAVE:
		//鼠标移开后还原鼠标所在区域的颜色值
		PaintPalette(false, m_nCurSelLine, m_nCurSelCol);
		Invalidate();
		break;
	default:
		break;
	}
	CControlUI::DoEvent(event);
}

void CColorSkinUI::PaintPalette(bool bPaintAll/*=true*/, const int nCurLine/*=0*/, const int nCulCol/*=0*/)
{//此函数就是用来绘制每一个小区域颜色快的，包括当前选中颜色状态和当前鼠标焦点所在区域
	if ( nCurLine == m_nPreCurSelLine && nCulCol == m_nPreCurSelCol )
		return ;//选中状态未改变，无需绘制
	if ( nCurLine>=m_nLine || nCulCol>=m_nColumn )
		return ;//鼠标移到有效区域外面去了，无需绘制
	if ( bPaintAll )
	{
		if ( m_hMemBmp )
		{
			DeleteObject(m_hMemBmp);
			m_hMemBmp = NULL;
		}
		m_hMemBmp = CreateBitmapIndirect(&m_bitmap);
	}
	//绘制到m_hMemBmp位图上面
	HDC hDC = CreateCompatibleDC(NULL);
	HBITMAP hBmp = (HBITMAP)SelectObject(hDC, m_hMemBmp);
	if ( bPaintAll )
	{
		for ( int i=0; i<m_nLine; ++i )
			for ( int j=0; j<m_nColumn; ++j )
			{
				DWORD dwColor = PaintSide(hDC, i, j, (i == m_nCurSelLine && j == m_nCurSelCol));
				m_arrColor.SetAt(i, j, dwColor);
			}
	}
	else
	{
		if ( m_nPreCurSelCol!=-1 && m_nPreCurSelLine!=-1 )
		{//还原前一个颜色快
			PaintSide(hDC, m_nPreCurSelLine, m_nPreCurSelCol, (m_nPreCurSelLine == m_nCurSelLine && m_nPreCurSelCol == m_nCurSelCol));
		}
		//绘制当前选中的颜色
		PaintSide(hDC, nCurLine, nCulCol, true);
		m_nPreCurSelLine= nCurLine;
		m_nPreCurSelCol	= nCulCol;
	}
	m_hMemBmp = (HBITMAP)SelectObject(hDC, hBmp);
	DeleteDC(hDC);
}

DWORD CColorSkinUI::PaintSide( HDC hDC, const int nLine, const int nColumn, bool bSelect )
{//绘制每一个小的块
	DWORD dwRetColor = 0;
	RECT rcFill = {0, 0, 0, 0};
	DWORD dwColor = 0, dwPreColor = 0;
	int r = 0, g = 0, b = 0;
	float fRate = (MAX_VALUE-ORG_VALUE)/(float)(m_nLine-1);
	rcFill.left	= (m_nSideLen+1)*nColumn;
	rcFill.top	= (m_nSideLen+1)*nLine;
	rcFill.right= rcFill.left+m_nSideLen;
	rcFill.bottom=rcFill.top+m_nSideLen;
	if ( nLine == 0 )
		dwColor = g_Palette[nColumn];//直接使用每一列的颜色基值
	else
	{//在每一列的颜色基值上面变换颜色
		r = GetRValue(g_Palette[nColumn]);
		g = GetGValue(g_Palette[nColumn]);
		b = GetBValue(g_Palette[nColumn]);
		if ( r>=ORG_VALUE ) r+=(int)(nLine*fRate);
		if ( g>=ORG_VALUE ) g+=(int)(nLine*fRate);
		if ( b>=ORG_VALUE ) b+=(int)(nLine*fRate);
		dwColor = RGB(r, g, b);
	}
	dwRetColor = dwColor;
	if ( bSelect )
	{//选中状态下，颜色值变为原来的0.6倍，也就是对亮度进行了处理
		dwColor = RGB(GetRValue(dwColor)*RATE_SEL_VALUE, GetGValue(dwColor)*RATE_SEL_VALUE, \
			GetBValue(dwColor)*RATE_SEL_VALUE );
	}
	HBRUSH hBrush = CreateSolidBrush(dwColor);
	FillRect(hDC, &rcFill, hBrush);
	DeleteObject(hBrush);
	return dwRetColor;
}

DWORD CColorSkinUI::GetSelectColor()
{
	DWORD dwColor = m_arrColor.GetAt(m_nCurSelLine, m_nCurSelCol);
	//这里的颜色需要我们还原成原始的RGB格式
	dwColor = 0xff000000 + RGB( GetBValue(dwColor), GetGValue(dwColor), GetRValue(dwColor) );
	return dwColor;
}

void CColorSkinUI::SetCurSelPos( const int nLine, const int nColumn )
{
	m_nCurSelLine	= nLine;
	m_nCurSelCol	= nColumn;
}
