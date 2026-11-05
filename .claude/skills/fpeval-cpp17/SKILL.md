---
name: "fpeval-cpp17"
description: "Implement or refactor Fingerprint-Evaluator C++17 code involving RAII, ownership, resource lifetime, templates, or exception boundaries. Excludes build-only changes and numerical metric validation."
license: "MIT"
---

# C++17 구현과 소유권

## 적용

원문의 아키텍처 확인 → 자원 수명 설계 → 구현 → 검증 흐름을 C++17로 제한한다. 먼저 `docs/CODE_CONVENTIONS.md`와 수정 대상 헤더·호출부를 읽는다.

1. 값, 단독 소유, 비소유 참조를 구분한다. 공동 소유가 실제 필요한 경우에만 `shared_ptr`를 도입한다. `vector` 재할당, 임시 문자열, 람다 캡처로 생기는 포인터 무효화를 확인한다.
2. 기존 `UniqueHandle`은 `CloseHandle` 자원에 쓴다. `HMODULE`, GDI 객체, DC, `FILE*`는 각 해제 API와 짝을 맞춘다. 이동 후 원본의 무효 상태와 이중 해제 여부를 검토한다.
3. Core는 MFC를 모르도록 유지하고 GUI·CLI 공용 `Evaluator`를 사용한다. DLL export 밖으로 예외를 보내지 않는다. 오류를 정상 값으로 숨기지 말고 기존 상태·오류 문자열 경계에 맞춘다.
4. 길이 곱셈·부호 변환·인덱스·빈 컨테이너를 검토한다. `static_cast` 자체는 범위 검사가 아니므로 좁히기 전에 범위를 검사한다.
5. 동작을 바꾸는 수정은 해당 경계의 회귀 사례를 만든 뒤 `fpeval-verification`으로 검증한다. 단순 이름·주석 정리는 의미 없는 테스트를 추가하지 않는다.

## 프로젝트 조정

- C++20 concepts/ranges/coroutines와 C++23 기능을 도입하지 않는다. 표준 버전은 CMake의 17을 유지한다.
- 원문의 모든 포인터를 스마트 포인터로 바꾸는 식의 적용은 피한다. `PostMessage` 소유권 이전은 송신 실패·수신·창 종료를 함께 다룬다.
- GCC 경고 플래그나 ASan/UBSan 실행을 모든 수정의 전제로 삼지 않는다. MSVC `/W4`와 실제 도구 가용성을 따른다.

완료 기록: 바뀐 소유권 또는 인터페이스, 영향을 받는 호출부, 실행한 검증과 남은 제약.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [Jeffallan/claude-skills/cpp-pro](https://github.com/Jeffallan/claude-skills/blob/078a25cc5bfd8011459be7b91d004e900c1d02d3/skills/cpp-pro/SKILL.md) · commit `078a25cc5bfd` · [원문 라이선스](../../../docs/skill-sources/licenses/Jeffallan--claude-skills.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
