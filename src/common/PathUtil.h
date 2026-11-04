#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace fpe {

std::string toUtf8(const std::wstring& w);
std::wstring fromUtf8(const std::string& s);
std::string acpToUtf8(const char* s);  // DLL이 돌려준 ANSI(CP949) 메시지 → UTF-8

std::wstring joinPath(const std::wstring& a, const std::wstring& b);
std::wstring dirOf(const std::wstring& path);
std::wstring fileNameOf(const std::wstring& path);
std::wstring exeDir();
// 사용자 데이터 폴더: 문서\FPEval (없으면 생성). 설치판은 Program Files에 쓸 수 없으므로
// 설정 파일·기본 결과 폴더는 여기에 둔다.
std::wstring userDataDir();
bool fileExists(const std::wstring& p);
bool dirExists(const std::wstring& p);
bool makeDirs(const std::wstring& p);

bool readFile(const std::wstring& p, std::vector<uint8_t>& out);
bool writeFile(const std::wstring& p, const void* data, size_t size);
bool writeText(const std::wstring& p, const std::string& utf8);

// PE 헤더 Machine 필드로 DLL bitness 판별: 32, 64, 0(알 수 없음)
int peBitness(const std::wstring& dllPath);

std::wstring nowStamp();  // yyyyMMdd_HHmmss

}  // namespace fpe
