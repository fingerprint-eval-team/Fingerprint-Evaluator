# 과적합 방지와 고정된 평가

기존 세 B 데이터셋의 모든 이미지와 결과는 이미 개발에 노출되었다. 이들을 나눠서 진정한 미관측 최종 검증이라고 주장하지 않는다. 현재 독립 검증 완료 상태는 false이다.

## 이미지·지문 중복 감사

fp_protocol은 이미지 파일 SHA-256과 디코딩된 픽셀 SHA-256을 각각 기록한다. TIFF를 PNG로 바꾸거나 파일 이름을 바꿔도 같은 회색조 픽셀은 탐지할 수 있다. 같은 데이터셋 안의 지문 ID 중복도 검사한다.

```powershell
./build/Release/fp_protocol.exe scan ./data/FVC2000_DB3_B 1 ./inventory.tsv FVC2000_DB3_B
./build/Release/fp_protocol.exe check ./protocol/exposed-db1.tsv ./candidate-inventory.tsv
```

검사에서 겹치면 종료 코드 1, 형식 오류는 2이다. 겹침이 없다고 해서 사람 수준의 독립성이나 다른 촬영본까지 자동으로 증명하는 것은 아니다. 별도 출처와 사람/손가락 매핑을 확인해야 한다. 공격자가 내용을 조작하는 것을 막는 보안 시스템도 아니다.

protocol/exposed-db1~3.tsv에는 기존 240장의 노출 목록이 있다.

## 임계값 보정과 평가 분리

EER은 평가 점수 전체로 오류 곡선을 설명하는 지표이다. 배포용 임계값을 평가 정답으로 다시 고르는 것은 별개의 문제이므로 분리한다.

```powershell
./build/Release/fp_calibrate.exe fit ./development/pairs.tsv 0.01 ./frozen-threshold.tsv RELEASE_ID
./build/Release/fp_calibrate.exe evaluate ./frozen-threshold.tsv ./separate-evaluation/pairs.tsv ./fixed-result.json RELEASE_ID
```

RELEASE_ID에는 고정한 알고리즘 릴리스 식별자, 가급적 DLL과 종속성 해시를 사용한다. 도구는 식별자 불일치와 보정 파일 자체의 재사용을 거부하고, 평가 단계에서는 임계값 탐색을 하지 않는다. 파일 SHA-256 및 임계값 해시가 출력된다.

같은 이미지나 지문이 다른 행·파일에 섞이는 경우는 이 도구만으로 차단하지 못하므로, 위의 이미지/지문 감사와 출처 확인을 함께 사용해야 한다. 현재 실DB는 모두 개발용이므로 이 경로로 실행해도 독립 검증이라는 뜻은 아니다.

fit에서 선택하는 값은 관측된 개발 타인 수락률 제약을 만족하는 가장 낮은 임계값이다. 데이터 수가 작으면 낮은 FMR을 신뢰성 있게 추정할 수 없다. 타인 비교 720회에서 한 건은 약 0.139%이고, 같은 이미지·지문의 반복 쌍은 서로 독립인 표본으로 볼 수 없다. 단순한 비교 쌍 수를 독립 표본 수처럼 해석하지 않는다.

## 현재 고정 상수

모든 DB에서 동일하게 사용한다. 반경 80 px@500dpi, 8×8 공간 셀, 6 방향 구간, 좌표 허용 오차 14 px@500dpi, 방향 허용 오차 0.55 rad, 최대 128개 특징점. 처음 개발 결과를 본 뒤 DB별로 조절하지 않았다.

템플릿 v1에서 v2로의 변경은 손상 체크섬 및 범용 이미지 API 추가이며, 정합 상수와 수식은 변경하지 않았다. 평가 중 모델 갱신·이전 입력 기억·템플릿 누적은 없다.
