[Setup]
AppName=EmAv
AppVerName=BTNRH EMAV 3.37
AppPublisher=Boys Town Nationial Research Hospital
AppPublisherURL=http://audres.org/
AppSupportURL=http://audres.org/rc/emav/
AppUpdatesURL=http://audres.org/downloads/emav-setup.zip
DefaultDirName={pf}\BTNRH\EMAV
DefaultGroupName=BTNRH

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; MinVersion: 4,4

[Files]
Source: "..\VS16\Release\aemav.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "emav.ini"; DestDir: "{app}"; Flags: promptifolder
Source: "std.lst"; DestDir: "{app}"; Flags: ignoreversion
Source: "fdpswp.lst"; DestDir: "{app}"; Flags: ignoreversion
Source: "suppr.lst"; DestDir: "{app}"; Flags: ignoreversion
Source: "92l21*.*"; DestDir: "{app}"; Flags: ignoreversion
Source: "93c25*.*"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\BTNRH\EmAv"; Filename: "{app}\aemav.exe"; WorkingDir: "{app}"
Name: "{userdesktop}\EmAv"; Filename: "{app}\aemav.exe"; WorkingDir: "{app}"; MinVersion: 4,4; Tasks: desktopicon

[Run]
Filename: "{app}\aemav.exe"; Description: "Launch EmAv?"; Flags: nowait postinstall skipifsilent

