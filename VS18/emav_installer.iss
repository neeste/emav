[Setup]
AppName=EMAV
AppVersion=1.0
AppVerName=EMAV 1.0
DefaultDirName={pf}\EMAV
DefaultGroupName=EMAV
OutputDir=Output
OutputBaseFilename=EMAV_Setup
Compression=lzma
SolidCompression=yes

[Files]
; x86 (Win32) Executables
Source: "Release\aemav.exe"; DestDir: "{app}"; Check: IsNotArm64
Source: "Release\aputt.exe"; DestDir: "{app}"; Check: IsNotArm64

; ARM64 Executables
Source: "ARM64\Release\aemav.exe"; DestDir: "{app}"; DestName: "aemav.exe"; Check: IsArm64
Source: "ARM64\Release\aputt.exe"; DestDir: "{app}"; DestName: "aputt.exe"; Check: IsArm64

[Icons]
Name: "{group}\EMAV"; Filename: "{app}\aemav.exe"
Name: "{group}\Uninstall EMAV"; Filename: "{uninstallexe}"
Name: "{commondesktop}\EMAV"; Filename: "{app}\aemav.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Code]
function IsArm64: Boolean;
var
  Arch: String;
begin
  Arch := GetEnv('PROCESSOR_ARCHITEW6432');
  if Arch = '' then
    Arch := GetEnv('PROCESSOR_ARCHITECTURE');
  Result := CompareText(Arch, 'ARM64') = 0;
end;

function IsNotArm64: Boolean;
begin
  Result := not IsArm64;
end;
