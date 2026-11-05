# Fingerprint-Evaluator 프로젝트 스킬

작업에 맞는 프로젝트 스킬은 이 문서에서 고른다. 스킬 선택의 기준 문서는 이 문서 하나이며, [CLAUDE.md](../CLAUDE.md)와 [AGENTS.md](../AGENTS.md)는 여기를 가리킨다. 각 `SKILL.md`는 실행 절차·프로젝트 제약·완료 증거·고정 커밋 출처를 담고, 이 문서는 선택 기준만 관리한다.

## 선택 방법

1. 아래 [변경 경로별 선택](#변경-경로별-선택)에서 수정할 파일에 맞는 주 스킬을 고른다. 경로로 정하기 어려우면 [스킬 목록](#스킬-목록)의 작업 설명으로 고른다.
2. 원인을 모르는 실패(예상 밖 상태값, 크래시, timeout, 테스트 실패)는 수정 전에 `fpeval-systematic-debugging`부터 적용한다.
3. 함께 볼 스킬은 해당 계약을 실제로 건드릴 때만 추가한다. 전체 스킬을 매번 로드하지 않는다.
4. 완료 보고 전에는 `fpeval-verification`, 리뷰 요청에는 `fpeval-code-review`를 적용한다.

Claude Code는 `.claude/skills/<이름>/SKILL.md`, Codex는 `.agents/skills/<이름>/SKILL.md`를 쓴다. 두 사본은 내용이 같으며 아래 표의 링크는 `.agents` 사본을 가리킨다. 자동으로 선택되지 않으면 파일을 직접 읽는다. 지원 클라이언트에서는 Claude Code `/fpeval-이름`, Codex `$fpeval-이름`으로 지정할 수 있다.

## 변경 경로별 선택

| 변경 경로 | 주 스킬 | 함께 볼 스킬 (해당할 때) |
| --- | --- | --- |
| `src/algo/`, `tools/AlgoDebug.cpp` | algorithm-experiments | cpp17, performance-profiling(속도 목표), dll-abi(템플릿 형식) |
| `src/common/KisaApi.h`·`Protocol.h`·`Status.h`, `*.def`, `src/testdlls/` | dll-abi | parser-fuzzing(입력 검증), resume-validation(상태값 저장) |
| `src/worker/`, `src/core/exec/` | concurrency | dll-abi(메시지 배치), systematic-debugging |
| `src/core/plan/` | property-testing | resume-validation(`plan.csv` 재현) |
| `src/core/metrics/` | metrics-validation | property-testing, report-generation(표시 수치) |
| `src/core/config/`, `src/core/result/`, `Evaluator.cpp`의 재개 경로 | resume-validation | property-testing, parser-fuzzing(`Json.cpp`) |
| `src/core/db/` | parser-fuzzing | property-testing |
| `src/core/report/` | report-generation | chart-validation |
| `src/core/chart/` | chart-validation | report-generation |
| `src/gui/` | mfc-ui | concurrency(`ProgressBridge` 메시지 수명) |
| `CMakeLists.txt`, `src/gui/CMakeLists.txt`, `build.ps1` | msvc-cmake | static-analysis |
| `installer/`, 배포 패키지 | release-validation | msvc-cmake |
| `tests/CoreTests.cpp` | property-testing | 검사 대상 영역의 주 스킬 |
| 여러 파일의 C++ 구현·리팩터링(소유권, RAII, 예외 경계) | cpp17 | static-analysis |

표의 이름은 `fpeval-` 접두사를 생략했다.

## 스킬 목록

| 스킬 | 작업 | 채택한 공개 스킬 |
| --- | --- | --- |
| [fpeval-cpp17](../.agents/skills/fpeval-cpp17/SKILL.md) | C++17 구현과 소유권 | cpp-pro |
| [fpeval-msvc-cmake](../.agents/skills/fpeval-msvc-cmake/SKILL.md) | MSVC·CMake 빌드 | msvc-cl, cmake |
| [fpeval-static-analysis](../.agents/skills/fpeval-static-analysis/SKILL.md) | 정적 분석과 컴파일 데이터베이스 | static-analysis, generate-compile-commands |
| [fpeval-dll-abi](../.agents/skills/fpeval-dll-abi/SKILL.md) | Windows DLL·파이프 계약 | msvc-cl |
| [fpeval-systematic-debugging](../.agents/skills/fpeval-systematic-debugging/SKILL.md) | 증거 기반 원인 추적 | systematic-debugging |
| [fpeval-concurrency](../.agents/skills/fpeval-concurrency/SKILL.md) | 동시성·취소·자원 수명 | concurrency-debugging, memory-model |
| [fpeval-resume-validation](../.agents/skills/fpeval-resume-validation/SKILL.md) | 저장·재개·설정 호환성 | systematic-debugging, verification-before-completion |
| [fpeval-property-testing](../.agents/skills/fpeval-property-testing/SKILL.md) | 속성·회귀 테스트 | property-based-testing, test-driven-development |
| [fpeval-parser-fuzzing](../.agents/skills/fpeval-parser-fuzzing/SKILL.md) | 입력 파서·메모리 오류 검사 | fuzzing, sanitizers |
| [fpeval-metrics-validation](../.agents/skills/fpeval-metrics-validation/SKILL.md) | 지문 평가 수식 검증 | property-based-testing |
| [fpeval-algorithm-experiments](../.agents/skills/fpeval-algorithm-experiments/SKILL.md) | 인식 알고리즘 실험 | experimental-design, statistical-analysis |
| [fpeval-performance-profiling](../.agents/skills/fpeval-performance-profiling/SKILL.md) | CPU·메모리·처리량 측정 | code-quality-performance-profiling-cpp, intel-vtune-amd-uprof |
| [fpeval-mfc-ui](../.agents/skills/fpeval-mfc-ui/SKILL.md) | MFC 화면·키보드·상태 검증 | agent-desktop-windows |
| [fpeval-report-generation](../.agents/skills/fpeval-report-generation/SKILL.md) | HTML·OOXML 리포트 생성 | document-generation-docx-report-generation |
| [fpeval-chart-validation](../.agents/skills/fpeval-chart-validation/SKILL.md) | 과학적 그래프·SVG·EMF 검증 | scientific-visualization |
| [fpeval-release-validation](../.agents/skills/fpeval-release-validation/SKILL.md) | 설치·배포 검증 | verification-before-completion, msvc-cl |
| [fpeval-code-review](../.agents/skills/fpeval-code-review/SKILL.md) | 변경 영향 중심 코드 리뷰 | code-review, find-bugs |
| [fpeval-verification](../.agents/skills/fpeval-verification/SKILL.md) | 작업 완료 검증 | verification-before-completion |

## 조합 예시

- 매처 정확도 개선: cpp17 → algorithm-experiments → verification. 속도도 목표면 performance-profiling 추가.
- EER 분모 수정: metrics-validation → property-testing → report-generation → verification.
- Worker timeout·중지 후 멈춤: systematic-debugging → concurrency → resume-validation → verification.
- DLL export·파이프 구조 변경: dll-abi → 관련 parser-fuzzing 또는 concurrency → verification.
- 설정·CSV 호환성 수정: resume-validation → property-testing → verification.
- GUI 배치: mfc-ui → verification. 메시지 수명도 바뀌면 concurrency 추가.
- Word 리포트 그림 누락: report-generation → chart-validation → verification.
- 패키지 제출: msvc-cmake → release-validation → verification.

## 이전 스킬 대응

기존 5개의 공용 절차 링크형 스킬은 공개 스킬 기반의 18개 작업별 적응본으로 교체했다.

| 삭제한 진입점 | 대체 |
| --- | --- |
| fpeval-verify | msvc-cmake, static-analysis, release-validation, verification |
| fpeval-benchmark | property-testing, metrics-validation, algorithm-experiments, performance-profiling |
| fpeval-worker-debug | systematic-debugging, dll-abi, concurrency, resume-validation, parser-fuzzing |
| fpeval-mfc-design | mfc-ui |
| fpeval-report-qa | report-generation, chart-validation |

기존 이름은 자동 선택 중복을 막기 위해 활성 alias로 남기지 않는다. 이력 문서에서의 이름은 위 대응표로 해석한다.

## 출처와 관리

[후보 비교·라이선스·조정 내역](skill-sources/README.md)과 [고정 커밋 manifest](skill-sources/manifest.json)를 관리한다. 원문 자동 업데이트는 하지 않는다. 업데이트는 차이·플랫폼·라이선스를 다시 검토하고 두 클라이언트 사본을 함께 갱신한다.

검사: `python tools/check_skills.py`, `git diff --check`. 검사기는 표준 라이브러리만 사용해 이름·설명·라이선스, 두 사본, 선택표·로컬 링크·출처 연결을 확인한다. frontmatter는 YAML 호환 단일행 문자열 형식을 사용한다. 문서만 바꾸면 빌드·FVC 평가를 생략할 수 있다. 스킬이 제공하는 절차와 실제로 해당 도구를 실행해 확인한 결과는 구분한다. 현재 스킬 문서는 외부 라이브러리를 제출 프로그램에 추가하지 않는다.
