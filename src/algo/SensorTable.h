#pragma once
// iType → 이미지 크기 표. iType과 크기의 대응은 이 파일 한 곳에서만 관리한다.
//
// [미정] 과제 PDF는 FVC2002_DB3_B(300x300)와 FVC2004_DB1_B(640x480)에 모두 iType=2를 지정했지만,
//        DLL 함수에는 크기 인자가 없어 둘을 구분할 수 없다. 교수님 답변 전까지 프로토타입에서는
//        FVC2004_DB1_B를 임시로 iType=3으로 평가한다.
namespace fpa {

struct SensorSize {
  int iType;
  int width;
  int height;
};

constexpr SensorSize kSensorTable[] = {
    {1, 448, 478},  // FVC2000_DB3_B
    {2, 300, 300},  // FVC2002_DB3_B
    {3, 640, 480},  // FVC2004_DB1_B (임시 값, 위 주석 참고)
    {5, 280, 320},  // K-NBTC DIG00  IZZIX FD-100
    {7, 292, 525},  // K-NBTC PER00  UareU
    {8, 248, 292},  // K-NBTC NIT00  EyeD Hamster
    {9, 288, 320},  // K-NBTC HUN00  MagicSecure2500
};

inline bool lookupSensor(int iType, int& w, int& h) {
  for (const auto& s : kSensorTable) {
    if (s.iType == iType) {
      w = s.width;
      h = s.height;
      return true;
    }
  }
  return false;
}

}  // namespace fpa
