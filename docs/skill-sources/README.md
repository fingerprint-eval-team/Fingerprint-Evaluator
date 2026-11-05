# 공개 스킬 조사와 채택 기록

검토일: 2026-11-04. GitHub 원 저장소와 공개 검색 색인에서 후보를 찾고 고정 커밋의 SKILL.md를 확인했다. 설치 수·별 수를 품질 보증으로 사용하지 않았다. 아래 표는 채택뿐 아니라 교체하지 않은 이유도 기록한다.

## 채택 방식

활성 스킬 18개는 **수정된 적응본**이다. 공개 원문의 전체 플러그인이나 설치 스크립트를 실행한 것이 아니다. 필요한 절차를 한국어로 재구성하고 프로젝트 파일·명령·실패 판정·완료 기준을 추가했다. 원문의 자동 위임·외부 게시·광범위한 설치·무조건 승인·기존 코드 삭제 지시는 가져오지 않았다.

원문 저장소 10개에서 확인한 후보와 검토 수준은 manifest에 기록했다. `procedure`는 본문 절차 검토, `scope-and-metadata`는 적용 범위만 검토하고 미채택한 후보다. 원문 예제 코드 전체를 실행 검증한 것은 아니다. SHA-256은 당시 내려받은 SKILL.md 내용의 식별자이며, 현재 웹의 HEAD와 동일함을 보장하지 않는다.

## 핵심 조정

- C++20/23 concepts·ranges·coroutines를 C++17 RAII·소유권·인터페이스 검토로 제한.
- MSVC의 일반 `/MD` 권고를 이 저장소의 `/MT`·정적 MFC 기준으로 대체. 런타임 충돌은 원인을 수정하며 `/NODEFAULTLIB`로 숨기지 않음.
- CMake Visual Studio 생성기의 compile_commands 미지원 확인. GCC/Linux 명령·TSan 지원을 Windows 기본 기능으로 가정하지 않음.
- generic ABI/ELF 대신 KISA export, Worker 격리, x86/x64 pipe·template 계약을 사용.
- 기존 CoreTests에 속성·작은 oracle·반례 축소를 적용. 외부 PBT 프레임워크 자동 도입과 의미 없는 전면 TDD 제외.
- 정확도 실험, 수식 검증, CPU 성능 측정을 분리. 같은 손가락/쌍의 종속성을 유지하고 오류 누락을 EER 개선으로 오해하지 않음.
- 웹 디자인 대신 네이티브 MFC 관찰·조작·상태 재확인. 도구의 실제 가용성을 먼저 확인.
- 리포트는 자체 ReportModel·ZipWriter·OOXML, 차트는 ICanvas·SVG/EMF를 유지. Python/Node 라이브러리는 앱 런타임에 추가하지 않음.
- 재개·릴리스는 외부의 원인 추적·증거 검증 원칙에 프로젝트 전용 절차를 더한 스킬이며, 해당 분야의 원문 전용 스킬을 발견했다고 주장하지 않음.

## 라이선스 범위

원저자 저작권·라이선스 전문은 `licenses/`에 보존했다. 활성 SKILL.md의 출처 절에서 원문과 라이선스로 연결한다. MIT 기반 적응본은 MIT, Sentry·Vercel 기반 적응본은 Apache-2.0, Trail of Bits property-based-testing을 이용한 property-testing·metrics-validation 적응본은 CC-BY-SA-4.0으로 표시했다. 혼합 출처의 기존 MIT 고지도 함께 보존한다. 이 표시는 해당 스킬 문서에 적용되며 프로젝트 C++ 코드 전체의 라이선스를 변경하지 않는다.

Anthropic docx는 확인한 원문의 개별 라이선스가 복제·파생물을 제한하여 저장소에 포함하지 않았다. frontend-design과 webapp-testing은 MFC 대체용으로 채택하지 않았다. 외부 스크립트·필수 도구를 포함하지 않은 채 원문과 동일한 실행 능력을 제공한다고 주장하지 않는다.

## 후보별 결정

| 원문 SKILL.md | 고정 커밋 | 적용된 활성 스킬 | 결정·조정 이유 |
| --- | --- | --- | --- |
| [Jeffallan/claude-skills/cpp-pro](https://github.com/Jeffallan/claude-skills/blob/078a25cc5bfd8011459be7b91d004e900c1d02d3/skills/cpp-pro/SKILL.md) | `078a25cc5bfd` | fpeval-cpp17 | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [K-Dense-AI/scientific-agent-skills/experimental-design](https://github.com/K-Dense-AI/scientific-agent-skills/blob/a25cf9ec00e2745d03f8704acf6f6919694ccd9a/skills/experimental-design/SKILL.md) | `a25cf9ec00e2` | fpeval-algorithm-experiments | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [K-Dense-AI/scientific-agent-skills/scientific-visualization](https://github.com/K-Dense-AI/scientific-agent-skills/blob/a25cf9ec00e2745d03f8704acf6f6919694ccd9a/skills/scientific-visualization/SKILL.md) | `a25cf9ec00e2` | fpeval-chart-validation | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [K-Dense-AI/scientific-agent-skills/statistical-analysis](https://github.com/K-Dense-AI/scientific-agent-skills/blob/a25cf9ec00e2745d03f8704acf6f6919694ccd9a/skills/statistical-analysis/SKILL.md) | `a25cf9ec00e2` | fpeval-algorithm-experiments | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [aniruddhaadak80/skills/code-quality-code-review-checklist-cpp](https://github.com/aniruddhaadak80/skills/blob/17f441875305220e7669ba1676bc61193d5a1a03/skills/code-quality-code-review-checklist-cpp/SKILL.md) | `17f441875305` | 미설치 | 간단한 일반 체크리스트보다 Sentry의 diff 수집·실제 결함 검증 절차를 선택. |
| [aniruddhaadak80/skills/code-quality-performance-profiling-cpp](https://github.com/aniruddhaadak80/skills/blob/17f441875305220e7669ba1676bc61193d5a1a03/skills/code-quality-performance-profiling-cpp/SKILL.md) | `17f441875305` | fpeval-performance-profiling | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [aniruddhaadak80/skills/document-generation-docx-report-generation](https://github.com/aniruddhaadak80/skills/blob/17f441875305220e7669ba1676bc61193d5a1a03/skills/document-generation-docx-report-generation/SKILL.md) | `17f441875305` | fpeval-report-generation | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [anthropics/skills/docx](https://github.com/anthropics/skills/blob/ca45fc496a3436251f191354091d557da31c15b5/skills/docx/SKILL.md) | `ca45fc496a34` | 미설치 | 현재 원문 LICENSE.txt가 복제·파생물 제한을 명시. 채택·배포하지 않고 MIT 리포트 스킬과 프로젝트 OOXML 코드 사용. |
| [anthropics/skills/frontend-design](https://github.com/anthropics/skills/blob/ca45fc496a3436251f191354091d557da31c15b5/skills/frontend-design/SKILL.md) | `ca45fc496a34` | 미설치 | 웹 디자인·CSS·브랜드 시각 언어 중심. 네이티브 MFC를 대체할 스킬로는 부적합. |
| [anthropics/skills/webapp-testing](https://github.com/anthropics/skills/blob/ca45fc496a3436251f191354091d557da31c15b5/skills/webapp-testing/SKILL.md) | `ca45fc496a34` | 미설치 | Playwright 웹 UI 테스트는 MFC 창 조작을 대체하지 못하므로 설치하지 않음. |
| [getsentry/skills/code-review](https://github.com/getsentry/skills/blob/0c464f411bdfb8d4c12314b36264d06cb2f60270/skills/code-review/SKILL.md) | `0c464f411bdf` | fpeval-code-review | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [getsentry/skills/find-bugs](https://github.com/getsentry/skills/blob/0c464f411bdfb8d4c12314b36264d06cb2f60270/skills/find-bugs/SKILL.md) | `0c464f411bdf` | fpeval-code-review | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [microsoft/cpp-language-server/generate-compile-commands](https://github.com/microsoft/cpp-language-server/blob/db8100bc696a42914a95e56c0ae3f84ed7548e79/plugins/cpp-language-server/skills/generate-compile-commands/SKILL.md) | `db8100bc696a` | fpeval-static-analysis | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [mohitmishra786/low-level-dev-skills/abi-and-calling-conventions](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/computer-architecture/abi-and-calling-conventions/SKILL.md) | `bdc58472fa9f` | 미설치 | SysV·ARM·RISC-V 설명 중심. Windows ABI는 MSVC 스킬과 Microsoft 공식 문서로 대체. |
| [mohitmishra786/low-level-dev-skills/cmake](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/build-systems/cmake/SKILL.md) | `bdc58472fa9f` | fpeval-msvc-cmake | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [mohitmishra786/low-level-dev-skills/concurrency-debugging](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/debuggers/concurrency-debugging/SKILL.md) | `bdc58472fa9f` | fpeval-concurrency | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [mohitmishra786/low-level-dev-skills/dynamic-linking](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/binaries/dynamic-linking/SKILL.md) | `bdc58472fa9f` | 미설치 | Linux ELF·RPATH·dlopen 중심이므로 Windows DLL 절차로 채택하지 않음. |
| [mohitmishra786/low-level-dev-skills/fuzzing](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/runtimes/fuzzing/SKILL.md) | `bdc58472fa9f` | fpeval-parser-fuzzing | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [mohitmishra786/low-level-dev-skills/intel-vtune-amd-uprof](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/profilers/intel-vtune-amd-uprof/SKILL.md) | `bdc58472fa9f` | fpeval-performance-profiling | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [mohitmishra786/low-level-dev-skills/memory-model](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/low-level-programming/memory-model/SKILL.md) | `bdc58472fa9f` | fpeval-concurrency | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [mohitmishra786/low-level-dev-skills/msvc-cl](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/compilers/msvc-cl/SKILL.md) | `bdc58472fa9f` | fpeval-msvc-cmake, fpeval-dll-abi, fpeval-release-validation | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [mohitmishra786/low-level-dev-skills/sanitizers](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/runtimes/sanitizers/SKILL.md) | `bdc58472fa9f` | fpeval-parser-fuzzing | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [mohitmishra786/low-level-dev-skills/static-analysis](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/build-systems/static-analysis/SKILL.md) | `bdc58472fa9f` | fpeval-static-analysis | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [obra/superpowers/receiving-code-review](https://github.com/obra/superpowers/blob/22f0b463e7a5e579904dd6211a520321035526f7/skills/receiving-code-review/SKILL.md) | `22f0b463e7a5` | 미설치 | 이번 구조에서 별도 스킬로 만들 필요가 낮아 코드 리뷰 절차에 범위를 집중. |
| [obra/superpowers/requesting-code-review](https://github.com/obra/superpowers/blob/22f0b463e7a5e579904dd6211a520321035526f7/skills/requesting-code-review/SKILL.md) | `22f0b463e7a5` | 미설치 | 자동 subagent 위임과 스킬 간 의존 대신 독립적인 Sentry 리뷰 절차 채택. |
| [obra/superpowers/systematic-debugging](https://github.com/obra/superpowers/blob/22f0b463e7a5e579904dd6211a520321035526f7/skills/systematic-debugging/SKILL.md) | `22f0b463e7a5` | fpeval-systematic-debugging, fpeval-resume-validation | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [obra/superpowers/test-driven-development](https://github.com/obra/superpowers/blob/22f0b463e7a5e579904dd6211a520321035526f7/skills/test-driven-development/SKILL.md) | `22f0b463e7a5` | fpeval-property-testing | 실패 사례 검증은 채택. 기존 코드 삭제·모든 작업의 의무 TDD·예외 승인 흐름은 제외. |
| [obra/superpowers/verification-before-completion](https://github.com/obra/superpowers/blob/22f0b463e7a5e579904dd6211a520321035526f7/skills/verification-before-completion/SKILL.md) | `22f0b463e7a5` | fpeval-resume-validation, fpeval-release-validation, fpeval-verification | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [trailofbits/skills/address-sanitizer](https://github.com/trailofbits/skills/blob/711af095215ad4ecde5b90cd1d0124b3f1c4ca9e/plugins/testing-handbook-skills/skills/address-sanitizer/SKILL.md) | `711af095215a` | 미설치 | 별도 보안 도구 스킬로 설치하지 않음. Windows MSVC 공식 지원 범위와 선택한 sanitizer 절차로 제한. |
| [trailofbits/skills/differential-review](https://github.com/trailofbits/skills/blob/711af095215ad4ecde5b90cd1d0124b3f1c4ca9e/plugins/differential-review/skills/differential-review/SKILL.md) | `711af095215a` | 미설치 | 보안 감사·강제 보고서·외부 agent 의존이 일반 과제 리뷰에 과함. Sentry 리뷰를 선택. |
| [trailofbits/skills/libfuzzer](https://github.com/trailofbits/skills/blob/711af095215ad4ecde5b90cd1d0124b3f1c4ca9e/plugins/testing-handbook-skills/skills/libfuzzer/SKILL.md) | `711af095215a` | 미설치 | 대규모 핸드북·LLVM/Linux 예제 전체 도입 대신 bounded parser-fuzzing 적응본 선택. |
| [trailofbits/skills/property-based-testing](https://github.com/trailofbits/skills/blob/711af095215ad4ecde5b90cd1d0124b3f1c4ca9e/plugins/property-based-testing/skills/property-based-testing/SKILL.md) | `711af095215a` | fpeval-property-testing, fpeval-metrics-validation | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |
| [vercel-labs/agent-desktop/agent-desktop-windows](https://github.com/vercel-labs/agent-desktop/blob/6660fbac768dc7e7332837b90ed465684859afa3/skills/agent-desktop-windows/SKILL.md) | `6660fbac768d` | fpeval-mfc-ui | 원문 절차의 적용 범위를 좁히고 C++17/MSVC·로컬 검증·현재 코드 계약으로 조정. |

## 공식 문서 교차 확인

- [CMake compile_commands 지원 생성기](https://cmake.org/cmake/help/latest/variable/CMAKE_EXPORT_COMPILE_COMMANDS.html)
- [MSVC AddressSanitizer 지원·충돌 옵션](https://learn.microsoft.com/en-us/cpp/sanitizers/asan)
- [Windows x64 호출 규약](https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention)
- [MFC 스레드 프로그래밍](https://learn.microsoft.com/en-us/cpp/mfc/multithreading-programming-tips)
- [Inno Setup 명령행 컴파일러](https://jrsoftware.org/ishelp/topic_compilercmdline.htm)

## 갱신 절차

1. manifest의 고정 원문과 새 원문 차이를 확인한다. 이름·명령만 최신으로 바꾸지 않는다.
2. 프로젝트 계약·플랫폼·라이선스에 맞는 변경만 반영한다. 새로운 외부 프로그램을 요구하면 가용성과 도입 범위를 명시한다.
3. `.agents`와 `.claude`의 같은 스킬을 갱신하고 `python tools/check_skills.py`로 이름·메타데이터·미러·로컬 링크·출처 연결을 확인한다.
4. 문서 구조 검증과 실제 스킬 행동 검증을 구분한다. 본 조사에서 앱 빌드·FVC·GUI·설치 절차를 실행한 것은 아니다.
