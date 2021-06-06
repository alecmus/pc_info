#define MainExe "..\..\bin\pc_info32.exe"
#define RedistPath "..\..\files\redist\vs2019\vc\x86\"
#define Architecture "32"
#define ModulesPath "..\..\bin\"
#define OutputPath ModulesPath

#define MyAppName "PC Info"
#define MyAppNameCode "pc_info"
#define MyAppVersion GetVersionNumbersString('..\..\bin\pc_info32.exe')
#define MyAppPublisher "Alec Musasa"
#define MyAppURL "https://github.com/alecmus/pc_info"
#define MyAppCopyright "© 2021 Alec Musasa"

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{4366AB0F-A68F-4388-B4FA-2BE684F86FC4}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoVersion={#MyAppVersion}
AppCopyright={#MyAppCopyright}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={localappdata}\{#MyAppPublisher}\{#MyAppName}
DisableDirPage=yes
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\License
OutputDir={#OutputPath}
OutputBaseFilename={#MyAppNameCode}{#Architecture}.setup.{#MyAppVersion}
;Installer rights
PrivilegesRequired=lowest
Compression=lzma2/max
SolidCompression=yes
;Appearance of setup
WizardStyle=modern
SetupIconFile=..\resources\ico\icon.ico
;Restart Options
RestartIfNeededByRun=no
;Uninstall icon
UninstallDisplayIcon={uninstallexe}
;Name displayed in Add/Remove Programs
UninstallDisplayName={#MyAppName}{#Architecture}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}";

[Files]
; pc_info
Source: "{#ModulesPath}pc_info32.exe"; DestDir: "{app}"; Flags: ignoreversion;

; liblec
Source: "{#ModulesPath}leccore32.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "{#ModulesPath}lecui32.dll"; DestDir: "{app}"; Flags: ignoreversion;

; additional dependencies
Source: "{#ModulesPath}sqlcipher32.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "{#ModulesPath}libeay32.dll"; DestDir: "{app}"; Flags: ignoreversion;

; vs2019 redistributable
Source: "{#RedistPath}concrt140.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "{#RedistPath}msvcp140.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "{#RedistPath}msvcp140_1.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "{#RedistPath}msvcp140_2.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "{#RedistPath}vccorlib140.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "{#RedistPath}vcruntime140.dll"; DestDir: "{app}"; Flags: ignoreversion;
