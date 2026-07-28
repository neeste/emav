[Setup]
AppName=PUTT
AppVerName=BTNRH PUTT 2.38
AppPublisher=Boys Town Nationial Research Hospital
AppPublisherURL=http://audres.org/
AppSupportURL=http://audres.org/rc/putt/
AppUpdatesURL=http://audres.org/downloads/putt-setup.zip
DefaultDirName={pf}\BTNRH\PUTT
DefaultGroupName=BTNRH

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; MinVersion: 4,4

[Files]
Source: "..\VS9\Release\aputt.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "putt.ini"; DestDir: "{app}"; Flags: promptifolder
Source: "swp.ils"; DestDir: "{app}"; Flags: ignoreversion
Source: "probe.fnf"; DestDir: "{app}"; Flags: ignoreversion
Source: "96h17*.*"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\BTNRH\PUTT"; Filename: "{app}\aputt.exe"; WorkingDir: "{app}"
Name: "{userdesktop}\PUTT"; Filename: "{app}\aputt.exe"; WorkingDir: "{app}"; MinVersion: 4,4; Tasks: desktopicon

[Run]
Filename: "{app}\aputt.exe"; Description: "Launch PUTT?"; Flags: nowait postinstall skipifsilent

