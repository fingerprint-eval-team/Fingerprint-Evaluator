; FPEval 설치 패키지 (Inno Setup 6)
; 빌드: build.ps1 실행 후  "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\FPEval.iss
; 정적 런타임(/MT)으로 빌드하므로 VC++ 재배포 패키지는 필요 없다.

#define AppName "FPEval"
#define AppVersion "0.1.0"

[Setup]
AppId={{6B0F2E3A-9C1D-4E57-9B7A-FPEVAL000001}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=SME3007 Team
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
OutputDir=..\dist
OutputBaseFilename=FPEval_Setup_{#AppVersion}
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
; 관리자 권한이 없으면 "현재 사용자만" 설치(%LOCALAPPDATA%\Programs) 선택 가능
PrivilegesRequiredOverridesAllowed=dialog commandline

[Languages]
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"

[Files]
Source: "..\bin\FPEval.exe";      DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\FPEvalCli.exe";   DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\FPWorker64.exe";  DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\FPWorker32.exe";  DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\plugins\x64\*.dll"; DestDir: "{app}\plugins\x64"; Flags: ignoreversion
Source: "..\bin\plugins\x86\*.dll"; DestDir: "{app}\plugins\x86"; Flags: ignoreversion
Source: "..\README.md";           DestDir: "{app}"; Flags: ignoreversion isreadme

; 결과 폴더·설정 파일은 프로그램이 처음 실행될 때 실행한 사용자의 문서\FPEval에 직접 만든다.
; (설치 프로그램이 만들면 관리자 계정으로 설치할 때 다른 사용자의 문서 폴더에 생길 수 있음)

[Icons]
Name: "{group}\FPEval";        Filename: "{app}\FPEval.exe"
Name: "{group}\FPEval 제거";   Filename: "{uninstallexe}"
Name: "{autodesktop}\FPEval";  Filename: "{app}\FPEval.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "바탕 화면에 아이콘 만들기"; Flags: unchecked

[Run]
Filename: "{app}\FPEval.exe"; Description: "FPEval 실행"; Flags: nowait postinstall skipifsilent
