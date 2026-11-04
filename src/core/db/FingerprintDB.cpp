#include "db/FingerprintDB.h"

#include <windows.h>

#include <algorithm>
#include <cwctype>

#include "PathUtil.h"

namespace fpe {

namespace {
bool parseName(const std::wstring& file, int& finger, int& index) {
  size_t dot = file.find_last_of(L'.');
  if (dot == std::wstring::npos) return false;
  std::wstring ext = file.substr(dot + 1);
  for (auto& c : ext) c = (wchar_t)towlower(c);
  if (ext != L"tif" && ext != L"tiff") return false;
  std::wstring stem = file.substr(0, dot);
  size_t us = stem.find(L'_');
  if (us == std::wstring::npos || us == 0 || us + 1 >= stem.size()) return false;
  for (size_t i = 0; i < stem.size(); ++i)
    if (i != us && !iswdigit(stem[i])) return false;
  finger = _wtoi(stem.substr(0, us).c_str());
  index = _wtoi(stem.substr(us + 1).c_str());
  return true;
}
}  // namespace

bool FingerprintDB::scan(const std::wstring& dir, std::string& err) {
  images_.clear();
  warnings_.clear();
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW(joinPath(dir, L"*").c_str(), &fd);
  if (h == INVALID_HANDLE_VALUE) {
    err = "cannot open folder " + toUtf8(dir);
    return false;
  }
  do {
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
    FingerprintImage img;
    if (!parseName(fd.cFileName, img.finger, img.index)) {
      warnings_.push_back("skipped: " + toUtf8(fd.cFileName));
      continue;
    }
    img.path = joinPath(dir, fd.cFileName);
    images_.push_back(img);
  } while (FindNextFileW(h, &fd));
  FindClose(h);
  if (images_.empty()) {
    err = "no {finger}_{index}.tif files in " + toUtf8(dir);
    return false;
  }
  index();
  return true;
}

void FingerprintDB::setImages(std::vector<FingerprintImage> imgs) {
  images_ = std::move(imgs);
  index();
}

void FingerprintDB::index() {
  std::sort(images_.begin(), images_.end(), [](const FingerprintImage& a, const FingerprintImage& b) {
    return a.finger != b.finger ? a.finger < b.finger : a.index < b.index;
  });
  byFinger_.clear();
  byName_.clear();
  for (int i = 0; i < (int)images_.size(); ++i) {
    byFinger_[images_[i].finger].push_back(i);
    byName_[images_[i].name()] = i;
  }
}

std::vector<int> FingerprintDB::fingers() const {
  std::vector<int> f;
  for (auto& kv : byFinger_) f.push_back(kv.first);
  return f;
}

std::vector<int> FingerprintDB::imagesOf(int finger) const {
  auto it = byFinger_.find(finger);
  return it == byFinger_.end() ? std::vector<int>{} : it->second;
}

int FingerprintDB::find(const std::string& name) const {
  auto it = byName_.find(name);
  return it == byName_.end() ? -1 : it->second;
}

}  // namespace fpe
