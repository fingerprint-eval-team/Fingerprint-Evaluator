---
name: "fpeval-chart-validation"
description: "Validate Fingerprint-Evaluator DET, score distributions and FMR/FNMR charts for truthful axes, normalization, vector output and SVG/EMF consistency. Use for ChartRenderer or canvas changes, not EER formula changes alone."
license: "MIT"
---

# 과학적 그래프·SVG·EMF 검증

## 데이터에서 출력까지

`MetricsResult` → `ChartRenderer` → `ICanvas` → `SvgCanvas/EmfCanvas`를 추적한다. 원문의 축·단위·변환 보존과 출력 파일 확인을 자체 C++ 벡터 렌더러에 적용한다.

1. 그래프 원본 수치, 분모, 축 변환, 표시 범위, bin, 선택 항목을 적는다. 현재 DET는 log-log다. 관례적인 다른 DET 변환으로 바꾸려면 요구사항과 기존 출력 호환성을 별도로 검토한다.
2. 로그축에서 0을 표시 하한으로 옮기는 것은 시각화 처리다. 원 지표를 양수로 바꾸거나 0·미정의·결측을 같은 값으로 취급하지 않는다. 빈 데이터가 정상 성능 곡선처럼 보이지 않게 한다.
3. G/I 히스토그램 분모와 마지막 bin의 점수 1 처리를 확인한다. FMR/FNMR의 단조성·범례·임계값 방향, EER 표시와 FMR100/1000 기준선이 수치와 일치하는지 본다.
4. 같은 데이터의 SVG/EMF에서 좌표 방향·여백·clip·색·대시·문자 정렬·한글·단위를 대조한다. `ICanvas` 공용 의미를 유지하고 한 backend만 다른 축을 사용하지 않는다.
5. 실제 리포트 크기와 확대 상태에서 폰트·축·범례 잘림을 확인한다. 색뿐 아니라 라벨·선 모양으로 G/I를 구분한다. 비교 그림은 동일한 축 또는 차이를 밝힌 축을 사용한다.
6. 벡터 파일 안에 전체 차트를 bitmap으로 넣어 요구사항을 우회하지 않는다. HTML의 SVG 참조와 DOCX의 EMF 관계도 확인한다.

원문의 Matplotlib/Plotly·저널별 DPI·논문 제출 워크플로는 기본 적용하지 않는다. 여기서는 SVG/EMF가 제출 산출물이다. 완료 기록에는 변환·0 처리, 데이터 대조, 두 형식의 실제 보기 결과와 한계를 적는다.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [K-Dense-AI/scientific-agent-skills/scientific-visualization](https://github.com/K-Dense-AI/scientific-agent-skills/blob/a25cf9ec00e2745d03f8704acf6f6919694ccd9a/skills/scientific-visualization/SKILL.md) · commit `a25cf9ec00e2` · [원문 라이선스](../../../docs/skill-sources/licenses/K-Dense-AI--scientific-agent-skills.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
