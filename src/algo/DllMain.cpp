#include <windows.h>

HMODULE g_hDLL = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    g_hDLL = hModule;
    DisableThreadLibraryCalls(hModule);
  }
  return TRUE;
}

// 부속 파일이 필요할 때 DLL 자신의 경로를 구하는 방법 (안내서 p31). 현재 FPAlgo는 부속 파일을 쓰지 않는다.
void GetDllDirectoryPath(char* out, DWORD size) {
  GetModuleFileNameA(g_hDLL, out, size);
}
