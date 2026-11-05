# Fingerprint-Evaluator 코드 컨벤션

이 문서는 현재 Windows C++17 코드의 스타일과 이 프로젝트에서 지켜야 할 호환성 기준을 정리한다. 새 코드와 수정하는 범위에 적용한다. 기존 이름이나 변환을 정리할 때는 해당 계약과 의미를 먼저 확인한다.

## 형식과 이름

- 공백 2칸으로 들여쓴다. 여는 중괄호는 선언이나 조건과 같은 줄에 둔다. `if`/`else if`/`else` 사슬은 모든 분기를 중괄호로 감싼다. 독립된 짧은 조건이나 반복문의 본문이 단순한 한 문장이면 한 줄 형식을 허용한다. 조건문이나 반복문의 본문이 여러 문장이면 문장마다 한 줄로 펼친다. 중첩되는 제어문에는 중괄호를 우선 사용한다.
- 타입과 클래스는 `PascalCase`, 일반 함수와 지역 변수는 `lowerCamelCase`, 새 상수는 `k` 접두사(`kMaxTemplateSize`)를 쓴다. 차트 좌표·색처럼 기존 짧은 이름을 쓰는 상수 묶음(`ChartRenderer`의 `W`/`H`, `ICanvas.h`의 `Ink`)은 같은 파일 안에서 기존 표기를 유지한다. 평가 코드의 클래스 멤버는 `value_`, MFC 멤버는 `m_value` 형태를 따른다. MFC 핸들러(`OnInitDialog`, `OnStart`), 리소스 ID(`IDC_START`), DLL export(`KISA_Enroll`)와 기존 도메인 표기(`EER`, `FMR`, `iType`)는 해당 API와 모듈의 표기를 따른다.
- 자체 헤더를 먼저 두고 시스템·표준 헤더와 나머지 프로젝트 헤더를 빈 줄로 구분한다. MFC `.cpp`는 `pch.h`를 먼저 둔다. 새로 쓰는 표준 타입과 함수의 선언은 해당 표준 헤더에서 가져온다. 헤더에 `#pragma once`를 쓰고, 전역 `using namespace`는 헤더에서 피한다.
- `fpe`는 평가·공용 코드, `fpa`는 인식 알고리즘의 네임스페이스다. 번역 단위 안에서만 쓰는 새 헬퍼는 이름 없는 네임스페이스에 둔다. 문자열·매크로·리소스 선언은 도구와 파일 형식이 요구하는 배치를 따른다. 인코딩은 UTF-8, 줄바꿈은 `.gitattributes`를 따른다.

```cpp
if (!enrollOk[job.a]) {
  skip = Status::SkipEF;
} else if (!attemptOk[job.b]) {
  skip = Status::SkipAF;
}
```

## 타입과 소유권

- 새로운 명시적 수치 변환에는 `static_cast`를 사용하고, 좁아지는 변환은 유효 범위를 확인한다. Win32 API·파이프·메시지의 포인터 변환은 실제 API 계약을 확인하고 필요한 곳으로 한정한다.
- 소유한 Win32 `HANDLE`에는 기존 RAII 래퍼인 `UniqueHandle`을 우선 사용한다. 소유 객체는 값이나 `std::unique_ptr`로 표현하고, 관찰용 포인터나 참조는 대상의 수명이 보장되는 범위에서 사용한다.
- Win32 파일 경로는 `std::wstring`과 `W` API를 사용한다. GUI 경계는 `CString`, UTF-8 로그·리포트 경계는 `PathUtil`의 기존 변환 함수를 활용한다.

## 계층과 오류 처리

- `src/core`와 `src/common`에 MFC 의존성을 넣지 않는다. GUI와 CLI는 같은 `Evaluator`를 사용하고, 진행 통지는 `IProgressListener`를 통해 전달한다. 평가 대상 DLL의 로드·호출은 Worker 내부에 유지한다.
- DLL ABI와 파이프에는 포인터, `size_t`, STL 객체처럼 프로세스나 아키텍처에 따라 의미·크기가 달라지는 값을 보내지 않는다. `Protocol.h`의 고정 크기 필드와 `pack(1)`을 유지하고, 배치를 바꾸면 크기 검증과 x86/x64 왕복 확인을 추가한다.
- DLL export 밖으로 C++ 예외를 내보내지 않는다. Worker의 DLL 호출 SEH, 출력 canary, 시간 제한과 상태 판정을 유지한다. DLL 실패와 평가 프로그램의 파일·통신 오류를 성공이나 점수 0으로 치환하지 않는다.
- `UniqueHandle`은 `CloseHandle` 대상에만 사용한다. GDI 객체, DC, DLL 모듈은 각 API에 맞는 해제 함수를 쓴다. 비동기 작업의 버퍼·핸들은 완료 또는 취소가 확인될 때까지 살아 있어야 한다.
- `CProgressBridge`의 `PostMessage` 페이로드는 수신 핸들러가 해제하는 소유권 이전이다. 관련 코드 변경 시 전송 실패, 창 종료, 미처리 메시지의 해제 경로도 검토한다. 작업 스레드가 컨트롤에 직접 접근하거나 창보다 오래 살아 있는 포인터를 사용하지 않게 한다.
- TIFF·JSON·CSV·템플릿·파이프 입력의 길이, 개수, 곱셈 오버플로와 인덱스를 사용 전에 검증한다. 기존 파서와 직렬화 도우미를 재사용하고, HTML/XML 출력에는 형식별 이스케이프를 적용한다.

## 모듈 계약

| 영역 | 확인할 계약 |
| --- | --- |
| `src/algo`, `src/worker`, `src/common` | `KisaApi.h`, `BioApi.h`, `.def`의 호출 규약과 export, `Protocol.h`의 필드 크기·배치, 결과 코드와 템플릿 한계 |
| `src/core/plan`, `src/core/metrics` | 같은 입력·규칙·seed에 대한 계획과 순서, 본인·타인 비교 조합, 지표 산식과 분모 |
| `src/core/config`, `src/core/result`, `src/core/exec` | 기존 JSON 키와 기본값, CSV 열과 상태값, 중지·재개에서의 작업 순서와 중복 처리 |
| `src/gui` | `.rc`의 컨트롤 ID와 `resource.h`·DDX·메시지 맵 연결, UI 스레드에서만 컨트롤 접근, `CProgressBridge`의 메시지 소유권 |
| `src/core/report`, `src/core/chart` | 공용 `ReportModel`과 `ICanvas`를 통한 출력, HTML/Word 및 SVG/EMF 간 항목과 수치의 일치 |

## 재현성과 저장 데이터

- 계획 생성은 기존 `Rng`와 하위 seed 분리를 사용한다. 무작위 순서·동일 이미지 제외·대칭 제거·작업 ID를 바꾸면 같은 입력의 계획 재현성과 저장된 `plan.csv`의 로드를 확인한다.
- 재개는 저장된 설정과 계획을 기준으로 한다. `ResultStore::load`는 같은 `jobId`의 마지막 기록을 사용하고, `isFinal`은 `ABORTED`·`INTERNAL`을 완료로 보지 않는다. CSV 원시 행 개수와 완료 작업 개수를 혼동하지 않는다.
- 지표의 `predefined`/`attempted`, EER/EER*를 구분한다. 현재 `attempted`는 등록 실패분을 제외하는 정책이며 단순한 DLL 호출 수가 아니다. `SKIP_EF`·`SKIP_AF`·호출 실패·`ABORTED`의 집계 의미를 보존한다. 임계값의 `>=`/`<`, 동점, 빈 입력, 분모 0은 작은 수치 예제로 확인한다.
- 로그의 `elapsedUs`는 마이크로초, 리포트 시간은 밀리초이며 비율과 퍼센트도 출력 경계에서 구분한다. 표시용 반올림을 계산 입력에 적용하지 않는다.

## 변경 범위와 커밋

- 중괄호·이름·헤더 순서 같은 스타일 정리는 동작 변경과 섞지 않고 별도 `refactor` 커밋으로 나눈다. 스타일 정리 커밋은 동작이 바뀌지 않아야 하며, 평가 결과에 영향을 줄 수 있는 변경은 같은 조건의 전후 실행으로 확인한다.
- 수정하는 함수나 블록 단위로 컨벤션을 적용한다. 관련 없는 파일 전체를 일괄 재서식하지 않는다.
- 문서·코드·빌드 변경도 가능한 한 커밋을 나눈다. 커밋 메시지와 브랜치 이름은 [README](../README.md#커밋-메시지) 규칙을 따른다.

## 검증 범위

코드 변경의 기본 확인은 `build.ps1`의 Win32/x64 빌드와 `build\x64\tests\CoreTests.exe` 직접 실행이다(CTest 미등록). 회귀 테스트는 `tests/CoreTests.cpp`의 기존 검사 방식에 맞춰 실제 결함·경계 조건을 다루며, GoogleTest를 이미 쓰는 것으로 가정하지 않는다. 문서만 바꾸면 링크·명령·스킬 동기화 확인으로 충분하다. 추가 확인은 변경 영역별 절차를 따른다. [프로젝트 스킬](SKILLS.md) · [PR 체크리스트](../.github/pull_request_template.md)
