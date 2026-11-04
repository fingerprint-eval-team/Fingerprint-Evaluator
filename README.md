# Fingerprint-Evaluator

지문인식 알고리즘 성능 평가 프로그램과 K-NBTC 규격 지문인식 DLL.
고급객체지향프로그래밍(SME3007) 2026-2학기 팀프로젝트.

> **소스 코드는 `develop` 브랜치에 있다.** `main`에는 발표·제출 시점의 안정 버전만 병합한다.
>
> ```bash
> git clone -b develop https://github.com/fingerprint-eval-team/Fingerprint-Evaluator.git
> ```
>
> 이미 클론했다면 `git switch develop`

## 개발 대상

| 산출물 | 설명 |
|---|---|
| 지문인식 알고리즘 평가 프로그램 | Windows GUI(MFC). 알고리즘 DLL과 테스트 DB를 선택해 인식 성능을 평가하고 리포트(HTML·Word, 벡터 그래프)를 생성 |
| 지문인식 알고리즘 | Windows DLL. K-NBTC 지문인식 성능시험 규격(`KISA_Enroll`, `KISA_Attempt`, `KISA_Compare`) 호환 |
| 설치 패키지 | 평가 프로그램 배포용 설치 파일 |

- 언어: C/C++ (외부 라이브러리 없음, Windows SDK·MFC만 사용)
- 시험 DB: FVC2000_DB3_B, FVC2002_DB3_B, FVC2004_DB1_B
- 평가 지표: FTE, FMR/FNMR, EER, EER*, ZeroFMR, ZeroFNMR, FMR100, FMR1000, 처리 시간, 템플릿 크기

### 빌드 산출물

| 파일 | 설명 |
|---|---|
| `bin/FPEval.exe` | 평가 프로그램 GUI (MFC 대화상자, x64) |
| `bin/FPEvalCli.exe` | 같은 평가 엔진의 콘솔 버전 (자동 테스트·일괄 평가) |
| `bin/FPWorker32.exe`, `FPWorker64.exe` | 평가 대상 DLL을 실행하는 격리 프로세스 |
| `bin/plugins/{x86,x64}/FPAlgo.dll` | 우리 팀 지문인식 DLL |
| `bin/plugins/{x86,x64}/Dummy_OK.dll`, `Bad_*.dll` | 오류 감지 검증·시연용 DLL |

## 빌드

Visual Studio 2022 이상(C++ 데스크톱 개발 + **C++ MFC** 구성 요소)이 필요하다. CMake는 VS에 포함된 것을 사용한다.
MFC가 없으면 Visual Studio Installer → 수정 → 개별 구성 요소에서 "MFC"를 검색해 설치한다
(구성 요소 ID `Microsoft.VisualStudio.Component.VC.ATLMFC`). 설치 전에 Visual Studio와 빌드를 모두 닫아야 한다.

```bash
powershell -ExecutionPolicy Bypass -File build.ps1
```

x86(Worker32, 32bit DLL) → x64(GUI, CLI, Worker64, 64bit DLL, 테스트) 순서로 빌드하고 결과를 `bin/`에 모은다.
Visual Studio에서 열려면 `build/x64/FPEval.sln`을 연다.

## 테스트 DB 준비

FVC 이미지는 재배포가 제한된 데이터라 저장소에 포함하지 않는다. 팀 공유 자료의 `FVC2000_DB3_B.zip`,
`FVC2002_DB3_B.zip`, `FVC2004_DB1_B.zip`을 `testdb/` 아래에 같은 이름의 폴더로 압축 해제한다.

```
testdb/
 ├─ FVC2000_DB3_B/101_1.tif …
 ├─ FVC2002_DB3_B/…
 ├─ FVC2004_DB1_B/…
 ├─ base.json            기본 평가 규칙
 └─ pairs_sample.csv     부분 평가 예시
```

## 실행

1. `bin/FPEval.exe` 실행
2. **1. 대상** 탭: DLL, DB 폴더, iType, 결과 폴더 지정
3. **2. 시험 규칙** 탭: 등록·본인·타인 규칙, seed, (선택) 부분 평가 쌍 파일
4. **평가 시작** → 완료 후 **리포트 열기**

iType: FVC2000 = 1, FVC2002 = 2, FVC2004 = 3 (FPAlgo의 임시 값, 아래 "알려진 제한" 참고)

콘솔 버전:

```bash
bin/FPEvalCli.exe --config testdb/base.json --dll bin/plugins/x64/FPAlgo.dll --db testdb/FVC2002_DB3_B --itype 2 --out results
```

```bash
bin/FPEvalCli.exe --resume results/<결과 폴더>
```

```bash
bin/FPEvalCli.exe --report-only results/<결과 폴더>
```

GUI 설정(`last_config.json`)과 GUI의 기본 결과 폴더는 **문서\FPEval**에 저장된다.

## 결과 폴더 구조

```
results/<DB명>_<날짜시간>/
 ├─ config.json, run_info.json   평가 설정·환경
 ├─ plan.csv                     실행 순서 전체 (재현용)
 ├─ pairs_genuine.csv, pairs_impostor.csv  (부분 평가 입력으로 재사용 가능)
 ├─ log_enroll.csv, log_match.csv           한 줄씩 즉시 기록 (중단 후 이어하기)
 ├─ report.html, report.docx
 ├─ figures/ det, score_dist, fmr_fnmr (.svg + .emf 벡터)
 ├─ cache/       raw 이미지
 └─ templates/   알고리즘 생성 템플릿
```

## 소스 구조

```
src/common/    GUI·Worker 공용: Protocol.h(파이프 메시지), Status.h, KisaApi.h, PathUtil, Handle(RAII)
src/worker/    FPWorker: DLL 로드, 가드 페이지 버퍼, SEH, 시간 측정
src/core/      FPEvalCore (UI 없는 평가 로직)
  config/      EvalConfig, 경량 JSON
  db/          TIFF 로더, FingerprintDB
  plan/        Rng(재현 가능 난수), 조합 규칙(전략 패턴 + 데코레이터), RuleFactory, EvalPlan
  exec/        WorkerProcess(Named Pipe), EvalScheduler(Worker 풀, timeout/crash 처리)
  result/      ResultStore(CSV)
  metrics/     MetricsCalculator (FTE, REJ, FMR/FNMR, EER(FVC), EER*, ZeroFMR …)
  chart/       ICanvas ← SvgCanvas / EmfCanvas, ChartRenderer
  report/      IReportWriter(템플릿 메서드) ← Html / Docx, ZipWriter
  Evaluator    STEP 0~5 전체 흐름
src/gui/       MFC GUI: CFPEvalApp, CMainDlg(탭 + 실행 영역), CTargetPage/CRulePage/CRunPage/CReportPage(DDX),
               CProgressBridge(작업 스레드 → PostMessage), FPEval.rc(대화상자 템플릿), 정적 MFC 링크
src/cli/       콘솔 실행기
src/algo/      FPAlgo.dll: 정규화 → 방향장·분할 → 국소 정규화 → Gabor → 세선화 → 미뉴셔 → Hough 매칭
src/testdlls/  Dummy_OK / Bad_Crash / Bad_Timeout / Bad_Score / Bad_Size / Bad_Random
tests/         CoreTests (규칙 쌍 개수, 재현성, 지표 수식, JSON/CSV/CRC)
tools/         AlgoDebug (알고리즘 중간 영상 덤프, 제출물 아님)
installer/     FPEval.iss (Inno Setup)
docs/          과제 계획서, 동작 명세, 통합 문서
```

## 설치 패키지

Inno Setup 6이 필요하다. 사용자 설치 기준:

```bash
"%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" installer\FPEval.iss
```

`dist/FPEval_Setup_0.1.0.exe`가 만들어진다. 관리자 권한이면 Program Files에, 아니면 "현재 사용자만" 설치를 고르면
`%LOCALAPPDATA%\Programs`에 설치된다. Inno Setup을 Program Files에 설치했다면 경로를 `C:\Program Files (x86)\Inno Setup 6\ISCC.exe`로 바꾼다.

## 검증 결과 (v0.1.0 프로토타입, FVC 공개 DB 각 80장)

규칙: 등록 All, 본인 Symmetric(280), 타인 FirstOne 대칭 제거(45), seed 20261104

| DB | iType | FTE | EER |
|---|---|---|---|
| FVC2000_DB3_B | 1 | 0% | 12.66% |
| FVC2002_DB3_B | 2 | 0% | 34.17% |
| FVC2004_DB1_B | 3 (임시) | 0% | 28.73% |

- x86/x64 FPAlgo 결과 동일, Worker 1개와 8개 결과 동일, 강제 종료 후 이어하기 결과 = 한 번에 수행한 결과
- 시험용 DLL: CRASH, TIMEOUT, MISMATCH, FAIL, OVERFLOW 모두 감지, Bad_Random EER ≈ 49%
- report.docx는 Word에서 표 3개·EMF 그림 3개로 열림, CoreTests 전체 통과

## 알려진 제한·남은 일

- **iType 2 모호성:** FVC2002(300x300)와 FVC2004(640x480)가 같은 iType 2라 DLL이 크기를 구분할 수 없다. FPAlgo는 임시로 FVC2004에 iType 3을 쓴다 (`src/algo/SensorTable.h`). 확인 후 수정.
- **알고리즘 성능:** 프로토타입 수준. 매칭 개선과 파라미터 튜닝은 13~14주 작업.
- **설치 패키지:** 깨끗한 Windows(VM)에서의 설치·실행·제거 확인이 남아 있다.
- 전체 남은 작업은 [docs/프로젝트_통합문서.html](docs/프로젝트_통합문서.html) 9장 참고.

## 역할

| 담당 | 영역 |
|---|---|
| A · 알고리즘 | 지문인식 DLL, 시험용 DLL, 성능 튜닝 |
| B · 평가 엔진 | Worker 프로세스·통신, 작업 분배(병렬·timeout·중지), 조합 규칙, 로그 |
| C · UI·리포트·배포 | MFC GUI, 지표 계산, 그래프·리포트, 설치 패키지 |

## 일정

| 주차 | 내용 |
|---|---|
| 12 | 진도 보고 |
| 13 | 중간 시연 발표 |
| 14 | 진도 보고 |
| 15 | 최종 발표, 최종 시연 |

## 브랜치 규칙

| 브랜치 | 용도 |
|---|---|
| `main` | 발표·제출 시점의 안정 버전만. PR로만 병합 |
| `develop` | 개발 통합 브랜치. 기능 브랜치를 여기에 병합 |
| `feature/<영역>-<내용>` | 기능 개발 (예: `feature/algo-local-matching`) |
| `fix/<영역>-<내용>` | 버그 수정 |
| `docs/<내용>` | 문서 |

버전 태그: `v0.1.0` 프로토타입 · `v0.2.0` 중간 시연 · `v1.0.0` 최종 제출

## 커밋 메시지

`<종류>(<영역>): <내용>` — 종류는 `feat`, `fix`, `docs`, `test`, `refactor`, `build`, `chore`

```
feat(algo): 로컬 구조 기반 미뉴셔 쌍 사전 필터 추가
fix(engine): 결과 폴더 이름 충돌 수정
```

## 문서

| 문서 | 내용 |
|---|---|
| [docs/팀프로젝트_전체계획.html](docs/팀프로젝트_전체계획.html) | 요구사항, 설계 개요, 역할, 일정, 리스크 |
| [docs/코드작성계획_동작명세.html](docs/코드작성계획_동작명세.html) | 동작 명세, 솔루션 구조, 클래스 설계, 코드 작성 순서 |
| [docs/프로젝트_통합문서.html](docs/프로젝트_통합문서.html) | 기술 명세, 현재 상태, 검증 결과, 남은 작업 |

## 참고 자료

- K-NBTC: https://www.ksecurity.or.kr/kisis/subIndex/99.do
- FVC2004: http://bias.csr.unibo.it/fvc2004/
