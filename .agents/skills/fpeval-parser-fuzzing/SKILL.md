---
name: "fpeval-parser-fuzzing"
description: "Build bounded malformed-input and fuzz tests for Fingerprint-Evaluator TIFF, JSON, CSV, template and pipe parsing; diagnose ASan memory failures. Use for parser hardening or reproduced memory defects, not every ordinary code change."
license: "MIT"
---

# 입력 파서·메모리 오류 검사

## 대상과 경계

`TiffReader`, `Json`, CSV, 템플릿, Worker 메시지 중 수정 대상 하나를 선택한다. 유효 입력 처리와 오류 반환을 모두 검사한다. 알려진 Bad DLL의 의도적 충돌과 파서 자체의 결함을 구분한다.

1. 공개 가능한 작은 합성 seed를 만든다. TIFF tag/strip 길이, JSON 중첩·escape·큰 숫자, CSV quote·열 수, 템플릿 count/version, 파이프 bodyLen을 경계 전후로 변형한다. FVC 원본을 corpus에 넣거나 외부 서비스에 보내지 않는다.
2. 하네스는 입력 바이트 길이를 존중하고 반복마다 상태를 초기화한다. 출력은 전용 임시 폴더로 제한한다. 입력 수·최대 길이·시간·메모리 예산을 정하고 무한 실행하지 않는다.
3. 길이 인자가 없는 `readTemplate`/KISA Compare에 짧은 임의 버퍼를 안전한 파서처럼 직접 넘기지 않는다. 실제 Worker의 가드 경계를 재현하거나 길이를 아는 내부 진입점을 만들어 검사한다. 시험 DLL은 격리 Worker에서 호출한다.
4. 우선 결정적 변형 회귀 사례를 실행한다. coverage-guided fuzzing이 필요하면 설치된 Windows LLVM/libFuzzer 지원과 target 링크를 확인하고 독립 테스트 대상으로 구성한다. Linux AFL fork 명령을 Windows에서 그대로 사용하지 않는다.
5. MSVC ASan은 `/fsanitize=address`와 기호 정보를 사용한다. `/RTC`, `/ZI`, incremental link와의 충돌을 확인한다. ASan/UBSan/TSan을 하나의 MSVC 옵션 세트로 가정하지 않는다. 진단 바이너리를 배포 `bin/`에 섞지 않도록 별도 출력 설정도 확인한다.
6. 오류 입력을 보존·축소하고 동일 binary/config에서 재현한다. 접근 지점과 할당/해제 스택을 함께 읽어 경계·수명 원인을 수정한 뒤 최소 입력을 회귀 테스트로 남긴다.

완료 기록: 하네스 경계, 도구·구성, seed/corpus, 실행 예산과 실제 건수, 오류·축소 입력, 미검사 영역. 짧은 무사고 실행은 안전성 증명이 아니다.

공식 기준: [MSVC ASan 지원·제한](https://learn.microsoft.com/en-us/cpp/sanitizers/asan). 원문의 플랫폼별 예제 코드는 복사하지 않았으며 파일·길이·수명 검사를 프로젝트에 맞게 추가했다.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [mohitmishra786/low-level-dev-skills/fuzzing](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/runtimes/fuzzing/SKILL.md) · commit `bdc58472fa9f` · [원문 라이선스](../../../docs/skill-sources/licenses/mohitmishra786--low-level-dev-skills.txt)
- [mohitmishra786/low-level-dev-skills/sanitizers](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/runtimes/sanitizers/SKILL.md) · commit `bdc58472fa9f` · [원문 라이선스](../../../docs/skill-sources/licenses/mohitmishra786--low-level-dev-skills.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
