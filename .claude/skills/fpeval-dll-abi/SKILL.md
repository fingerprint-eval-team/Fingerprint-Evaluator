---
name: "fpeval-dll-abi"
description: "Review or change KISA DLL exports, x86/x64 loading, calling conventions, template binary layout or named-pipe message ABI in Fingerprint-Evaluator. Use for compatibility changes, not general scheduler debugging."
license: "MIT"
---

# Windows DLL·파이프 계약

## 계약 추적

`src/common/KisaApi.h`, `Protocol.h`, `Status.h`, `src/algo/BioApi.h`, 두 `.def`, `templ/Template.h`, Worker 송수신 코드를 읽는다.

1. 함수 포인터 선언과 export의 인자·반환형·`__cdecl`을 대조한다. export 이름은 `KISA_Enroll`, `KISA_Attempt`, `KISA_Compare`다. x86 이름 장식과 x64 호출 규약을 구분한다.
2. VS 개발 환경에서 `dumpbin /headers <DLL>`로 Machine, `/exports <DLL>`로 export, `/dependents <DLL>`로 종속 DLL을 읽는다. 도구가 없으면 기존 PE 판별과 Worker probe 결과를 사용하고 미확인 항목을 적는다.
3. x64 GUI/CLI가 시험 DLL을 직접 로드하지 않도록 한다. DLL 아키텍처별 Worker에서 Hello와 실제 호출을 확인한다. 검색 경로·부속 DLL 문제와 export 누락을 구분한다.
4. 파이프에 포인터·STL·`size_t`를 추가하지 않는다. `pack(1)`·고정 크기 타입·Windows 2바이트 `wchar_t`를 확인한다. 구조 변경은 양쪽의 sizeof/offsetof, 메시지 type/bodyLen/jobId 검증을 함께 다룬다.
5. 템플릿 magic/version/count/전체 길이와 x86/x64 호환성을 확인한다. KISA Compare는 길이 인자가 없으므로 Worker 가드와 내부 헤더 검사가 필요하다. 직렬화 형식을 바꾸면 이전 템플릿의 처리 방침을 기록한다.
6. `0 < size < 1000000`, 유한한 `[0,1]` 점수, 오류 메시지 종료와 상태 매핑을 확인한다. FVC2004 iType 3은 FPAlgo 임시 규칙이며 외부 DLL의 표준으로 강제하지 않는다.

완료 기준: 양쪽 아키텍처 export와 정상 왕복, 관련 Bad DLL 상태, 이전 데이터 호환성의 실제 확인. 빌드만으로 ABI 검증을 대신하지 않는다.

상세 ABI는 Linux SysV 스킬이 아니라 [Microsoft x64 규약](https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention)을 따른다. 원문의 플랫폼 일반론에 프로젝트의 KISA·IPC 검사를 추가했다.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [mohitmishra786/low-level-dev-skills/msvc-cl](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/compilers/msvc-cl/SKILL.md) · commit `bdc58472fa9f` · [원문 라이선스](../../../docs/skill-sources/licenses/mohitmishra786--low-level-dev-skills.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
