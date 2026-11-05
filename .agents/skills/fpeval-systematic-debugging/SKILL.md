---
name: "fpeval-systematic-debugging"
description: "Investigate reproducible or intermittent Fingerprint-Evaluator failures before fixing them. Use for unexpected statuses, crashes, timeouts and failed tests; route known build or ABI problems to their specialized skills."
license: "MIT"
---

# 증거 기반 원인 추적

## 원문에서 적용한 흐름

재현·경계별 증거 수집 → 정상 사례와 차이 확인 → 한 가지 가설 시험 → 원인 수정·재검증 순서로 진행한다.

1. 정확한 명령, DB·설정·seed·DLL 해시·아키텍처, 기대 결과와 실제 상태를 기록한다. 사용자 결과를 덮어쓰지 않는 전용 `results/` 실행을 만든다.
2. 최초 실패에서 역으로 추적한다. CLI/GUI → Evaluator → Scheduler → WorkerProcess → Worker → DLL → 로그 경계 중 어디에서 값·상태가 달라졌는지 확인한다.
3. `Dummy_OK.dll`의 같은 입력과 비교한다. 동시성 의심이면 Worker 1개와 문제의 개수로 비교한다. 시간 문제는 timeout을 올려 숨기기 전에 연결·Hello·실제 호출 중 어느 구간인지 분리한다.
4. 근거를 붙여 단일 가설을 세우고 한 변수를 바꿔 시험한다. 재현되지 않으면 필요한 로그·스택·입력을 더 수집하고 원인 확정을 유보한다.
5. 원인에 맞는 작은 수정과 최소 회귀 사례를 적용한다. DLL 오류 검출은 CLI 종료 코드가 아니라 작업별 CSV 상태로 판단한다.

| 시험 DLL | 입력 조건이 충족될 때 기대 상태 |
| --- | --- |
| Bad_Crash | CRASH |
| Bad_Timeout | TIMEOUT |
| Bad_Score | MISMATCH 또는 FAIL |
| Bad_Size | MISMATCH 또는 OVERFLOW |

오류 선택은 입력 해시에 의존한다. `pairs_sample.csv`에서 안 나오면 입력을 넓힌다. Bad_Random도 현재 결정적 해시 점수이며 EER 정확히 50%를 기대하지 않는다.

연속된 가설이 실패하면 같은 수정을 반복하지 말고 경계·입력·가정부터 다시 조사한다. 원문의 고정 횟수 후 승인 요구와 강제 코드 삭제는 적용하지 않는다. 진단 대상 DLL은 항상 Worker에서 실행한다.

완료 기록: 재현 → 원인 증거 → 수정 → 같은 사례의 결과. 미확인 원인과 환경 제약을 분리한다.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [obra/superpowers/systematic-debugging](https://github.com/obra/superpowers/blob/22f0b463e7a5e579904dd6211a520321035526f7/skills/systematic-debugging/SKILL.md) · commit `22f0b463e7a5` · [원문 라이선스](../../../docs/skill-sources/licenses/obra--superpowers.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
