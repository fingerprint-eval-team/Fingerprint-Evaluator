---
name: "fpeval-mfc-ui"
description: "Implement and validate Fingerprint-Evaluator native MFC dialogs, resource/DDX bindings, Korean text, DPI, keyboard navigation and run-state UI. Not a web frontend, WPF or Electron migration skill."
license: "Apache-2.0"
---

# MFC 화면·키보드·상태 검증

## 구현 경계

`FPEval.rc`, `resource.h`, `Pages.cpp`, `MainDlg.cpp`, `ProgressBridge.cpp`를 읽고 변경할 컨트롤의 ID → DDX → 메시지 핸들러 → 상태 갱신을 추적한다. Core에 UI 의존성을 추가하지 않는다.

1. 실제 대기·실행·완료 화면에서 시작한다. 오류·중지·이어하기에 영향이 있으면 그 화면도 포함한다. 현재 창 크기·배율과 긴 한글 경로를 기록한다.
2. 읽기 순서와 Tab 순서를 맞추고 시작·중지·강제 종료·결과 열기의 의미를 분명히 한다. 배치 수정으로 ID·기본 버튼·포커스·설정 잠금이 바뀌지 않는지 확인한다.
3. 가용한 Windows UI 도구의 현재 지침을 먼저 읽는다. 원문의 agent-desktop CLI가 설치되어 있다고 가정하거나 설치하지 않는다. 앱·창을 관찰한 뒤 정확한 컨트롤에 동작하고 새 상태로 결과를 확인한다.
4. 최소화·가려짐·권한 경계·창 교체를 구분한다. 자동화가 실패했다고 앱 결함으로 확정하지 않는다. 상태를 바꾸는 버튼은 결과를 확인하지 않고 반복 클릭하지 않는다.
5. 100%·125%·150%에서 라벨·경로·버튼 잘림, Tab/Shift+Tab·포커스·키보드 실행을 확인한다. 배율 조작이 불가능하면 확인한 범위를 기록한다. 설정 변경 검증이 필요하면 DDX 값을 실제 저장/재로드까지 확인한다.
6. 작업 스레드에서 컨트롤에 직접 접근하지 않는다. PostMessage 페이로드의 송신 실패·수신 삭제·창 종료·작업 완료 시 수명을 함께 검토한다.

원문의 Windows 관찰→동작→재관찰 원칙을 적용했고 도구별 명령·플랫폼 설치 지침은 제외했다. MFC 설계는 [Microsoft 스레드 지침](https://learn.microsoft.com/en-us/cpp/mfc/multithreading-programming-tips)을 따른다.

완료 기록: 바뀐 UI 흐름, 실행 화면·배율·키보드·상태별 결과, 미확인 항목. 화면 캡처는 한글·정렬 검증에 쓰고 빌드 성공으로 실제 조작을 대신하지 않는다.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [vercel-labs/agent-desktop/agent-desktop-windows](https://github.com/vercel-labs/agent-desktop/blob/6660fbac768dc7e7332837b90ed465684859afa3/skills/agent-desktop-windows/SKILL.md) · commit `6660fbac768d` · [원문 라이선스](../../../docs/skill-sources/licenses/vercel-labs--agent-desktop.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
