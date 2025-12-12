[Setup]
AppName=Mayerism
; AppVersion will be replaced by the workflow if a git tag is used; fallback to 1.0.0
AppVersion={#MyAppVersion}
DefaultDirName={pf}\Mayerism
OutputBaseFilename=Mayerism-Installer-{#MyAppVersion}
Compression=lzma
SolidCompression=yes
DisableProgramGroupPage=yes
DisableDirPage=no
ChangesAssociations=no
Uninstallable=yes
; Reduce prompting during automated builds
DisableStartupPrompt=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; copy everything inside the .vst3 bundle to the VST3 directory and ensure the folder is created
Source: "output\Mayerism.vst3\*"; DestDir: "{commoncf}\VST3\Mayerism.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{commonprograms}\Mayerism\Uninstall Mayerism"; Filename: "{uninstallexe}"
