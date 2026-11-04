# FPEval 전체 빌드: x86(Worker32 + 32bit DLL) → x64(GUI, CLI, Worker64, 64bit DLL, 테스트)
# 사용법: powershell -ExecutionPolicy Bypass -File build.ps1 [-Config Release|Debug]
param([string]$Config = "Release")
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmake = Join-Path $vs "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not (Test-Path $cmake)) { $cmake = "cmake" }

foreach ($arch in @("Win32", "x64")) {
  $bdir = Join-Path $root "build\$arch"
  & $cmake -S $root -B $bdir -A $arch | Out-Host
  if ($LASTEXITCODE -ne 0) { throw "cmake configure failed ($arch)" }
  # /nodeReuse:false: 빌드 후 MSBuild 프로세스가 남지 않게 (남아 있으면 VS Installer 수정이 막힘)
  & $cmake --build $bdir --config $Config --parallel -- /nodeReuse:false | Out-Host
  if ($LASTEXITCODE -ne 0) { throw "build failed ($arch)" }
}
Write-Host "`n완료: $root\bin"
