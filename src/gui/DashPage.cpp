#include "pch.h"

#include "DashPage.h"

#include "Theme.h"

namespace {
const UINT_PTR kTimerElapsed = 1;
const wchar_t* const kStepNames[6] = {L"검증", L"이미지", L"계획", L"특징 추출", L"비교", L"리포트"};

CString percentText(double v) {
  CString s;
  if (fpe::isNA(v)) {
    s = L"N/A";
  } else {
    s.Format(L"%.2f%%", v * 100);
  }
  return s;
}
}  // namespace

BEGIN_MESSAGE_MAP(CDashPage, CDialogEx)
  ON_WM_PAINT()
  ON_WM_ERASEBKGND()
  ON_WM_TIMER()
  ON_WM_SIZE()
  ON_WM_CTLCOLOR()
  ON_MESSAGE(WM_DPICHANGED_AFTERPARENT, &CDashPage::OnDpiChangedAfterParent)
END_MESSAGE_MAP()

void CDashPage::DoDataExchange(CDataExchange* pDX) {
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_LOG, m_log);
}

BOOL CDashPage::OnInitDialog() {
  CDialogEx::OnInitDialog();
  m_log.SetHorizontalExtent(2000);
  CreateFonts();
  return TRUE;
}

void CDashPage::CreateFonts() {
  const UINT dpi = GetDpiForWindow(m_hWnd);
  theme::CreateUiFont(m_captionFont, 11, FW_BOLD, dpi);
  theme::CreateUiFont(m_labelFont, 9, FW_NORMAL, dpi);
  theme::CreateUiFont(m_valueFont, 17, FW_BOLD, dpi);
  theme::CreateUiFont(m_stepFont, 9, FW_NORMAL, dpi);
  theme::CreateUiFont(m_sectionFont, 9, FW_BOLD, dpi);
  theme::CreateIconFont(m_iconFont, 9, dpi);
  GetDlgItem(IDC_DASH_LOG_LABEL)->SetFont(&m_sectionFont);
}

LRESULT CDashPage::OnDpiChangedAfterParent(WPARAM, LPARAM) {
  CreateFonts();
  Invalidate();
  return 0;
}

// ------------------------------------------------------------ 상태 갱신 (UI 스레드)

void CDashPage::Begin(bool reportOnly) {
  m_state = State::Running;
  m_reportOnly = reportOnly;
  m_step = -1;
  m_phase.Empty();
  m_status.Empty();
  m_done = m_total = m_failures = 0;
  m_start = m_end = std::chrono::steady_clock::now();
  m_log.ResetContent();
  SetTimer(kTimerElapsed, 1000, nullptr);
  Invalidate(FALSE);
}

void CDashPage::SetPhase(const CString& phase) {
  m_phase = phase;
  m_status.Empty();
  // 단계 이름은 "STEP n ..." 형식 (Evaluator.cpp)
  if (phase.Left(5) == L"STEP " && phase.GetLength() > 5 && iswdigit(phase[5])) m_step = phase[5] - L'0';
  m_done = 0;
  m_total = 0;
  Invalidate(FALSE);
}

void CDashPage::SetProgress(size_t done, size_t total, size_t failures) {
  m_done = done;
  m_total = total;
  m_failures = failures;
  Invalidate(FALSE);
}

void CDashPage::SetStatusText(const CString& text) {
  m_status = text;
  Invalidate(FALSE);
}

void CDashPage::Finish(const fpe::RunOutcome& o) {
  KillTimer(kTimerElapsed);
  m_end = std::chrono::steady_clock::now();
  m_status = toCS(o.message);
  if (o.ok) {
    m_state = State::Done;
    m_eer = o.metrics.eer;
    m_fmr100 = o.metrics.fmr100;
    m_fte = o.metrics.fte;
  } else {
    m_state = o.stopped ? State::Stopped : State::Failed;
  }
  Invalidate(FALSE);
}

void CDashPage::AppendLog(const CString& line) {
  const int n = m_log.AddString(line);
  if (n > 5000) m_log.DeleteString(0);
  m_log.SetTopIndex(m_log.GetCount() - 1);
}

int CDashPage::Percent() const {
  if (m_state != State::Running) return -1;
  return m_total ? static_cast<int>(m_done * 100 / m_total) : 0;
}

void CDashPage::OnTimer(UINT_PTR id) {
  if (id == kTimerElapsed) Invalidate(FALSE);
  CDialogEx::OnTimer(id);
}

void CDashPage::OnSize(UINT type, int cx, int cy) {
  CDialogEx::OnSize(type, cx, cy);  // 로그 목록은 AFX_DIALOG_LAYOUT으로 늘어난다
  Invalidate(FALSE);
}

HBRUSH CDashPage::OnCtlColor(CDC* dc, CWnd* wnd, UINT ctlColor) {
  if (wnd->GetDlgCtrlID() == IDC_DASH_LOG_LABEL) {
    dc->SetTextColor(theme::kText);
    dc->SetBkColor(theme::kPanel);
    static CBrush panel(theme::kPanel);
    return panel;
  }
  return CDialogEx::OnCtlColor(dc, wnd, ctlColor);
}

// ------------------------------------------------------------ 그리기

CRect CDashPage::Dlu(int l, int t, int r, int b) {
  CRect rc(l, t, r, b);
  MapDialogRect(&rc);
  return rc;
}

CString CDashPage::Elapsed() const {
  if (m_state == State::Idle) return L"-";
  const auto end = m_state == State::Running ? std::chrono::steady_clock::now() : m_end;
  const long long sec = std::chrono::duration_cast<std::chrono::seconds>(end - m_start).count();
  CString s;
  s.Format(L"%lld:%02lld", sec / 60, sec % 60);
  return s;
}

void CDashPage::DrawCard(CDC& dc, const CRect& rc, const CString& label, const CString& value, COLORREF valueColor) {
  theme::FillRoundRect(&dc, rc, MulDiv(8, GetDpiForWindow(m_hWnd), 96), theme::kCard, theme::kBorder);
  CRect inner(rc);
  inner.DeflateRect(Dlu(7, 5, 7, 5).left, Dlu(7, 5, 7, 5).top);
  dc.SelectObject(&m_labelFont);
  dc.SetTextColor(theme::kTextSecondary);
  dc.DrawText(label, inner, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
  dc.SelectObject(&m_valueFont);
  dc.SetTextColor(valueColor);
  dc.DrawText(value, inner, DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void CDashPage::DrawSteps(CDC& dc, const CRect& rc) {
  const int dpi = static_cast<int>(GetDpiForWindow(m_hWnd));
  const int r = MulDiv(11, dpi, 96);
  const int cy = rc.top + r + 1;
  const int slot = rc.Width() / 6;
  for (int i = 0; i < 6; ++i) {
    const int cx = rc.left + slot * i + slot / 2;
    const bool skipped = m_reportOnly && i < 5;
    const bool done = !skipped && (m_state == State::Done || (m_step > i));
    const bool current = !skipped && !done && m_step == i && m_state != State::Idle;
    if (i < 5) {  // 다음 단계와 잇는 선
      const COLORREF line = done ? theme::kSuccess : theme::kBorder;
      dc.FillSolidRect(cx + r + MulDiv(4, dpi, 96), cy, slot - 2 * r - MulDiv(8, dpi, 96), MulDiv(2, dpi, 96), line);
    }
    CRect circle(cx - r, cy - r, cx + r, cy + r);
    const COLORREF stopColor = m_state == State::Failed ? theme::kDanger : theme::kAccent;
    COLORREF fill = theme::kPanel, border = theme::kBorder, fg = theme::kTextSecondary;
    if (done) {
      fill = border = theme::kSuccess;
      fg = RGB(255, 255, 255);
    } else if (current) {
      fill = border = stopColor;
      fg = RGB(255, 255, 255);
    }
    CBrush brush(fill);
    CPen pen(PS_SOLID, MulDiv(1, dpi, 96), border);
    CBrush* oldBrush = dc.SelectObject(&brush);
    CPen* oldPen = dc.SelectObject(&pen);
    dc.Ellipse(circle);
    dc.SelectObject(oldPen);
    dc.SelectObject(oldBrush);
    dc.SetTextColor(fg);
    if (done) {
      dc.SelectObject(&m_iconFont);
      dc.DrawText(L"\uE73E", circle, DT_CENTER | DT_VCENTER | DT_SINGLELINE);  // CheckMark
    } else {
      dc.SelectObject(&m_stepFont);
      CString num;
      num.Format(L"%d", i);
      dc.DrawText(skipped ? CString(L"–") : num, circle, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    CRect label(cx - slot / 2, cy + r + MulDiv(4, dpi, 96), cx + slot / 2, rc.bottom);
    dc.SelectObject(&m_stepFont);
    dc.SetTextColor(current ? theme::kText : theme::kTextSecondary);
    dc.DrawText(kStepNames[i], label, DT_CENTER | DT_TOP | DT_SINGLELINE);
  }
}

void CDashPage::OnPaint() {
  CPaintDC paintDc(this);
  CRect client;
  GetClientRect(&client);
  CDC dc;
  dc.CreateCompatibleDC(&paintDc);
  CBitmap bmp;
  bmp.CreateCompatibleBitmap(&paintDc, client.Width(), client.Height());
  CBitmap* oldBmp = dc.SelectObject(&bmp);
  CFont* oldFont = dc.SelectObject(&m_captionFont);
  dc.FillSolidRect(client, theme::kPanel);
  dc.SetBkMode(TRANSPARENT);

  const CRect margin = Dlu(10, 6, 10, 0);
  const int left = margin.left, right = client.right - margin.right;

  // 상태 문구
  CString caption;
  COLORREF captionColor = theme::kText;
  switch (m_state) {
    case State::Idle:
      caption = L"대기 중 · 평가 시작 후 진행 상황 표시";
      captionColor = theme::kTextSecondary;
      break;
    case State::Running:
      caption = L"●  " + (m_status.IsEmpty() ? (m_phase.IsEmpty() ? CString(L"준비 중") : m_phase) : m_status);
      captionColor = theme::kAccent;
      break;
    case State::Done:  // 평가 엔진 메시지가 이미 "완료…"로 시작하면 그대로 쓴다
      caption = m_status.Left(2) == L"완료" ? m_status : L"완료 — " + m_status;
      captionColor = theme::kSuccess;
      break;
    case State::Stopped:
      caption = m_status.Left(3) == L"중지됨" ? m_status : L"중지됨 — " + m_status;
      break;
    case State::Failed:
      caption = m_status.Find(L"실패") >= 0 ? m_status : L"실패 — " + m_status;
      captionColor = theme::kDanger;
      break;
  }
  CRect capRc(left, margin.top, right, Dlu(0, 18, 0, 0).top);
  dc.SetTextColor(captionColor);
  dc.DrawText(caption, capRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

  // 수치 카드 4개
  const int gap = Dlu(6, 0, 0, 0).left;
  const int top = Dlu(0, 22, 0, 0).top, bottom = Dlu(0, 58, 0, 0).top;
  const int w = (right - left - 3 * gap) / 4;
  CString v[4], label[4];
  COLORREF color[4] = {theme::kText, theme::kText, theme::kText, theme::kText};
  if (m_state == State::Done) {
    label[0] = L"EER";
    v[0] = percentText(m_eer);
    label[1] = L"FMR100";
    v[1] = percentText(m_fmr100);
    label[2] = L"FTE (등록 실패율)";
    v[2] = percentText(m_fte);
    label[3] = L"소요 시간";
    v[3] = Elapsed();
    color[0] = theme::kAccent;
  } else {
    label[0] = L"현재 단계 진행률";
    label[1] = L"완료 / 전체";
    if (m_total) {
      v[0].Format(L"%zu%%", m_done * 100 / m_total);
      v[1].Format(L"%zu / %zu", m_done, m_total);
    } else {
      v[0] = v[1] = L"-";
    }
    label[2] = L"실패 (CRASH·TIMEOUT 등)";
    v[2].Format(L"%zu", m_failures);
    if (m_failures > 0) color[2] = theme::kDanger;
    label[3] = L"경과 시간";
    v[3] = Elapsed();
  }
  for (int i = 0; i < 4; ++i) {
    DrawCard(dc, CRect(left + i * (w + gap), top, left + i * (w + gap) + w, bottom), label[i], v[i], color[i]);
  }

  // 단계 표시
  DrawSteps(dc, CRect(left, Dlu(0, 64, 0, 0).top, right, Dlu(0, 90, 0, 0).top));

  dc.SelectObject(oldFont);
  paintDc.BitBlt(0, 0, client.Width(), client.Height(), &dc, 0, 0, SRCCOPY);
  dc.SelectObject(oldBmp);
}
