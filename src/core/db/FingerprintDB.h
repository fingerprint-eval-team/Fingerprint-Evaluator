#pragma once
#include <map>
#include <string>
#include <vector>

namespace fpe {

// 이미지 하나: 파일명 {손가락ID}_{순번}.tif
struct FingerprintImage {
  int finger = 0;
  int index = 0;
  std::wstring path;
  std::string name() const { return std::to_string(finger) + "_" + std::to_string(index); }
};

class FingerprintDB {
 public:
  // 폴더 스캔. 형식이 틀린 파일은 warnings에 기록하고 제외한다.
  bool scan(const std::wstring& dir, std::string& err);

  const std::vector<FingerprintImage>& images() const { return images_; }
  std::vector<int> fingers() const;
  // 손가락별 이미지 번호(images_ 인덱스), 순번 오름차순
  std::vector<int> imagesOf(int finger) const;
  int find(const std::string& name) const;  // 없으면 -1
  const std::vector<std::string>& warnings() const { return warnings_; }

  // 테스트용: 파일 없이 구성
  void setImages(std::vector<FingerprintImage> imgs);

 private:
  void index();
  std::vector<FingerprintImage> images_;
  std::map<int, std::vector<int>> byFinger_;
  std::map<std::string, int> byName_;
  std::vector<std::string> warnings_;
};

}  // namespace fpe
