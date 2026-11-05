---
name: "fpeval-concurrency"
description: "Diagnose or modify Fingerprint-Evaluator worker-pool races, deadlocks, overlapped I/O cancellation, stop/abort and GUI thread lifetime. Use for synchronization changes; persisted resume correctness has its own skill."
license: "MIT"
---

# 동시성·취소·자원 수명

## 절차

`EvalScheduler`, `WorkerProcess`, `Evaluator`, `ResultStore`, `MainDlg`·`ProgressBridge`에서 변경과 연결되는 경로를 읽는다.

1. 공유 상태별 쓰는 스레드, 읽는 스레드, mutex/atomic, 수명을 표로 정리한다. 컨테이너 변경과 순회, callback과 파괴, `sched_`와 stop/abort 경합을 확인한다.
2. 교착은 모든 관련 스레드의 스택과 소유·대기 락으로 설명한다. 락을 잡은 상태의 UI 호출·I/O 대기·join·재진입 callback을 확인한다.
3. atomic 한 번의 읽기/쓰기가 여러 단계의 check-then-act를 원자적으로 만들지는 않는다. 공유 데이터 공개에는 실제 happens-before 관계가 필요하다. `volatile`나 임의 sleep을 동기화 수정으로 쓰지 않는다.
4. overlapped 요청 취소 시 버퍼·OVERLAPPED·이벤트를 작업 완료 확인 전 해제하지 않는다. 타임아웃·Crash 후 Worker 재기동과 Job Object 종료 경로를 확인한다.
5. 정상 중지는 새 작업 배분을 막고 진행 중 작업을 마친다. 강제 종료는 취소와 Worker 종료를 동반한다. GUI 창 닫기·완료 메시지·join의 순서를 함께 검토한다.
6. 같은 결정적 입력을 Worker 1개와 4/8개로 실행하고 상태·점수를 작업 ID 기준으로 비교한다. 시간과 기록 순서는 일치 기준에서 제외한다. 수정 경계에서 중지·강제 종료를 반복해 종료 지연·잔존 Worker·중복 기록을 확인한다.

원문의 GDB/Helgrind/TSan 명령은 이 Windows MSVC 프로젝트의 기본 도구가 아니다. 설치된 VS/WinDbg의 스레드·스택 기능을 사용하고, 제공되지 않는 TSan 지원을 가정하지 않는다. 검증된 필요 없이 lock-free 구조나 relaxed ordering으로 바꾸지 않는다.

완료 기록: 충돌하는 접근 또는 대기 고리, 수정한 동기화 관계, 재현 횟수·구성·남은 한계. 반복 실행 무사고만으로 race가 없다고 단정하지 않는다.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [mohitmishra786/low-level-dev-skills/concurrency-debugging](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/debuggers/concurrency-debugging/SKILL.md) · commit `bdc58472fa9f` · [원문 라이선스](../../../docs/skill-sources/licenses/mohitmishra786--low-level-dev-skills.txt)
- [mohitmishra786/low-level-dev-skills/memory-model](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/low-level-programming/memory-model/SKILL.md) · commit `bdc58472fa9f` · [원문 라이선스](../../../docs/skill-sources/licenses/mohitmishra786--low-level-dev-skills.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
