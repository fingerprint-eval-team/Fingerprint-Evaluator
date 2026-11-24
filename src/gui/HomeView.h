#pragma once
#include "pch.h"

#include <string>
#include <vector>

#include "ResultsPage.h"

// 홈 화면 (대시보드형): 메인 창 클라이언트 전체를 덮는 자식 창.
// 위쪽 머리글과 인사말, 바로가기 타일 4개(새 테스트는 보라 핵심 버튼, 나머지는 흰 타일), 최근 실행 목록, 아래쪽 정보 링크.
// 항목을 누르면 부모에 WM_APP_HOME_CMD(wParam = Cmd, lParam = 최근 실행 번호)를 보낸다. 실제 동작은 부모(CMainDlg)가 한다.
// 평가가 진행 중이면 타일 위에 "평가 진행 중" 띠가 생기고(누르면 실행 현황), 새 실행을 시작하는 타일은 잠긴다.
// 키보드: 화살표로 이동, Enter/Space로 실행.
class CHomeView : public CWnd {
 public:
  enum Cmd { kCmdRunning, kCmdNewTest, kCmdResume, kCmdLoadConfig, kCmdResults, kCmdOpenRun, kCmdAbout };

  ~CHomeView() override;
  BOOL Create(CWnd* parent, UINT id);
  void SetRunning(bool running, const CString& progress);  // progress: "42%" 같은 진행 표시
  void SetRunRoots(const std::vector<std::wstring>& roots);  // 최근 실행을 찾을 결과 폴더들 (바꾸면 다시 읽는다)
  void ReloadRuns();
  const RecentRun* RunAt(int index) const;

 protected:
  afx_msg void OnPaint();
  afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
  afx_msg void OnLButtonDown(UINT flags, CPoint pt);
  afx_msg void OnMouseMove(UINT flags, CPoint pt);
  afx_msg void OnMouseLeave();
  afx_msg UINT OnGetDlgCode();
  afx_msg void OnKeyDown(UINT key, UINT repeat, UINT flags);
  afx_msg void OnSetFocus(CWnd* old);
  afx_msg void OnKillFocus(CWnd* next);
  afx_msg void OnSize(UINT type, int cx, int cy);
  afx_msg LRESULT OnDpiChangedAfterParent(WPARAM, LPARAM);
  DECLARE_MESSAGE_MAP()

 private:
  enum class Kind { Banner, Primary, Tile, Link, Run, Footer };
  struct Item {
    Cmd cmd;
    Kind kind;
    wchar_t glyph = 0;
    CString title, desc;
    int run = -1;  // Kind::Run: m_runs 번호
    bool enabled = true;
    bool visible = true;
    CRect rect;
  };
  void EnsureResources();
  void RebuildItems();
  void Layout();
  int HitTest(CPoint pt) const;
  void Activate(int index);
  void MoveFocus(int step);
  int FindItem(Cmd cmd, Kind kind) const;
  void DrawRun(CDC& dc, const Item& it, bool hot);
  int Px(int value96) const { return MulDiv(value96, m_dpi, 96); }

  std::vector<Item> m_items;
  std::vector<std::wstring> m_roots;
  std::vector<RecentRun> m_runs;
  bool m_running = false;
  CString m_progress;
  CRect m_recentHead{0, 0, 0, 0};
  int m_hot = -1;
  int m_focus = -1;
  bool m_tracking = false;
  bool m_hasFocus = false;
  int m_dpi = 0;
  HICON m_icon = nullptr;
  CFont m_brandFont, m_headFont, m_titleFont, m_textFont, m_smallFont, m_iconFont, m_chevronFont;
};
