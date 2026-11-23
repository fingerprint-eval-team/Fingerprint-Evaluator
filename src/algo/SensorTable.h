#pragma once
// iType → 이미지 크기 표. iType과 크기의 대응은 이 파일 한 곳에서만 관리한다.
// 과제 PDF ver1 6쪽의 FVC2000/2002/2004 할당값은 각각 1, 2, 3이다.
namespace fpa {

struct SensorSize {
  int iType;
  int width;
  int height;
};

constexpr SensorSize kSensorTable[] = {
    {1, 448, 478},  // FVC2000_DB3_B
    {2, 300, 300},  // FVC2002_DB3_B
    {3, 640, 480},  // FVC2004_DB1_B
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
