---
name: "fpeval-msvc-cmake"
description: "Configure, build, or diagnose Fingerprint-Evaluator MSVC/CMake, MFC, x86/x64, CRT, linker and PDB issues. Use for build.ps1 or CMake changes; not for source-level runtime failures."
license: "MIT"
---

# MSVC·CMake 빌드

## 절차

1. `build.ps1`, 루트·GUI `CMakeLists.txt`, 최초 실패 로그를 확인한다. 구성 실패, 컴파일 실패, 링크 실패, 실행 실패를 구분한다.
2. `build.ps1`의 vswhere 결과와 C++·MFC 설치 여부를 사용한다. 설치 경로·VS 버전을 하드코딩하거나 생성된 `.vcxproj`를 직접 고쳐 CMake와 어긋나게 하지 않는다.
3. 기본 명령은 저장소 루트의 `powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Config Release`다. 성공 후 `build/x64/tests/CoreTests.exe`를 실행한다. 구체 명령은 `fpeval-verification`을 따른다.
4. Win32는 Worker32와 x86 DLL, x64는 GUI·CLI·Worker64·x64 DLL·CoreTests를 만든다. `build/Win32`와 `build/x64`를 혼용하지 않는다. VS 생성기는 `--config Release|Debug`로 구성을 선택한다.
5. 새 설정은 가능한 한 `target_*`와 PUBLIC/PRIVATE/INTERFACE로 필요한 대상에만 전달한다. 기존 공통 플래그 전체 재작성이나 새 패키지 관리자는 작업에 필요한 경우에만 검토한다.

## 오류 분리

| 증상 | 확인 |
| --- | --- |
| LNK2019 | 정의가 실제 target에 포함되는지, 라이브러리·함수 시그니처·호출 규약 |
| LNK2038 / CRT 충돌 | 같은 링크에 들어가는 대상의 `/MT`·`/MTd`와 구성 일치 |
| LNK1104 | 실제 파일·경로·아키텍처, 실행 중인 프로세스의 출력 파일 잠금 |
| CMake generator 불일치 | 기존 캐시의 생성기·VS 인스턴스. 캐시를 무작정 지우지 말고 별도 빌드 폴더 검토 |
| MFC 헤더/링크 오류 | MFC 구성 요소와 GUI의 `CMAKE_MFC_FLAG 1`, 유니코드 설정 |

원문의 `/MD` 기본 권고는 적용하지 않는다. 이 프로젝트는 `/MT`와 정적 MFC 배포다. `/NODEFAULTLIB`로 런타임 충돌을 감추지 않는다. C++17, `/utf-8`, `/W4`, `/permissive-`, `/EHsc`를 보존한다.

Release/Debug는 `bin/`을 공유하므로 동시에 빌드하지 않는다. 별도 빌드 디렉터리도 현재 `fp_output` 때문에 `bin/`을 공유한다. 비교 기준 DLL을 먼저 보관한다. 검증 결과에는 구성·아키텍처·최초 오류·수정 후 종료 코드를 적는다.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [mohitmishra786/low-level-dev-skills/msvc-cl](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/compilers/msvc-cl/SKILL.md) · commit `bdc58472fa9f` · [원문 라이선스](../../../docs/skill-sources/licenses/mohitmishra786--low-level-dev-skills.txt)
- [mohitmishra786/low-level-dev-skills/cmake](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/build-systems/cmake/SKILL.md) · commit `bdc58472fa9f` · [원문 라이선스](../../../docs/skill-sources/licenses/mohitmishra786--low-level-dev-skills.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
