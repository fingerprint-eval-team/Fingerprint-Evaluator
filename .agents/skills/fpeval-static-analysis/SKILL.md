---
name: "fpeval-static-analysis"
description: "Run or configure MSVC analysis or clang-tidy for Fingerprint-Evaluator, diagnose compile_commands.json and IntelliSense errors, and triage C++ diagnostics. Do not switch the production generator merely to enable tooling."
license: "MIT"
---

# 정적 분석과 컴파일 데이터베이스

## 절차

1. 분석 대상 파일과 버그 종류를 정한다. 단순 경고 정리는 정상 빌드의 `/W4` 진단부터 확인한다. 심화 검토에는 설치된 MSVC `/analyze` 또는 clang-tidy를 선택한다.
2. 실제 구성의 매크로, include, PCH, 아키텍처, CRT를 사용한다. `UNICODE`, `NOMINMAX`, MSVC 확장·MFC를 빠뜨린 분석 결과는 코드 결함으로 확정하지 않는다.
3. 현재 기본 VS 생성기는 `CMAKE_EXPORT_COMPILE_COMMANDS`를 지원하지 않는다. CMake 옵션만 넣고 파일이 생겼다고 보고하지 않는다. 공식 CMake 문서는 Makefile/Ninja 생성기에만 지원된다고 명시한다.
4. 컴파일 DB가 꼭 필요하면 설치된 추출 도구로 실제 생성 프로젝트의 명시적 구성·플랫폼을 읽거나, 별도 분석용 Ninja 구성을 검토한다. 후자는 MSVC 환경·MFC 설정과 실제 명령의 일치를 확인해야 한다. 원문의 자동 다운로드·실행 절차는 그대로 수행하지 않는다.
5. DB는 JSON 배열인지, `directory`·`file`·`command` 또는 `arguments`가 실재하는지, 수정 대상 TU가 포함되는지 확인한다. 경로·플래그를 추측해서 만들어 넣지 않는다.
6. `bugprone`·`clang-analyzer`·필요한 `performance` 진단부터 적용한다. C++20 modernization이나 전체 자동 fix를 피하고, 실제 수명·경계·오버플로 문제인지 호출부와 함께 판정한다.

도구 실행 실패·헤더 해석 실패·진단 없음은 서로 다르다. 사용한 도구 버전, 대상 구성, 분석 범위, 확정 결함과 오탐 근거를 기록한다. 수정 후 정상 MSVC 빌드와 관련 회귀 사례를 확인한다.

공식 기준: [CMake compile commands](https://cmake.org/cmake/help/latest/variable/CMAKE_EXPORT_COMPILE_COMMANDS.html).

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [mohitmishra786/low-level-dev-skills/static-analysis](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/build-systems/static-analysis/SKILL.md) · commit `bdc58472fa9f` · [원문 라이선스](../../../docs/skill-sources/licenses/mohitmishra786--low-level-dev-skills.txt)
- [microsoft/cpp-language-server/generate-compile-commands](https://github.com/microsoft/cpp-language-server/blob/db8100bc696a42914a95e56c0ae3f84ed7548e79/plugins/cpp-language-server/skills/generate-compile-commands/SKILL.md) · commit `db8100bc696a` · [원문 라이선스](../../../docs/skill-sources/licenses/microsoft--cpp-language-server.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
