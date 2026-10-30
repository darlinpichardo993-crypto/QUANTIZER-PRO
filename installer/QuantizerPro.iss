; Quantizer Pro - instalador (icono: ojo amarillo)
#define AppName "Quantizer Pro"
#define AppVer  "1.0.0"
#define Vst3Dir "{commoncf64}\VST3\Quantizer Pro.vst3"

[Setup]
AppId={{7B5E0C1A-2F4D-4C7E-9A11-0A51B0A1A001}
AppName={#AppName}
AppVersion={#AppVer}
AppPublisher=ORYMY / YMY Records
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
OutputBaseFilename=QuantizerPro_Setup
SetupIconFile=..\assets\icon.ico
UninstallDisplayIcon={#Vst3Dir}\icon.ico
WizardStyle=modern
WizardImageFile=wizard.bmp
WizardSmallImageFile=wizard_small.bmp
Compression=lzma2
SolidCompression=yes

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Dirs]
Name: "{#Vst3Dir}"

[Files]
Source: "..\build\QuantizerPro_artefacts\Release\VST3\Quantizer Pro.vst3\*"; DestDir: "{#Vst3Dir}"; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "..\assets\icon.ico"; DestDir: "{#Vst3Dir}"; Flags: ignoreversion

[Messages]
spanish.WelcomeLabel2=Se instalará [name/ver] (VST3) para tu DAW.%n%nQuantizer Pro - Made by ORYMY · YMY Records.%n%nCierra FL Studio antes de continuar y luego escanea los plugins.
