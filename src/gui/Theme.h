#pragma once
#include "pch.h"

// FP 모노그램의 자두색과 라벤더를 바탕으로 한 GUI 공용 색·글꼴.
namespace theme {

const COLORREF kWindowBg = RGB(244, 239, 249);
const COLORREF kPanel = RGB(255, 255, 255);
const COLORREF kCard = RGB(250, 247, 253);
const COLORREF kBorder = RGB(226, 215, 236);
const COLORREF kControlBorder = RGB(201, 184, 216);
const COLORREF kHover = RGB(242, 232, 250);
const COLORREF kSelected = RGB(232, 217, 246);
const COLORREF kText = RGB(42, 29, 53);
const COLORREF kTextSecondary = RGB(101, 82, 117);
const COLORREF kTextDisabled = RGB(133, 119, 145);
const COLORREF kBrand = RGB(48, 10, 80);
const COLORREF kAccent = RGB(105, 52, 160);
const COLORREF kAccentHover = RGB(87, 40, 142);
const COLORREF kAccentPressed = RGB(66, 23, 115);
const COLORREF kSuccess = RGB(15, 123, 15);
const COLORREF kDanger = RGB(196, 43, 28);
const COLORREF kAccentTint = RGB(241, 230, 252);
const COLORREF kOnAccent = RGB(255, 255, 255);
const COLORREF kOnAccentSub = RGB(232, 211, 251);

// 시작 화면·홈 화면·정보 창에 같이 쓰는 버전 표시
const wchar_t* const kAppVersion = L"0.1.0";

// 대화상자와 같은 한글 글꼴(맑은 고딕)을 DPI에 맞는 크기로 만든다
inline void CreateUiFont(CFont& font, int points, int weight, UINT dpi) {
  font.DeleteObject();
  LOGFONT lf{};
  lf.lfHeight = -MulDiv(points, static_cast<int>(dpi), 72);
  lf.lfWeight = weight;
  lf.lfCharSet = DEFAULT_CHARSET;
  lf.lfQuality = CLEARTYPE_QUALITY;
  wcscpy_s(lf.lfFaceName, L"맑은 고딕");
  font.CreateFontIndirect(&lf);
}

// Windows 기본 아이콘 글꼴. Windows 11은 Segoe Fluent Icons, Windows 10은 Segoe MDL2 Assets.
// 두 글꼴은 같은 코드 포인트를 쓴다 (예: U+E768 재생, U+E713 설정).
inline void CreateIconFont(CFont& font, int points, UINT dpi) {
  for (const wchar_t* face : {L"Segoe Fluent Icons", L"Segoe MDL2 Assets"}) {
    font.DeleteObject();
    LOGFONT lf{};
    lf.lfHeight = -MulDiv(points, static_cast<int>(dpi), 72);
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    wcscpy_s(lf.lfFaceName, face);
    font.CreateFontIndirect(&lf);
    // 설치되지 않은 글꼴이면 다른 글꼴로 대체되므로 실제 선택된 이름을 확인한다
    CClientDC dc(nullptr);
    CFont* old = dc.SelectObject(&font);
    wchar_t actual[LF_FACESIZE] = {};
    GetTextFaceW(dc.m_hDC, LF_FACESIZE, actual);
    dc.SelectObject(old);
    if (_wcsicmp(actual, face) == 0) return;
  }
}

inline void FillRoundRect(CDC* dc, const CRect& rc, int radius, COLORREF fill, COLORREF border) {
  CBrush brush(fill);
  CPen pen(PS_SOLID, 1, border);
  CBrush* oldBrush = dc->SelectObject(&brush);
  CPen* oldPen = dc->SelectObject(&pen);
  dc->RoundRect(rc, CPoint(radius, radius));
  dc->SelectObject(oldPen);
  dc->SelectObject(oldBrush);
}

inline void DrawHelpBadge(CDC* dc, const CRect& bounds, bool highlighted, UINT dpi) {
  const int size = MulDiv(17, static_cast<int>(dpi), 96);
  const CPoint center = bounds.CenterPoint();
  const CRect badge(center.x - size / 2, center.y - size / 2, center.x - size / 2 + size, center.y - size / 2 + size);
  const COLORREF ink = highlighted ? kOnAccent : kAccent;
  CBrush background(highlighted ? kAccent : kAccentTint);
  CBrush mark(ink);
  CBrush* previousBrush = dc->SelectObject(&background);
  CGdiObject* previousPen = dc->SelectStockObject(NULL_PEN);
  dc->Ellipse(badge);
  dc->SelectObject(&mark);
  const int stroke = MulDiv(2, static_cast<int>(dpi), 96);
  const int left = center.x - stroke / 2;
  dc->Ellipse(left, center.y - 2 * stroke, left + stroke, center.y - stroke);
  const CRect stem(left, center.y - stroke / 2, left + stroke, center.y + 2 * stroke + 1);
  dc->RoundRect(stem, CPoint(stroke, stroke));
  dc->SelectObject(previousBrush);
  dc->SelectObject(previousPen);
}

}  // namespace theme
