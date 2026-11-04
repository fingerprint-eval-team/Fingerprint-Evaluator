#include "PathUtil.h"

#include <windows.h>
#include <shlobj.h>

#include <cstdio>
#include <cstring>

namespace fpe {

std::string toUtf8(const std::wstring& w) {
  if (w.empty()) return {};
  int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
  std::string s(n, '\0');
  WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
  return s;
}

std::wstring fromUtf8(const std::string& s) {
  if (s.empty()) return {};
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
  std::wstring w(n, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
  return w;
}

std::string acpToUtf8(const char* s) {
  if (!s || !*s) return {};
  int n = MultiByteToWideChar(CP_ACP, 0, s, -1, nullptr, 0);
  if (n <= 0) return {};
  std::wstring w(n - 1, L'\0');
  MultiByteToWideChar(CP_ACP, 0, s, -1, w.data(), n);
  return toUtf8(w);
}

std::wstring joinPath(const std::wstring& a, const std::wstring& b) {
  if (a.empty()) return b;
  wchar_t last = a.back();
  if (last == L'\\' || last == L'/') return a + b;
  return a + L"\\" + b;
}

std::wstring dirOf(const std::wstring& path) {
  size_t p = path.find_last_of(L"\\/");
  return p == std::wstring::npos ? L"." : path.substr(0, p);
}

std::wstring fileNameOf(const std::wstring& path) {
  size_t p = path.find_last_of(L"\\/");
  return p == std::wstring::npos ? path : path.substr(p + 1);
}

std::wstring exeDir() {
  wchar_t buf[MAX_PATH * 2] = {};
  GetModuleFileNameW(nullptr, buf, MAX_PATH * 2);
  return dirOf(buf);
}

std::wstring userDataDir() {
  std::wstring base;
  PWSTR docs = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_CREATE, nullptr, &docs))) base = docs;
  CoTaskMemFree(docs);
  if (base.empty()) base = exeDir();  // 문서 폴더를 못 찾는 경우의 대비
  std::wstring dir = joinPath(base, L"FPEval");
  makeDirs(dir);
  return dir;
}

bool fileExists(const std::wstring& p) {
  DWORD a = GetFileAttributesW(p.c_str());
  return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

bool dirExists(const std::wstring& p) {
  DWORD a = GetFileAttributesW(p.c_str());
  return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

bool makeDirs(const std::wstring& p) {
  if (p.empty() || dirExists(p)) return true;
  std::wstring parent = dirOf(p);
  if (parent != p && parent != L".") makeDirs(parent);
  return CreateDirectoryW(p.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool readFile(const std::wstring& p, std::vector<uint8_t>& out) {
  FILE* f = _wfopen(p.c_str(), L"rb");
  if (!f) return false;
  _fseeki64(f, 0, SEEK_END);
  long long n = _ftelli64(f);
  _fseeki64(f, 0, SEEK_SET);
  out.resize(n > 0 ? (size_t)n : 0);
  size_t r = n > 0 ? fread(out.data(), 1, out.size(), f) : 0;
  fclose(f);
  return r == out.size();
}

bool writeFile(const std::wstring& p, const void* data, size_t size) {
  FILE* f = _wfopen(p.c_str(), L"wb");
  if (!f) return false;
  size_t w = size ? fwrite(data, 1, size, f) : 0;
  fclose(f);
  return w == size;
}

bool writeText(const std::wstring& p, const std::string& utf8) { return writeFile(p, utf8.data(), utf8.size()); }

int peBitness(const std::wstring& dllPath) {
  std::vector<uint8_t> buf;
  FILE* f = _wfopen(dllPath.c_str(), L"rb");
  if (!f) return 0;
  buf.resize(4096);
  size_t n = fread(buf.data(), 1, buf.size(), f);
  fclose(f);
  if (n < 64 || buf[0] != 'M' || buf[1] != 'Z') return 0;
  uint32_t peOff = *reinterpret_cast<uint32_t*>(&buf[0x3C]);
  if (peOff + 6 > n || memcmp(&buf[peOff], "PE\0\0", 4) != 0) return 0;
  uint16_t machine = *reinterpret_cast<uint16_t*>(&buf[peOff + 4]);
  if (machine == IMAGE_FILE_MACHINE_I386) return 32;
  if (machine == IMAGE_FILE_MACHINE_AMD64) return 64;
  return 0;
}

std::wstring nowStamp() {
  SYSTEMTIME t;
  GetLocalTime(&t);
  wchar_t b[32];
  swprintf(b, 32, L"%04d%02d%02d_%02d%02d%02d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
  return b;
}

}  // namespace fpe
