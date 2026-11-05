---
name: "fpeval-code-review"
description: "Review Fingerprint-Evaluator local diffs or requested PR changes for concrete correctness, lifetime, compatibility and regression issues. Review-only by default; implement fixes when the user also asks for them."
license: "Apache-2.0"
---

# 변경 영향 중심 코드 리뷰

## 리뷰 범위

1. `git status --short`, staged/unstaged diff와 요청한 base를 확인한다. 통합 대상은 develop이며 GitHub 기본 브랜치를 무조건 기준으로 쓰지 않는다. 미추적 파일은 별도로 읽는다. 사용자가 남긴 변경을 보존한다.
2. 변경 파일의 전체 diff와 관련 호출부·직렬화 양쪽을 읽는다. 출력이 잘렸으면 파일별로 나눠 확인한다. 새 결함, 기존 결함, 아직 증거가 없는 의심을 구분한다.
3. 위험도가 큰 경계부터 검토한다: DLL/IPC 크기·호출 규약, parser 길이·overflow, 스레드 수명·취소, 저장/재개 호환성, 수식 분모, 리포트 escape·관계, 설치 파일 목록.
4. 각 의심은 실제 입력·호출 경로·발생 결과로 검증한다. 다른 계층에서 이미 방어하는지와 테스트가 해당 경로를 거치는지 확인한다. 단순 스타일 취향·근거 없는 가능성은 결함으로 올리지 않는다.
5. 관찰 가능한 회귀가 큰 순서로 파일/줄, 조건, 영향, 수정 방향을 적는다. 이슈가 없으면 없다고 말하고 검토 범위·실행하지 않은 검증을 남긴다.

원문의 Sentry 웹·Django·인증 체크리스트를 네이티브 DLL 평가 경계로 교체했다. GitHub 댓글·승인·메시지 전송, 자동 커밋은 하지 않는다. 리뷰만 요청받으면 코드 수정 대신 발견 사항을 보고하고, 수정도 요청받았다면 범위 안에서 고친 뒤 해당 영역 스킬로 검증한다.

완료 기록: 기준 diff, 읽은 경계, 확정 발견 사항, 수행/미수행 검증. 빠른 리뷰를 전체 보증으로 표현하지 않는다.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [getsentry/skills/code-review](https://github.com/getsentry/skills/blob/0c464f411bdfb8d4c12314b36264d06cb2f60270/skills/code-review/SKILL.md) · commit `0c464f411bdf` · [원문 라이선스](../../../docs/skill-sources/licenses/getsentry--skills.txt)
- [getsentry/skills/find-bugs](https://github.com/getsentry/skills/blob/0c464f411bdfb8d4c12314b36264d06cb2f60270/skills/find-bugs/SKILL.md) · commit `0c464f411bdf` · [원문 라이선스](../../../docs/skill-sources/licenses/getsentry--skills.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
