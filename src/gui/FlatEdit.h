#pragma once
#include "pch.h"

#include <memory>
#include <vector>

#include "Theme.h"

// 테두리를 둥근 사각형으로 직접 그리는 한 줄 입력 상자.
// 비클라이언트 영역(WM_NCCALCSIZE/WM_NCPAINT)만 바꿔 안쪽 여백·세로 가운데 정렬·흰 바탕 둥근 테두리를 만들고,
// 포커스가 있으면 강조색 테두리와 아래쪽 강조선을 그린다 (CFlatCombo와 같은 표시). 입력·DDX·EN_* 알림은 기본 그대로다.
class CFlatEdit : public CEdit {
 public:
  explicit CFlatEdit(COLORREF parentBg = theme::kPanel) : m_parentBg(parentBg) {}

  // parent의 직속 자식 중 한 줄 입력 상자(ES_MULTILINE 없음)를 모두 CFlatEdit로 바꾼다. 만든 객체는 owned가 가진다.
  static void SubclassEdits(CWnd* parent, COLORREF parentBg, std::vector<std::unique_ptr<CFlatEdit>>& owned);

 protected:
  afx_msg void OnNcCalcSize(BOOL calcValidRects, NCCALCSIZE_PARAMS* params);
  afx_msg void OnNcPaint();
  afx_msg void OnSetFocus(CWnd* old);
  afx_msg void OnKillFocus(CWnd* next);
  afx_msg void OnEnable(BOOL enable);
  afx_msg LRESULT OnSetFont(WPARAM, LPARAM);  // 글꼴(DPI)이 바뀌면 여백을 다시 계산한다
  DECLARE_MESSAGE_MAP()

 private:
  int Px(int v) const;
  void RedrawFrame();

  COLORREF m_parentBg;
};
