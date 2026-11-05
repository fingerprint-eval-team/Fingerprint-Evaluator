---
name: "fpeval-property-testing"
description: "Design deterministic property and regression tests for Fingerprint-Evaluator RNG, pair rules, JSON/CSV and template serialization. Use for algebraic invariants and minimized counterexamples, not UI integration or coverage-guided fuzzing."
license: "CC-BY-SA-4.0"
---

# 속성·회귀 테스트

## 속성 선택

Trail of Bits의 왕복·불변식·독립 기준 구현·반례 축소와 Superpowers의 실패 사례 확인 → 수정 → 재검증 흐름을 적용한다. 외부 테스트 라이브러리 없이 기존 `tests/CoreTests.cpp`의 CHECK/NEAR와 결정적 입력 생성부터 쓴다.

| 대상 | 검증할 속성 |
| --- | --- |
| Rng·EvalPlan | 같은 seed·입력에서 동일 계획, 독립 하위 seed, 동일 이미지 비교 없음 |
| 조합 규칙 | G/I의 손가락 관계, 대칭 제거·중복 제거, 작은 DB의 독립 전수 조합과 일치 |
| JSON·EvalConfig | 지원 값의 의미 보존 왕복, 누락 기본값, 큰 수·잘못된 값 처리 |
| CSV | 쉼표·인용부호·빈 필드 왕복. 줄바꿈 지원 여부는 실제 계약으로 분리 |
| 템플릿 | 유효 헤더·개수·크기, 좌표·각도의 직렬화 정밀도 내 의미 보존 |

1. 실패해야 할 구체 결함을 먼저 적고 기대값을 구현과 독립적으로 만든다. 같은 알고리즘을 복제한 기대값, `f(x)==f(x)`만 있는 순수함수 검사는 피한다.
2. 정상·경계·잘못된 입력을 나눈다. 대부분을 필터링해서 실제 검사 건수가 0이 되는 생성기를 피한다. seed·실제 검사 수·실패 입력을 남긴다.
3. 반례가 생기면 이미지 수, 쌍 수, 문자열, 숫자를 줄여 최소 사례로 고정한다. 구현 결함과 잘못 가정한 속성을 구분한다. 등록/비교의 비대칭 때문에 매처 대칭성을 무조건 요구하지 않는다.
4. 결함 수정은 가능하면 수정 전 실패와 수정 후 성공을 확인한다. 기존 사용자 코드를 삭제해 TDD 순서를 맞추지 않는다. 의미 없는 테스트를 만드는 대신 관찰 가능한 계약을 검증한다.
5. 규칙 변경은 기본 FVC형 10×8 입력의 560/280 본인, 5760/90/45 타인 조합을 관련 설정에 맞춰 확인하고 전체 CoreTests를 실행한다.

템플릿·알고리즘 코드는 현재 CoreTests 링크 대상에 자동 포함되지 않는다. 필요하면 해당 소스를 별도 테스트 target에 연결하는 최소 변경을 검토한다. GoogleTest·Hypothesis가 설치되어 있다고 가정하지 않는다.

완료 기록: 속성, 생성 범위·seed·검사 수, 최소 반례, 실제 실행 결과. 이 적응본은 CC-BY-SA-4.0이며 원저자·변경 내역은 아래 출처에 기록한다.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [trailofbits/skills/property-based-testing](https://github.com/trailofbits/skills/blob/711af095215ad4ecde5b90cd1d0124b3f1c4ca9e/plugins/property-based-testing/skills/property-based-testing/SKILL.md) · commit `711af095215a` · [원문 라이선스](../../../docs/skill-sources/licenses/trailofbits--skills.txt)
- [obra/superpowers/test-driven-development](https://github.com/obra/superpowers/blob/22f0b463e7a5e579904dd6211a520321035526f7/skills/test-driven-development/SKILL.md) · commit `22f0b463e7a5` · [원문 라이선스](../../../docs/skill-sources/licenses/obra--superpowers.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
