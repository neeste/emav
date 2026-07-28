[Setup]
AppName=ABRAV
AppVerName=BTNRH ABRAV 1.32
AppPublisher=Boys Town Nationial Research Hospital
AppPublisherURL=http://audres.org/
AppSupportURL=http://audres.org/rc/abrav/
AppUpdatesURL=http://audres.org/downloads/abrav-setup.zip
DefaultDirName={pf}\BTNRH\ABRAV
DefaultGroupName=BTNRH

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; MinVersion: 4,4

[Files]
Source: "..\VS9\Release\aabrav.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "abrav.ini"; DestDir: "{app}"; Flags: promptifolder
Source: "93I14A0?.ABR"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\BTNRH\ABRAV"; Filename: "{app}\aabrav.exe"; WorkingDir: "{app}"
Name: "{userdesktop}\ABRAV"; Filename: "{app}\aabrav.exe"; WorkingDir: "{app}"; MinVersion: 4,4; Tasks: desktopicon

[Run]
Filename: "{app}\aabrav.exe"; Description: "Launch ABRAV?"; Flags: nowait postinstall skipifsilent

