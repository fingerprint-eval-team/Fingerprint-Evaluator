---
name: "fpeval-algorithm-experiments"
description: "Plan and compare Fingerprint-Evaluator FPAlgo preprocessing, minutiae, matching or parameter changes on FVC data with controlled baselines. Use for accuracy experiments and tuning, not formula correctness or CPU profiling alone."
license: "MIT"
---

# 인식 알고리즘 실험

## 실험 설계

측정 전에 변경 인자, 주 지표, 기준 버전, 비교할 DB·규칙을 적는다. 원문의 통제·반복·의존 표본 구분을 지문 실험에 적용하며 과학 Python 패키지나 외부 서비스는 기본 의존성으로 설치하지 않는다.

1. 변경 전 DLL과 실행 결과를 보관한다. 소스 commit·미커밋 diff, DLL SHA-256, 빌드 구성·아키텍처, DB 이미지 수·크기, iType, 규칙·seed·쌍 파일, Worker·timeout, EER 방식·분모를 기록한다.
2. 한 번에 비교 목적에 맞는 인자를 바꾼다. 전처리·미뉴셔·매칭을 동시에 바꿨으면 효과를 분리할 수 없는 비교라고 적는다. 튜닝에 반복 사용한 DB 결과를 독립 검증 성능으로 부르지 않는다.
3. 아래 기준 명령을 실행하고 후보는 `--out results/experiment-after`로 바꾼다. `--config`는 override보다 먼저 둔다. 실제 실행 폴더는 CLI 출력으로 확인한다.

```powershell
& .\bin\FPEvalCli.exe --config testdb/base.json --dll bin/plugins/x64/FPAlgo.dll `
  --db testdb/FVC2002_DB3_B --itype 2 --seed 20261104 --workers 4 --timeout 5 --out results/experiment-before
if ($LASTEXITCODE -ne 0) { throw "Evaluation failed: $LASTEXITCODE" }
```

4. FVC2000/2002/2004에서 같은 비교를 한다. FPAlgo의 iType은 1/2/3이며 마지막 값은 임시다. 외부 DLL에는 해당 제공자의 매핑을 사용한다.
5. FTE·REJ·EER·EER*·쌍 수·상태 분포·시간·크기를 비교한다. 실패를 누락해 EER이 낮아진 경우 개선으로 판정하지 않는다. 알고리즘만 바꿨다면 계획은 같아야 한다. 규칙을 바꿨다면 의미상 이미지 쌍으로 대응시킨다.
6. 같은 손가락의 이미지·매칭 쌍은 독립 표본이 아니다. 동일 결정적 실행 반복은 정확도 표본을 늘리지 않는다. 신뢰구간·유의성 요청은 손가락/DB 의존 구조와 재표집 단위를 먼저 정하고, 단순 쌍별 t-test를 자동 적용하지 않는다.

부분 쌍은 smoke test다. 전체 성능 주장은 선언한 전체 규칙 결과로 제한한다. FVC 데이터가 없으면 수치 실험 미실행을 기록한다. 원본·캐시·템플릿은 로컬에 보관한다.

완료 기록: 고정 조건, 의도된 차이, DB별 전후 표, 실패·시간의 부작용, 탐색적 튜닝 여부. 실험 계획과 통계 절차의 원저자는 K-Dense이며 출처 문서에 기록한다.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [K-Dense-AI/scientific-agent-skills/experimental-design](https://github.com/K-Dense-AI/scientific-agent-skills/blob/a25cf9ec00e2745d03f8704acf6f6919694ccd9a/skills/experimental-design/SKILL.md) · commit `a25cf9ec00e2` · [원문 라이선스](../../../docs/skill-sources/licenses/K-Dense-AI--scientific-agent-skills.txt)
- [K-Dense-AI/scientific-agent-skills/statistical-analysis](https://github.com/K-Dense-AI/scientific-agent-skills/blob/a25cf9ec00e2745d03f8704acf6f6919694ccd9a/skills/statistical-analysis/SKILL.md) · commit `a25cf9ec00e2` · [원문 라이선스](../../../docs/skill-sources/licenses/K-Dense-AI--scientific-agent-skills.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
