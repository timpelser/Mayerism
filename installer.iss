[Setup]
AppName=Mayerism
; AppVersion will be replaced by the workflow if a git tag is used; fallback to 1.0.0
AppVersion=1.0.0
DefaultDirName={commoncf}\VST3\Mayerism
OutputBaseFilename=Mayerism-Installer-1.0.0
Compression=lzma
SolidCompression=yes
DisableProgramGroupPage=yes
DisableDirPage=yes
ChangesAssociations=no
Uninstallable=yes
; Reduce prompting during automated builds
DisableStartupPrompt=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; copy everything inside the .vst3 bundle to the VST3 directory and ensure the folder is created
Source: "output\Mayerism.vst3\*"; DestDir: "{commoncf}\VST3\Mayerism.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
