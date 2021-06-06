;
; MIT License
;
; Copyright(c) 2021 Alec Musasa
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files(the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions :
;
; The above copyright noticeand this permission notice shall be included in all
; copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.
;

#define MainExe "..\..\bin\pc_info64.exe"
#define RedistPath "..\..\files\redist\vs2019\vc\x64\"
#define Architecture "64"
#define ModulesPath "..\..\bin\"
#define OutputPath ModulesPath

#define MyAppName "PC Info"
#define MyAppNameCode "pc_info"
#define MyAppExeName "pc_info64.exe"
#define MyAppVersion GetVersionNumbersString('..\..\bin\pc_info64.exe')
#define MyProductVersion GetStringFileInfo("..\..\bin\pc_info64.exe", "ProductVersion")
#define MyAppPublisher "Alec Musasa"
#define MyAppPublisherPath "com.github.alecmus"
#define MyAppPublisherURL "https://alecmus.github.io"
#define MyAppURL "https://github.com/alecmus/pc_info"
#define MyAppUpdates "https://github.com/alecmus/pc_info/releases"
#define MyAppCopyright "© 2021 Alec Musasa"

[Messages]
// define wizard title and tray status msg
// both are normally defined in innosetup's default.isl (install folder)
SetupAppTitle = Setup - {#MyAppName} {#MyProductVersion} ({#Architecture} bit)  
SetupWindowTitle = Setup - {#MyAppName} {#MyProductVersion} ({#Architecture} bit)

[Setup]
; AppId uniquely identifies this app. No other app should use this ID.
AppId={{5F794184-4C64-402C-AE99-E88BBF681851}
AppName={#MyAppName}
AppVersion={#MyProductVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoVersion={#MyAppVersion}
AppCopyright={#MyAppCopyright}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppPublisherURL}
AppUpdatesURL={#MyAppUpdates}
DefaultDirName={localappdata}\{#MyAppPublisherPath}\{#MyAppNameCode}{#Architecture}
DisableDirPage=yes
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\License
OutputDir={#OutputPath}
OutputBaseFilename={#MyAppNameCode}{#Architecture}.setup.{#MyProductVersion}
;Installer rights
PrivilegesRequired=lowest
Compression=lzma2/max
SolidCompression=no
;Appearance of setup
WizardStyle=modern
SetupIconFile=..\resources\ico\icon.ico
;Restart Options
RestartIfNeededByRun=no
;Uninstall icon
UninstallDisplayIcon={uninstallexe}
;Name displayed in Add/Remove Programs
UninstallDisplayName={#MyAppName} ({#Architecture} bit)

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}";

[Files]
; pc_info
Source: "{#ModulesPath}pc_info64.exe"; DestDir: "{app}"; Flags: ignoreversion;

; liblec
Source: "{#ModulesPath}leccore64.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "{#ModulesPath}lecui64.dll"; DestDir: "{app}"; Flags: ignoreversion;

; additional dependencies
Source: "{#ModulesPath}sqlcipher64.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "{#ModulesPath}libeay64.dll"; DestDir: "{app}"; Flags: ignoreversion;

; vs2019 redistributable
Source: "{#RedistPath}msvcp140.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "{#RedistPath}vcruntime140.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "{#RedistPath}vcruntime140_1.dll"; DestDir: "{app}"; Flags: ignoreversion;

[Icons]

; my programs shortcuts
Name: "{group}\{#MyAppName} ({#Architecture} bit)"; Filename: "{app}\{#MyAppExeName}";

; desktop icons
Name: "{userdesktop}\{#MyAppName} ({#Architecture} bit)"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon;

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName} ({#Architecture} bit)}"; Flags: nowait postinstall skipifsilent
