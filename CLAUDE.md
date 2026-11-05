# Fingerprint-Evaluator 공용 작업 지침

## 프로젝트 경계

- 작업 디렉터리는 `Fingerprint-Evaluator/` 저장소 루트다. 먼저 `git status --short`와 관련 diff를 확인하고 기존 작업을 보존한다. `develop`은 통합 대상이며, 검토를 위해 현재 브랜치를 자동 전환하거나 기존 변경을 되돌리지 않는다.
- Windows C++17/MSVC 프로젝트다. `FPEval.exe`와 `FPEvalCli.exe`는 x64 평가 엔진을 공유한다. 시험 대상 DLL은 아키텍처에 맞는 `FPWorker32.exe` 또는 `FPWorker64.exe`에서 호출한다.
- 제출 프로그램은 C/C++와 Windows SDK·MFC로 구성하며 현재 외부 라이브러리 의존성이 없다. 과제에서 허용하는 단위 기능 라이브러리를 도입할 때는 필요성과 출처·라이선스·배포 영향을 기록한다. 개발용 검증 도구와 제출 프로그램의 런타임 의존성을 구분한다.
- 개발 통합 브랜치는 `develop`이다. 작업은 [README](README.md#브랜치-규칙)의 `feature/`·`fix/`·`docs/` 브랜치에서 하고 `develop`·`main`에 직접 커밋하지 않는다. 현재 브랜치가 `develop`이면 커밋 전에 작업 브랜치를 제안한다. 브랜치 생성·커밋·push는 요청받았을 때만 한다.
- 커밋 메시지는 `<종류>(<영역>): <내용>` 형식을 따르고, 문서·스킬·스타일 정리·동작 변경은 커밋을 나눈다.
- 경로별 리뷰 담당은 [CODEOWNERS](.github/CODEOWNERS)를 따른다. 팀별로 지정되지 않은 경로(`src/common/`, `tests/`, `docs/`, 빌드 설정, 스킬 등)는 `*` 규칙에 따라 세 팀 모두 리뷰어로 지정된다.
- 코드 작업 전에 [코드 컨벤션](docs/CODE_CONVENTIONS.md)을 읽는다. 동작 요구사항과 설계 배경은 [프로젝트 통합 문서](docs/프로젝트_통합문서.html)에서 확인한다.
- KISA DLL 호출 규약·export, x86/x64 파이프 메시지 배치, JSON 설정·CSV 결과와 재개 호환성을 유지한다. 계약을 바꾸는 작업은 영향을 받는 양쪽 호출부와 기존 데이터도 함께 검토한다.
- FVC 원본, 캐시, 생성 템플릿과 로컬 결과는 저장소에 추가하지 않는다. 재현에는 DB 이름, 설정, seed, DLL 아키텍처와 집계 지표를 기록한다.

## 문서의 기준

- 현재 작업 규칙은 이 문서, 스타일과 호환성은 `docs/CODE_CONVENTIONS.md`, 스킬 선택은 `docs/SKILLS.md`, 실행 절차는 해당 `SKILL.md`에서 관리한다. 같은 절차를 여러 문서에 복제하지 않는다.
- `팀프로젝트_전체계획.*`와 `코드작성계획_동작명세.html`은 초기 계획이다. GoogleTest, miniz, `dev` 브랜치, 예시 JSON·클래스 구조를 현재 구현으로 간주하지 않는다. 현재 빌드·실행 계약은 소스, `CMakeLists.txt`, `build.ps1`, README와 대조한다.
- `프로젝트_통합문서.html`의 완료 표시·측정값은 기준일 당시 기록이다. 요구사항과 구현이 다르면 차이를 명시하고, 구현에 맞추려고 과제 요구사항을 임의로 낮추지 않는다. 이번 작업에서 실행하지 않은 검증은 통과로 보고하지 않는다.
- 스킬 수정 시 `.agents/skills/<이름>/SKILL.md`와 `.claude/skills/<이름>/SKILL.md`를 함께 갱신하고 `docs/SKILLS.md`의 선택표·상대 링크와 `docs/skill-sources/manifest.json`의 출처를 확인한다. 외부 스킬·플러그인의 설치 여부는 현재 환경에서 확인한다.

## 작업에 맞는 스킬 선택

작업을 시작할 때 [docs/SKILLS.md](docs/SKILLS.md)를 열어 변경 경로와 작업 내용에 맞는 스킬을 고르고, 고른 스킬의 `SKILL.md`를 읽어 절차를 적용한다. 스킬 목록과 선택 기준은 그 문서에서만 관리하며 이 문서에 복제하지 않는다. Claude Code는 `.claude/skills/`, Codex는 `.agents/skills/`의 사본을 사용하고, 자동으로 선택되지 않으면 해당 파일을 직접 읽는다.

코드 변경 결과에는 실행한 검증과 실제 결과를 기록한다. 필요한 검증이 환경 제약으로 빠졌다면 그 항목을 적는다. [PR 체크리스트](.github/pull_request_template.md)
