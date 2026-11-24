# 지문인식 알고리즘 v1 (FPAlgoV1)

`c_project` 저장소(github.com/seungbinlee49-alt/c_project, `main` `9887cd5`)의 `algorithm-v1/submission`(`v1-submission-20261123`)을 옮겨 온 것이다. `src/` 11개 파일을 [src/algo_v1/](../../src/algo_v1/)에 그대로 두었고 [manifest.json](manifest.json)의 SHA-256과 같다. 빌드만 이 저장소의 CMake(`/MT` 정적 런타임)로 바꿨다.

2026-11-24 이전 판(`1d9b868`, `algorithm-v1-20261114`)에서 바뀐 점: 추출·정합 규칙과 점수는 같고(최대 차이 1e-17 미만), 템플릿을 가변 길이 v2 형식(평균 약 110KB, FNV 체크섬)으로 줄였으며 비교가 약 2.2배 빨라졌다. `FRFXLL.dll`은 링크하지 않고 `FPAlgoV1.dll`과 같은 폴더에서 추출 시점에 연다. iType은 1·2·3만 받는다(이전 판의 5·7·8·9 제거). 오류 코드는 c_project의 [submission README](https://github.com/seungbinlee49-alt/c_project/tree/main/algorithm-v1/submission)를 따른다. 템플릿 형식이 바뀌었으므로 이전 판으로 만든 결과 폴더는 이어하기 하지 않는다.

## 구성

| 단계 | 위치 |
|---|---|
| DLL 진입점 `KISA_Enroll`·`KISA_Attempt`·`KISA_Compare`, iType → 크기, 오류 코드 | [algorithm.cpp](../../src/algo_v1/algorithm.cpp), [api.h](../../src/algo_v1/api.h), [errors.h](../../src/algo_v1/errors.h) |
| FRFXLL.dll 동적 로드 (DLL 자신의 폴더 기준) | [fingerjet_loader.h](../../src/algo_v1/fingerjet_loader.h) |
| 1/99 백분위 대비 정규화 | [photometric.h](../../src/algo_v1/photometric.h) |
| 특징점 추출: FingerJetFX OSE (`FRFXLL.dll`, 외부) | [third_party/FingerJetFXOSE/](../../third_party/FingerJetFXOSE/) |
| 특징점 주변 8×8×6 기술자, 강체 정렬 | [minutiae.h](../../src/algo_v1/minutiae.h) |
| 거리·방향 관계 일관성 그래프 정합 | [minutiae_graph.h](../../src/algo_v1/minutiae_graph.h) |
| 볼록껍질 겹침 보정 (최대 2배) | [minutiae_overlap.h](../../src/algo_v1/minutiae_overlap.h) |
| 원 영상 방향장 검증 | [minutiae_orientation.h](../../src/algo_v1/minutiae_orientation.h) |
| 템플릿 v2 직렬화·체크섬 | [template_codec.h](../../src/algo_v1/template_codec.h) |

빌드 정의는 `FP_THREE_DECIMAL_SCORE`다. 평가 프로그램의 소수점 3자리 검사에 맞춘 평가용 빌드(c_project의 `fingerprint_algorithm_eval.dll`)만 플러그인으로 낸다.

## 빌드와 실행

`build.ps1`이 x86·x64 각각 `bin/plugins/{x86,x64}/FPAlgoV1.dll`과 `FRFXLL.dll`을 만든다. 두 파일은 같은 폴더에 있어야 한다. 평가 프로그램에서 DLL로 `FPAlgoV1.dll`을 고르면 된다. iType은 FPAlgo와 같다(1=448×478, 2=300×300, 3=640×480).

```bash
bin/FPEvalCli.exe --config testdb/base.json --dll bin/plugins/x64/FPAlgoV1.dll --db testdb/FVC2002_DB3_B --itype 2 --out results
```

단독 호출 예제: `build/x64/tools/AlgoV1Compare.exe <dll> <iType> enroll.raw attempt.raw` (헤더 없는 8비트 RAW).

- 템플릿: 가변 길이 v2 (c_project 측정 평균 109,946 / 최대 208,296바이트, 한도 1,000,000바이트 미만)
- 템플릿 형식이 FPAlgo(FPT1)와 다르므로 두 DLL의 결과 폴더를 섞어 이어하기 하지 않는다.

## 이 저장소 평가기 결과 (2026-11-21, 2026-11-24 새 판으로 재실행해 같은 값 확인)

`FPEvalCli`, `testdb/base.json`(등록 All, 본인 Symmetric 280쌍, 타인 FirstOne 대칭 제거 45쌍), seed 20261104, workers 4, timeout 10초.

| DB (iType) | x64 EER | FMR100 | FTE | 비교: FPAlgo(MCC) EER |
|---|---:|---:|---:|---:|
| FVC2000_DB3_B (1) | 2.90% | 3.57% | 0% | 9.09% |
| FVC2002_DB3_B (2) | 0.00% | 0.00% | 0% | 3.25% |
| FVC2004_DB1_B (3) | 4.33% | 7.14% | 0% | 7.80% |

- x86 FVC2002 결과도 x64와 같았다. 네 실행 모두 325개 작업이 OK이고 실패는 0이었다.
- 타인 쌍이 DB당 45개뿐이라 해상도가 낮다. FMR100과 FMR1000이 같은 값으로 나온다.
- FPAlgo 수치는 [알고리즘_개선_결과.md](../알고리즘_개선_결과.md)의 같은 규칙 결과다(`feature/algo-mcc` 브랜치).

## c_project 자체 벤치 결과

DB별 10지문 × 8촬영, 첫 촬영 등록, 조건별 본인 70쌍·타인 720쌍. 위 표와 규칙이 다르다.

- 원본 EER: FVC2000 1.409%, FVC2002 1.548%, FVC2004 4.226%
- 원본·Gaussian15·impulse2%·Gaussian30·밝기·blur1의 18조건 평균 EER 3.019%, 최악 5.913%. 출처: [results.csv](results.csv), 3자리 반올림판은 [evaluation-results.csv](evaluation-results.csv)
- 미사용 FVC2000 DB1·DB2 160장: 원본 EER 1.409%·4.296%. 개발 때 고정한 임계값의 FMR은 2.361%·2.500%로 1%를 넘었다. [external-validation.csv](external-validation.csv)

튜닝과 평가에 같은 개발 DB를 썼으므로 독립 검증 성능이 아니다.

## 외부 라이브러리 기록

| 항목 | 내용 |
|---|---|
| 라이브러리 | FingerJetFX OSE `libFRFXLL` (특징점 추출만 사용) |
| 출처 | https://github.com/FingerJetFXOSE/FingerJetFXOSE |
| 라이선스 | LGPL-3.0-or-later + COPYRIGHT.txt 조건 ([NOTICE.md](../../NOTICE.md)) |
| 필요성 | v1의 정합은 FingerJetFX 특징점을 전제로 만들어졌다. 직접 구현한 추출기(FPAlgo)로 바꾸면 다른 알고리즘이 된다 |
| 배포 영향 | `FRFXLL.dll`을 별도 교체 가능한 DLL로 배포한다. 설치 패키지에 NOTICE와 라이선스 원문을 포함한다. 평가 프로그램(GUI·CLI·Worker)은 이 라이브러리에 의존하지 않는다 |

저장소에는 `libFRFXLL/include`, `libFRFXLL/src`와 라이선스 파일만 원본 그대로 두었다. 샘플·테스트 데이터는 제외했다.

## 이전 버전 기록

c_project에서 v1에 이르기까지의 기록을 [history/](history/)에 보관한다. 빌드 대상은 아니다.

| 버전 | 소스 (history/src) | 결과 문서 |
|---|---|---|
| baseline: 셀 방향 히스토그램 | `baseline.cpp` | [experiment-results.md](history/experiment-results.md) |
| aligned: 방향장 + 회전·이동 검색 + 질감 | `improved.cpp` | 같은 문서 |
| minutiae-v2: FingerJetFX + 기술자 정합 | `minutiae_algorithm.cpp`, `minutiae.h` | [minutiae-v2-results.md](history/minutiae-v2-results.md) |
| graph-v1: v2 + 관계 일관성 | `minutiae_graph.h` (`FP_GRAPH_MATCHER`) | [graph-v1-results.md](history/graph-v1-results.md), [graph-v1-paired-results.md](history/graph-v1-paired-results.md) |

전체 실행 요약은 [summary.csv](history/summary.csv), 프로토콜 고정 파일은 [history/protocol/](history/protocol/)에 있다. c_project 자체 평가 프로그램(`fp_bench`, `fp_gui` 등)은 FPEval과 기능이 겹쳐 옮기지 않았다. 원본은 위 GitHub 저장소에 있다.
