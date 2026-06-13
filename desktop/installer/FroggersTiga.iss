; Version and paths are passed by package-windows.ps1 via ISCC /D defines.
#ifndef MyAppVersion
  #error MyAppVersion must be set via ISCC /DMyAppVersion=x.y.z
#endif
#ifndef ReleaseDir
  #error ReleaseDir must be set via ISCC /DReleaseDir=...
#endif
#ifndef OutputDir
  #define OutputDir "..\dist"
#endif

#define MyAppName "FroggersTiga"
#define MyAppPublisher "FroggersTiga"
#define MyAppExeName "FroggersTiga.exe"

[Setup]
AppId={{A7B3C9D1-E4F2-4A8B-9C1D-2E3F4A5B6C7D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir={#OutputDir}
OutputBaseFilename=FroggersTiga-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#ReleaseDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
