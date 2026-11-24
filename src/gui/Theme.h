#pragma once
#include "pch.h"

// GUI 공용 색·글꼴. 60-30-10 배분:
// - 60% 화이트: 창 배경·패널·여백
// - 30% 중립색: 딥 네이비 사이드바·브랜드 패널, 네이비 그레이 글자·테두리·보조 버튼
// - 10% 보라: 평가 시작·새 테스트 같은 핵심 동작, 선택·포커스·진행 표시 (FP 로고의 보라와 같은 계열)
namespace theme {

// 화이트 (60%)
const COLORREF kWindowBg = RGB(248, 248, 251);
const COLORREF kPanel = RGB(255, 255, 255);
const COLORREF kCard = RGB(246, 247, 250);
// 중립색 (30%): 밝은 쪽은 테두리·눌림, 어두운 쪽은 글자
const COLORREF kBorder = RGB(228, 230, 237);
const COLORREF kControlBorder = RGB(203, 207, 218);
const COLORREF kHover = RGB(241, 242, 246);
const COLORREF kPressed = RGB(231, 233, 240);
const COLORREF kText = RGB(26, 29, 46);
const COLORREF kTextSecondary = RGB(92, 98, 117);
const COLORREF kTextDisabled = RGB(150, 155, 170);
// 딥 네이비: 사이드바·시작 화면 브랜드 패널
const COLORREF kNavy = RGB(22, 26, 48);
const COLORREF kNavyHover = RGB(35, 40, 68);
const COLORREF kNavySelected = RGB(46, 51, 84);
const COLORREF kNavyLine = RGB(50, 55, 88);
const COLORREF kOnNavy = RGB(236, 238, 245);
const COLORREF kOnNavySub = RGB(156, 162, 188);
const COLORREF kBrand = kNavy;  // 이전 홈 화면 호환 (다음 커밋에서 제거)
// 보라 (10%)
const COLORREF kAccent = RGB(124, 58, 237);
const COLORREF kAccentHover = RGB(109, 40, 217);
const COLORREF kAccentPressed = RGB(91, 33, 182);
const COLORREF kAccentTint = RGB(243, 238, 255);
const COLORREF kAccentOnNavy = RGB(178, 150, 255);  // 네이비 위에서 읽히는 밝은 보라
const COLORREF kSelected = kAccentTint;             // 목록 선택 행
const COLORREF kOnAccent = RGB(255, 255, 255);
const COLORREF kOnAccentSub = RGB(226, 216, 254);
// 상태색
const COLORREF kSuccess = RGB(21, 128, 61);
const COLORREF kSuccessTint = RGB(232, 245, 236);
const COLORREF kWarning = RGB(180, 83, 9);
const COLORREF kWarningTint = RGB(254, 243, 226);
const COLORREF kDanger = RGB(196, 43, 28);

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
