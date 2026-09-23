[Setup]
AppName=ABRAV
AppVerName=BTNRH ABRAV {#APP_VERSION}
AppPublisher=Boys Town National Research Hospital
AppPublisherURL=http://audres.org/
AppSupportURL=http://audres.org/rc/abrav/
AppUpdatesURL=http://audres.org/downloads/ABRAV_Setup.exe
DefaultDirName={autopf}\BTNRH\ABRAV
DefaultGroupName=BTNRH
OutputDir=Output
OutputBaseFilename=ABRAV_Setup
Compression=lzma
SolidCompression=yes

[Files]
; x86 (Win32) Executables
Source: "Release\aabrav.exe"; DestDir: "{app}"; Check: IsNotArm64; Flags: ignoreversion

; ARM64 Executables
Source: "ARM64\Release\aabrav.exe"; DestDir: "{app}"; DestName: "aabrav.exe"; Check: IsArm64; Flags: ignoreversion

; Configuration and Data Files
Source: "..\abrav\abrav.ini"; DestDir: "{app}"; Flags: promptifolder
Source: "..\abrav\93I14A0?.ABR"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\ABRAV"; Filename: "{app}\aabrav.exe"; WorkingDir: "{app}"
Name: "{group}\Uninstall ABRAV"; Filename: "{uninstallexe}"
Name: "{commondesktop}\ABRAV"; Filename: "{app}\aabrav.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{app}\aabrav.exe"; Description: "Launch ABRAV?"; Flags: nowait postinstall skipifsilent

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
