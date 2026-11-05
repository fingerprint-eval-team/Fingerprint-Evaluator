---
name: "fpeval-performance-profiling"
description: "Profile Fingerprint-Evaluator CPU, memory, allocation, IPC or worker-scaling bottlenecks and verify performance changes. Use for speed or memory work; accuracy tuning uses algorithm-experiments."
license: "MIT"
---

# CPU·메모리·처리량 측정

## 측정 우선

1. 목표를 전체 평가 wall time, 추출/비교 지연, 처리량, peak memory 중에서 정한다. DB·규칙·seed·Worker·timeout·DLL을 고정하고 Release 최적화와 기호 정보를 확인한다.
2. cold cache와 warm cache를 구분하고 동일 조건에서 여러 번 측정한다. 반복 횟수와 요약 방식(중앙값·범위 등)을 적는다. 다른 프로그램 부하·CPU·전원 설정처럼 비교에 영향을 주는 조건도 기록한다.
3. `elapsedUs`는 Worker의 DLL 호출 구간이다. 이미지 디코딩·프로세스 시작·IPC·파일 I/O·리포트 시간을 설명하지 못한다. 전체 시간과 단계 시간을 함께 보고 병목 층을 정한다.
4. 설치된 Visual Studio profiler, VTune 또는 uProf를 사용한다. 프로파일러가 자식 Worker와 실제 DLL을 수집하는지 확인한다. 부모 GUI만 수집하고 매처가 빠르다고 결론 내리지 않는다.
5. CPU hotspot인지, 할당·메모리 대역폭인지, I/O·락 대기인지 구분하고 한 병목씩 바꾼다. Worker 수를 1/2/4/8로 비교하되 스케줄러 순서와 로그 순서는 정확도 비교 기준에서 제외한다.
6. 변경 후 같은 조건에서 속도·메모리를 다시 재고 상태·점수·템플릿 의미가 유지되는지 확인한다. SIMD·floating-point 최적화가 점수를 바꾸면 정확도 실험도 수행한다.

원문의 Linux perf·apt 명령과 보편적인 IPC 목표 수치는 적용하지 않는다. MSVC를 지원하는 실제 도구 기능만 사용한다. sanitizer 실행 시간과 정상 Release 성능을 비교하지 않는다.

완료 기록: 재현 명령·하드웨어·구성, 프로파일 근거, 전후 반복 측정, 정확도 회귀, 적용하지 않은 최적화의 이유.

## 출처와 적용 범위

2026-11-04에 확인한 공개 스킬을 프로젝트용으로 재구성한 적응본이다. 원문 그대로 설치한 버전이 아니다. 원문의 전체 플러그인·스크립트·다른 스킬을 요구하지 않으며, 위 절차만 독립적으로 적용한다. 일반 지침은 [CLAUDE.md](../../../CLAUDE.md)를 따른다.

- [aniruddhaadak80/skills/code-quality-performance-profiling-cpp](https://github.com/aniruddhaadak80/skills/blob/17f441875305220e7669ba1676bc61193d5a1a03/skills/code-quality-performance-profiling-cpp/SKILL.md) · commit `17f441875305` · [원문 라이선스](../../../docs/skill-sources/licenses/aniruddhaadak80--skills.txt)
- [mohitmishra786/low-level-dev-skills/intel-vtune-amd-uprof](https://github.com/mohitmishra786/low-level-dev-skills/blob/bdc58472fa9f309ed1b3f7d985a0d8e9bd8f4608/skills/profilers/intel-vtune-amd-uprof/SKILL.md) · commit `bdc58472fa9f` · [원문 라이선스](../../../docs/skill-sources/licenses/mohitmishra786--low-level-dev-skills.txt)

[후보 비교·변경 내역](../../../docs/skill-sources/README.md) · [작업별 선택표](../../../docs/SKILLS.md)
