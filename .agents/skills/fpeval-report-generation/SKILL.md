---
name: "fpeval-report-generation"
description: "Implement or validate Fingerprint-Evaluator ReportModel, HTML/DOCX writers, OOXML ZIP relationships, selected report items and report-only regeneration. Not a generic standalone Word authoring skill."
license: "MIT"
---

# HTML·OOXML 리포트 생성

## 모델과 출력 분리

`ReportModel`, `Evaluator::reportOnly`와 `Evaluator.cpp`의 내부 함수 `buildReport`, 두 Writer, `ZipWriter`를 읽는다. 같은 의미의 모델을 형식별로 출력하는 구조를 유지한다. 외부 DOCX/Office 런타임을 평가 프로그램 의존성으로 추가하지 않는다.

1. 완료된 전용 실행을 사용하고 보존할 결과는 복사한다. `FPEvalCli.exe --report-only "<실행 폴더>"`는 해당 폴더 설정과 로그에서 출력물을 재생성한다.
2. 항목 선택은 복사본 `config.json`의 `report.items`와 `report.formats` 또는 GUI 재생성 설정으로 검사한다. 현재 빈 items는 전체 항목으로 채워지는 정책이므로 빈 배열을 '모두 끔'으로 가정하지 않는다.
3. HTML/DOCX의 제목·선택 항목·표·지표·단위가 같은 모델과 일치하는지 확인한다. 한글, `&`, `<`, 따옴표, 긴 오류 메시지를 포함한다. HTML/XML escape와 잘못된 제어문자 처리를 검토한다.
4. DOCX ZIP의 `[Content_Types].xml`, `_rels/.rels`, `word/document.xml`, `word/_rels/document.xml.rels`, 참조 EMF를 확인한다. XML 파싱 성공, relationship ID/target의 일치, ZIP CRC와 실제 파일 존재를 검사한다.
5. 그림을 끄면 문서의 참조가 제거되어야 한다. 이전 실행의 미참조 그림 파일이 폴더에 남는 것만으로 실패로 판단하지 않는다. `plan.csv`·로그가 재생성 과정에서 바뀌지 않았는지 확인한다.
6. Word와 브라우저에서 표·한글·축·그림을 확인한다. 다른 렌더러가 있으면 호환성을 추가 확인하되 EMF 지원 차이를 구분한다. ZIP/XML 검사만 했으면 시각 검증 미실행을 적는다.

원문의 모델/렌더링 분리·표/그림 검증을 채택했다. 새 문서 템플릿 라이브러리 도입, 무조건적인 파일 byte equality는 요구하지 않는다. 시간 메타데이터와 출력 의미의 결정성을 구분한다.

완료 기록: 입력 실행, 항목/형식 조합, 구조 검사, 실제 렌더러·관찰 결과. 차트 내용은 `fpeval-chart-validation`, 수식 변경은 `fpeval-metrics-validation`을 사용한다.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [aniruddhaadak80/skills/document-generation-docx-report-generation](https://github.com/aniruddhaadak80/skills/blob/17f441875305220e7669ba1676bc61193d5a1a03/skills/document-generation-docx-report-generation/SKILL.md) · commit `17f441875305` · [원문 라이선스](../../../docs/skill-sources/licenses/aniruddhaadak80--skills.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
