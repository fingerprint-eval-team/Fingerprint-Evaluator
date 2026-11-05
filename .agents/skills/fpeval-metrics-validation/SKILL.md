---
name: "fpeval-metrics-validation"
description: "Validate Fingerprint-Evaluator FTE, REJ, FMR/FNMR, EER/EERstar, threshold selection, denominators and time/size aggregation using independent small examples. Excludes algorithm tuning and cosmetic chart edits."
license: "CC-BY-SA-4.0"
---

# 지문 평가 수식 검증

## 수치 기준

`MetricsCalculator.cpp/.h`, `Status.h`, `ResultStore`와 통합 문서의 수식을 대조한다. 외부 속성 테스트의 독립 oracle·경계 입력을 이 프로젝트 산식에 적용한다. 구현을 복사한 계산기로 기대값을 만들지 않는다.

1. 계획된 G/I, 실제 호출, 성공 점수, 등록 실패·Attempt 실패·호출 실패를 별도 집계한다. SKIP_EF와 SKIP_AF는 다르며 ABORTED는 완료 평가 결과가 아니다.
2. 기본 분모에서 FMR(t)는 타인 성공 점수 `>=t` 건수 / 계획 타인 수, FNMR(t)는 본인 등록·비교 실패 및 성공 점수 `<t` 건수 / 계획 본인 수다. `attempted`는 현재 등록 실패분을 제외하며 실제 호출 수와 같다고 가정하지 않는다.
3. EER*는 OK 점수만 사용한다. FVC와 Nearest 방법의 선택·동점·교차 없음·정확한 교차를 따로 확인한다. 후보에는 점수 고유값과 0, 1보다 큰 끝점이 포함되며 그래프의 0.001 grid와 혼동하지 않는다.
4. 독립 소형 예: G=[0.8,0.9], I=[0.1,0.2], 실패 없음에서 t=0.5의 FMR=FNMR=0. G=[0.2,0.8], I=[0.2,0.8]에서 t=0.5는 각각 0.5다. t=0.2에서는 `>=`/`<` 규칙에 따라 FMR=1, FNMR=0이다.
5. G 성공 1건(0.8)+SKIP_EF 1건, 계획 G=2, I 성공 1건(0.1), 계획 I=1인 사례에서 t=0.5의 기본 FNMR=0.5, attempted FNMR=0, FMR=0을 확인한다. SKIP_AF를 대입하면 등록 실패 제외와 같은 처리가 되지 않아야 한다.
6. 빈 G/I, 전부 실패, 0/1 동점, 정의 불가 ZeroFNMR, 비정상 점수 유입을 검토한다. 분모 0에 대한 현재 반환과 사용자 표시를 구분하며, 미정의 값을 정상 성능으로 해석하지 않는다.
7. 유효 입력에서 FMR 비증가·FNMR 비감소, [0,1] 범위, 기록 순서 무관성을 검사한다. 시간은 OK 건 `elapsedUs`를 ms로 바꾸고 크기는 등록 OK 기준인지 확인한다.

완료 기준: 수식·옵션별 손계산 사례, 해당 CoreTests, 같은 저장 로그의 HTML/DOCX 수치 일치. 알고리즘 점수 자체가 바뀌면 `fpeval-algorithm-experiments`를 추가한다. 이 적응본의 라이선스는 CC-BY-SA-4.0이다.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [trailofbits/skills/property-based-testing](https://github.com/trailofbits/skills/blob/711af095215ad4ecde5b90cd1d0124b3f1c4ca9e/plugins/property-based-testing/skills/property-based-testing/SKILL.md) · commit `711af095215a` · [원문 라이선스](../../../docs/skill-sources/licenses/trailofbits--skills.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
