#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace fpe {

// 무압축(stored) ZIP 작성기. DOCX(OOXML 패키지) 생성에 사용한다.
class ZipWriter {
 public:
  void add(const std::string& name, const std::string& data) { add(name, std::vector<uint8_t>(data.begin(), data.end())); }
  void add(const std::string& name, std::vector<uint8_t> data);
  std::vector<uint8_t> finish() const;
  static uint32_t crc32(const uint8_t* p, size_t n);

 private:
  struct Entry {
    std::string name;
    std::vector<uint8_t> data;
  };
  std::vector<Entry> entries_;
};

}  // namespace fpe
