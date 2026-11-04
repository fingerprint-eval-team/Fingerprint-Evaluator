# Fingerprint-Evaluator

지문인식 알고리즘 성능 평가 프로그램과 K-NBTC 규격 지문인식 DLL.
고급객체지향프로그래밍(SME3007) 2026-2학기 팀프로젝트.

> **소스 코드는 `develop` 브랜치에 있다.** `main`에는 발표·제출 시점의 안정 버전만 병합한다.
>
> ```bash
> git clone -b develop https://github.com/fingerprint-eval-team/Fingerprint-Evaluator.git
> ```
>
> 이미 클론했다면 `git switch develop`

## 개발 대상

| 산출물 | 설명 |
|---|---|
| 지문인식 알고리즘 평가 프로그램 | Windows GUI(MFC). 알고리즘 DLL과 테스트 DB를 선택해 인식 성능을 평가하고 리포트(HTML·Word, 벡터 그래프)를 생성 |
| 지문인식 알고리즘 | Windows DLL. K-NBTC 지문인식 성능시험 규격(`KISA_Enroll`, `KISA_Attempt`, `KISA_Compare`) 호환 |
| 설치 패키지 | 평가 프로그램 배포용 설치 파일 |

- 언어: C/C++ (공개 라이브러리는 단위 기능에 한해 사용, 출처 명시)
- 시험 DB: FVC2000_DB3_B, FVC2002_DB3_B, FVC2004_DB1_B
- 평가 지표: FTE, FMR/FNMR, EER, EER*, ZeroFMR, ZeroFNMR, FMR100, FMR1000, 처리 시간, 템플릿 크기

## 역할

| 담당 | 영역 |
|---|---|
| A · 알고리즘 | 지문인식 DLL, 시험용 DLL, 성능 튜닝 |
| B · 평가 엔진 | Worker 프로세스·통신, 작업 분배(병렬·timeout·중지), 조합 규칙, 로그 |
| C · UI·리포트·배포 | MFC GUI, 지표 계산, 그래프·리포트, 설치 패키지 |

## 일정

| 주차 | 내용 |
|---|---|
| 12 | 진도 보고 |
| 13 | 중간 시연 발표 |
| 14 | 진도 보고 |
| 15 | 최종 발표, 최종 시연 |

## 브랜치 규칙

| 브랜치 | 용도 |
|---|---|
| `main` | 발표·제출 시점의 안정 버전만. PR로만 병합 |
| `develop` | 개발 통합 브랜치. 기능 브랜치를 여기에 병합 |
| `feature/<영역>-<내용>` | 기능 개발 (예: `feature/algo-local-matching`) |
| `fix/<영역>-<내용>` | 버그 수정 |
| `docs/<내용>` | 문서 |

버전 태그: `v0.1.0` 프로토타입 · `v0.2.0` 중간 시연 · `v1.0.0` 최종 제출

## 커밋 메시지

`<종류>(<영역>): <내용>` — 종류는 `feat`, `fix`, `docs`, `test`, `refactor`, `build`, `chore`

```
feat(algo): 로컬 구조 기반 미뉴셔 쌍 사전 필터 추가
fix(engine): 결과 폴더 이름 충돌 수정
```

## 문서

| 문서 | 내용 |
|---|---|
| [docs/팀프로젝트_전체계획.html](docs/팀프로젝트_전체계획.html) | 요구사항, 설계 개요, 역할, 일정, 리스크 |
| [docs/코드작성계획_동작명세.html](docs/코드작성계획_동작명세.html) | 동작 명세, 솔루션 구조, 클래스 설계, 코드 작성 순서 |
| [docs/프로젝트_통합문서.html](docs/프로젝트_통합문서.html) | 기술 명세, 현재 상태, 검증 결과, 남은 작업 |

## 참고 자료

- K-NBTC: https://www.ksecurity.or.kr/kisis/subIndex/99.do
- FVC2004: http://bias.csr.unibo.it/fvc2004/
