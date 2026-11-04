# FPEval 전체 빌드: x86(Worker32 + 32bit DLL) → x64(GUI, CLI, Worker64, 64bit DLL, 테스트)
# 사용법: powershell -ExecutionPolicy Bypass -File build.ps1 [-Config Release|Debug]
param([string]$Config = "Release")
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path

function Fail($msg) {
  Write-Host "`n[빌드 불가] $msg" -ForegroundColor Red
  exit 1
}

# 1) C++ 도구와 MFC가 모두 설치된 Visual Studio 찾기 (여러 버전이 있으면 그중 최신)
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
  Fail "Visual Studio가 설치되어 있지 않습니다. Visual Studio 2022 이상을 설치하고 'C++를 사용한 데스크톱 개발' 워크로드를 선택하세요."
}
$cpp = "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
$mfc = "Microsoft.VisualStudio.Component.VC.ATLMFC"
$vs = & $vswhere -latest -products * -requires $cpp $mfc -property installationPath
if (-not $vs) {
  $noMfc = & $vswhere -latest -products * -requires $cpp -property installationPath
  if ($noMfc) {
    Fail ("C++ MFC 구성 요소가 없습니다 ($noMfc).`n" +
          "  Visual Studio Installer → 수정 → 개별 구성 요소에서 'MFC'를 검색해 '최신 v14x 빌드 도구용 C++ MFC(x86 및 x64)'를 설치하세요.`n" +
          "  설치 전에 Visual Studio와 진행 중인 빌드를 모두 닫아야 합니다.")
  }
  Fail "C++ 빌드 도구가 없습니다. Visual Studio Installer에서 'C++를 사용한 데스크톱 개발' 워크로드를 설치하세요."
}

# 2) 그 Visual Studio에 맞는 CMake 생성기 (예: 17 → Visual Studio 17 2022)
$major = [int]((& $vswhere -path $vs -property installationVersion).Split('.')[0])
$year = @{ 16 = 2019; 17 = 2022; 18 = 2026 }[$major]
if (-not $year) { Fail "지원하지 않는 Visual Studio 버전입니다 ($major). Visual Studio 2022 이상을 사용하세요." }
$generator = "Visual Studio $major $year"
$cmake = Join-Path $vs "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not (Test-Path $cmake)) { $cmake = "cmake" }
Write-Host "Visual Studio: $vs ($generator)"

# 3) x86 → x64 순서로 구성·빌드
foreach ($arch in @("Win32", "x64")) {
  $bdir = Join-Path $root "build\$arch"
  & $cmake -S $root -B $bdir -G $generator -A $arch "-DCMAKE_GENERATOR_INSTANCE=$vs" | Out-Host
  if ($LASTEXITCODE -ne 0) {
    Fail "CMake 구성 실패 ($arch). 다른 Visual Studio로 빌드한 적이 있다면 build 폴더를 지우고 다시 실행하세요."
  }
  # /nodeReuse:false: 빌드 후 MSBuild 프로세스가 남지 않게 (남아 있으면 VS Installer 수정이 막힘)
  & $cmake --build $bdir --config $Config --parallel -- /nodeReuse:false | Out-Host
  if ($LASTEXITCODE -ne 0) { Fail "빌드 실패 ($arch). 위의 오류 메시지를 확인하세요." }
}
Write-Host "`n완료: $root\bin" -ForegroundColor Green
