; FPEval 설치 패키지 (Inno Setup 6)
; 빌드: build.ps1 실행 후  "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\FPEval.iss
; 정적 런타임(/MT)으로 빌드하므로 VC++ 재배포 패키지는 필요 없다.

#define AppName "FPEval"
#define AppVersion "0.5.1"

; 시험 DB(testdb\FVC*)가 빌드 PC에 있으면 설치 파일에 넣고 파일 이름 끝에 _DB를 붙인다.
; DB 없이 만들려면: ISCC /DNoTestDb installer\FPEval.iss
#define TestDbDir AddBackslash(SourcePath) + "..\testdb\"
#ifndef NoTestDb
  #define Db2000 FileExists(TestDbDir + "FVC2000_DB3_B\101_1.tif")
  #define Db2002 FileExists(TestDbDir + "FVC2002_DB3_B\101_1.tif")
  #define Db2004 FileExists(TestDbDir + "FVC2004_DB1_B\101_1.tif")
#else
  #define Db2000 0
  #define Db2002 0
  #define Db2004 0
#endif
#if Db2000 || Db2002 || Db2004
  #define WithTestDb
  #define SetupSuffix "_DB"
#else
  #define SetupSuffix ""
#endif

[Setup]
AppId={{6B0F2E3A-9C1D-4E57-9B7A-FPEVAL000001}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=SME3007 Team
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
OutputDir=..\dist
OutputBaseFilename=FPEval_Setup_{#AppVersion}{#SetupSuffix}
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
; plugins의 FRFXLL.dll(FingerJetFX OSE, LGPL-3.0) 배포 조건: 고지문과 라이선스 원문 동봉
Source: "..\NOTICE.md";           DestDir: "{app}"; Flags: ignoreversion
Source: "..\third_party\FingerJetFXOSE\COPYRIGHT.txt";      DestDir: "{app}\licenses\FingerJetFXOSE"; Flags: ignoreversion
Source: "..\third_party\FingerJetFXOSE\COPYING.txt";        DestDir: "{app}\licenses\FingerJetFXOSE"; Flags: ignoreversion
Source: "..\third_party\FingerJetFXOSE\COPYING.LESSER.txt"; DestDir: "{app}\licenses\FingerJetFXOSE"; Flags: ignoreversion

; 시험 DB: GUI는 첫 실행 때 {app}\testdb\FVC* 중 첫 폴더를 기본 DB로 고른다 (MainDlg.cpp findDefaultDb).
; DB 포함본(_DB)은 FVC 이미지가 들어 있으므로 공개 GitHub Release에 올리지 않는다.
Source: "..\testdb\base.json";        DestDir: "{app}\testdb"; Flags: ignoreversion
Source: "..\testdb\pairs_sample.csv"; DestDir: "{app}\testdb"; Flags: ignoreversion
#if Db2000
Source: "..\testdb\FVC2000_DB3_B\*.tif"; DestDir: "{app}\testdb\FVC2000_DB3_B"; Tasks: testdb; Flags: ignoreversion
#endif
#if Db2002
Source: "..\testdb\FVC2002_DB3_B\*.tif"; DestDir: "{app}\testdb\FVC2002_DB3_B"; Tasks: testdb; Flags: ignoreversion
#endif
#if Db2004
Source: "..\testdb\FVC2004_DB1_B\*.tif"; DestDir: "{app}\testdb\FVC2004_DB1_B"; Tasks: testdb; Flags: ignoreversion
#endif

; 결과 폴더·설정 파일은 프로그램이 처음 실행될 때 실행한 사용자의 문서\FPEval에 직접 만든다.
; (설치 프로그램이 만들면 관리자 계정으로 설치할 때 다른 사용자의 문서 폴더에 생길 수 있음)

[Icons]
Name: "{group}\FPEval";        Filename: "{app}\FPEval.exe"
Name: "{group}\FPEval 제거";   Filename: "{uninstallexe}"
Name: "{autodesktop}\FPEval";  Filename: "{app}\FPEval.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "바탕 화면에 아이콘 만들기"; Flags: unchecked
#ifdef WithTestDb
Name: "testdb"; Description: "시험 DB(FVC) 설치"
#endif

[Run]
Filename: "{app}\FPEval.exe"; Description: "FPEval 실행"; Flags: nowait postinstall skipifsilent
