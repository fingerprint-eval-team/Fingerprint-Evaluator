---
name: "fpeval-resume-validation"
description: "Validate Fingerprint-Evaluator saved JSON/CSV compatibility, interrupted-run resume, retry deduplication and template persistence. Use for ResultStore, EvalConfig, EvalPlan serialization or resume behavior."
license: "MIT"
---

# 저장·재개·설정 호환성

## 비교 설계

전용 기준 실행과 중단 실행을 준비한다. DB, DLL 해시, iType, seed, 규칙, 지표 옵션을 동일하게 유지한다. 외부 `templateDir`를 썼다면 실행 폴더 밖의 템플릿 위치도 기록한다.

1. `config.json`, `run_info.json`, `plan.csv`, 두 로그와 캐시·템플릿의 역할을 읽는다. 재개에서 계획을 새로 섞거나 DB 이름만으로 동일 입력이라고 가정하지 않는다.
2. 추출 중과 비교 중에서 관련 중단 경로를 재현한다. GUI `중지`, GUI `강제 종료`, CLI Ctrl+C를 구분한다.
3. `FPEvalCli.exe --resume "<실제 실행 폴더>" --workers 4 --timeout 5`처럼 실행 옵션을 명시한다. 현재 구현은 저장 설정의 Worker 수와 timeout을 현재 CLI 값으로 덮어쓴다.
4. `ResultStore::load`와 같은 규칙으로 각 로그의 `jobId`별 마지막 행을 사용한다. `isFinal`은 ABORTED·INTERNAL을 완료로 보지 않는다. 실패를 포함한 다른 final 상태의 재시도 정책을 임의로 바꾸지 않는다.
5. 기준과 재개 결과의 완료 ID 집합·추출 상태·비교 상태·점수·지표를 대조한다. 원시 행 수가 같을 필요는 없지만 누락·이중 집계는 없어야 한다. 시간값은 비교에서 제외한다.
6. 직렬화 변경이면 기존 저장 예시를 읽고 새 값의 왕복·누락 키 기본값·잘못된 입력을 확인한다. CSV 인용부호·빈 필드·한글, JSON 큰 seed의 숫자 정밀도, 끊긴 마지막 행을 확인한다. 허용/거부 방침은 명시하고 결함을 새 계약으로 고정하지 않는다.

관련 코드: `src/core/Evaluator.cpp`, `config/EvalConfig.cpp`, `config/Json.cpp`, `plan/EvalPlan.cpp`, `result/ResultStore.cpp`, `common/Status.h`.

외부 스킬의 재현·증거 검증을 저장 데이터 계약에 적용한 프로젝트 특화 절차다. 기존 실행에서 DLL·DB를 바꿔 재개하지 않는다. 완료 기록은 중단 지점, 저장/현재 옵션, 누락·중복·상태 차이, 데이터 호환성이다.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [obra/superpowers/systematic-debugging](https://github.com/obra/superpowers/blob/22f0b463e7a5e579904dd6211a520321035526f7/skills/systematic-debugging/SKILL.md) · commit `22f0b463e7a5` · [원문 라이선스](../../../docs/skill-sources/licenses/obra--superpowers.txt)
- [obra/superpowers/verification-before-completion](https://github.com/obra/superpowers/blob/22f0b463e7a5e579904dd6211a520321035526f7/skills/verification-before-completion/SKILL.md) · commit `22f0b463e7a5` · [원문 라이선스](../../../docs/skill-sources/licenses/obra--superpowers.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
