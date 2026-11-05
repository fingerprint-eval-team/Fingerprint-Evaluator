---
name: "fpeval-verification"
description: "Verify evidence before reporting Fingerprint-Evaluator work complete. Select fresh build, CoreTests, document/skill checks and domain checks proportional to the actual change; do not rerun unchanged successful checks without a reason."
license: "MIT"
---

# 작업 완료 검증

## 검증 선택

먼저 변경 파일과 요구 결과를 대조한다. 원문의 '주장에 맞는 실제 증거' 원칙을 사용하되 모든 작업에 같은 전체 실행을 강제하지 않는다.

| 변경 | 최소 확인 |
| --- | --- |
| 문서·스킬 | 링크·메타데이터·두 클라이언트 사본·출처, `git diff --check` |
| C++·빌드·리소스 | Win32/x64 빌드 후 x64 CoreTests |
| 실행·평가·UI·출력·설치 | 기본 확인 + 해당 작업별 스킬의 실제 동작 확인 |

## 코드 검증 명령

저장소 루트의 Windows PowerShell에서 실행한다. 네이티브 명령마다 종료 코드를 확인하고 실패한 빌드의 이전 바이너리를 실행해 성공을 주장하지 않는다.

```powershell
git diff --check
if ($LASTEXITCODE -ne 0) { throw "Diff whitespace check failed" }
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Config Release
if ($LASTEXITCODE -ne 0) { throw "Build failed: $LASTEXITCODE" }
& .\build\x64\tests\CoreTests.exe
if ($LASTEXITCODE -ne 0) { throw "CoreTests failed: $LASTEXITCODE" }
```

`CoreTests`는 현재 CTest 등록이 없어 직접 실행한다. Debug 확인은 필요할 때 `-Config Debug`로 하며 구성 간 `bin/` 공유에 주의한다.

문서·스킬 관리 검사는 `python tools/check_skills.py`를 사용한다. Python은 개발 검증용이며 제출 프로그램 의존성이 아니다. 미추적 문서는 git diff 검사가 빠뜨리므로 검사기에 포함한다.

## 완료 보고

- 사용한 명령·구성·종료 코드와 실제 검사 수를 기록한다. 과거 README의 통과 수를 이번 결과로 재사용하지 않는다.
- 빌드, 수치, UI 렌더링, 설치 결과를 서로 대체하지 않는다. FVC·Word·VM·도구 부재는 구체적인 미실행 범위로 기록한다.
- 동일 변경에 대해 이미 성공한 검증은 새 수정·실패·미해결 의문이 없으면 반복하지 않는다. 사용자에게는 핵심 결과와 한계를 간결히 전달한다.
- 확인되지 않은 성공 대신 현재까지 확인한 범위를 말한다. 이 스킬은 커밋·push·외부 게시 권한을 추가하지 않는다.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [obra/superpowers/verification-before-completion](https://github.com/obra/superpowers/blob/22f0b463e7a5e579904dd6211a520321035526f7/skills/verification-before-completion/SKILL.md) · commit `22f0b463e7a5` · [원문 라이선스](../../../docs/skill-sources/licenses/obra--superpowers.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
