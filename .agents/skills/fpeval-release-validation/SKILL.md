---
name: "fpeval-release-validation"
description: "Prepare and validate Fingerprint-Evaluator Release artifacts and Inno Setup packaging, architecture completeness and clean-Windows install/run/uninstall behavior. Does not authorize publishing or modifying a user installation."
license: "MIT"
---

# 설치·배포 검증

## 산출물 검증

`installer/FPEval.iss`, README, CMake 출력 설정을 읽고 실제 설치 입력을 확인한다. 원문의 구성·런타임 확인과 증거 기반 완료 판단을 배포에 적용한다.

1. Win32/x64 Release를 빌드하고 CoreTests를 실행한다. Debug·ASan·프로파일 진단 바이너리가 `bin/`을 덮어썼는지 확인한다.
2. GUI·CLI·Worker32/64·`plugins/x86`과 `plugins/x64`의 FPAlgo 및 의도한 시험 DLL을 점검한다. Inno의 `*.dll` wildcard가 실험용 외부 DLL까지 포함하지 않는지 실제 목록을 확인한다.
3. `/MT`와 정적 MFC를 유지한다. 외부 평가 DLL의 자체 의존성은 별도다. FVC·캐시·템플릿·로컬 결과·AlgoDebug를 설치 파일에 넣지 않는다.
4. 실제 설치된 Inno Setup 6의 ISCC 경로를 찾아 `& "<ISCC.exe 절대 경로>" .\installer\FPEval.iss`를 실행하고 종료 코드·생성 파일·버전을 기록한다. 생성만으로 설치 성공이라고 하지 않는다.
5. 전용 Windows VM에서 현재 사용자/관리자 설치 중 변경에 관련된 경로를 검증한다. 설치 위치 쓰기 권한 없이 실행, 문서 폴더 설정·결과, 정상 평가·리포트 열기, 제거를 확인한다.
6. 업그레이드/제거 변경이면 앱 파일과 사용자의 설정·결과 보존 정책을 따로 확인한다. 실제 사용자의 설치본에서 임의 제거 시험을 하지 않는다.

VM이 없으면 패키지 생성과 정적 목록 검증까지를 결과로 기록한다. 설치 파일 publish·업로드·태그 생성은 별도 사용자 요청 범위에 따른다.

공식 명령 기준: [Inno Setup compiler](https://jrsoftware.org/ishelp/topic_compilercmdline.htm). 완료 증거: 버전·SHA-256·포함 파일, 컴파일 결과, VM 환경·설치/평가/제거 결과와 미실행 항목.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [obra/superpowers/verification-before-completion](https://github.com/obra/superpowers/blob/22f0b463e7a5e579904dd6211a520321035526f7/skills/verification-before-completion/SKILL.md) · commit `22f0b463e7a5` · [원문 라이선스](../../../docs/skill-sources/licenses/obra--superpowers.txt)
- [mohitmishra786/low-level-dev-skills/msvc-cl](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/compilers/msvc-cl/SKILL.md) · commit `bdc58472fa9f` · [원문 라이선스](../../../docs/skill-sources/licenses/mohitmishra786--low-level-dev-skills.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
