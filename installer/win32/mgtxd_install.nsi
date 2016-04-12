OutFile setup.exe
Unicode true
RequestExecutionLevel admin

!define APPNAME "Magic.TXD"
!define APPNAME_FRIENDLY "Magic TXD"

!define MUI_ICON "..\inst.ico"

!include "MUI2.nsh"
!include "Sections.nsh"
!include "x64.nsh"

InstallDir "$PROGRAMFILES\${APPNAME_FRIENDLY}"
Name "${APPNAME}"

var StartMenuFolder

!define COMPONENT_REG_PATH  "Software\Magic.TXD"

!define MUI_STARTMENUPAGE_REGISTRY_ROOT HKLM
!define MUI_STARTMENUPAGE_REGISTRY_KEY ${COMPONENT_REG_PATH}
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "SMFolder"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!define MUI_PAGE_CUSTOMFUNCTION_PRE pre_startmenu
!insertmacro MUI_PAGE_STARTMENU SM_PAGE_ID $StartMenuFolder
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

!include LogicLib.nsh

!macro CHECK_ADMIN
UserInfo::GetAccountType
pop $0
${If} $0 != "admin"
    MessageBox mb_iconstop "Please run this setup using administrator rights."
    SetErrorLevel 740
    Quit
${EndIf}
!macroend

!macro UPDATE_INSTDIR
${If} ${RunningX64}
    StrCpy $INSTDIR "$PROGRAMFILES64\${APPNAME_FRIENDLY}"
${Else}
    StrCpy $INSTDIR "$PROGRAMFILES\${APPNAME_FRIENDLY}"
${EndIf}
!macroend

!macro INCLUDE_FORMATS dir
File "${dir}\a8.magf"
File "${dir}\v8u8.magf"
!macroend

!macro SHARED_INSTALL
setOutPath $INSTDIR\resources
File /r "..\..\resources\*"
setOutPath $INSTDIR\data
File /r "..\..\data\*"
setOutPath $INSTDIR\languages
File /r "..\..\languages\*"
${If} ${RunningX64}
    setOutPath $INSTDIR\formats_x64
    !insertmacro INCLUDE_FORMATS "..\..\output\formats_x64"
${Else}
    setOutPath $INSTDIR\formats
    !insertmacro INCLUDE_FORMATS "..\..\output\formats"
${EndIf}
setOutPath $INSTDIR
${If} ${RunningX64}
    File "..\..\rwlib\vendor\pvrtexlib\Windows_x86_64\Dynamic\PVRTexLib.dll"
${Else}
    File "..\..\rwlib\vendor\pvrtexlib\Windows_x86_32\Dynamic\PVRTexLib.dll"
${EndIf}
File /r "..\..\releasefiles\*"
!macroend

!macro REG_INIT
# Access the proper registry view.
${If} ${RunningX64}
    SetRegView 64
${Else}
    SetRegView 32
${EndIf}
!macroend

VAR DoStartMenu

!define INST_REG_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Magic.TXD"
!define SM_PATH "$SMPROGRAMS\$StartMenuFolder"

!define REDIST32_120 "https://download.microsoft.com/download/2/E/6/2E61CFA4-993B-4DD4-91DA-3737CD5CD6E3/vcredist_x86.exe"
!define REDIST64_120 "https://download.microsoft.com/download/2/E/6/2E61CFA4-993B-4DD4-91DA-3737CD5CD6E3/vcredist_x64.exe"

!define REDIST32_120_KEY "SOFTWARE\Microsoft\DevDiv\vc\Servicing\12.0\RuntimeMinimum"
!define REDIST64_120_KEY "SOFTWARE\Microsoft\DevDiv\vc\Servicing\12.0\RuntimeMinimum"

!macro INSTALL_REDIST name friendlyname url32 url64 key32 key64
    # Check whether the user has to install a redistributable and which.
    StrCpy $3 0
    StrCpy $4 "HTTP://REDIST.PATH/"
    
    ${If} ${RunningX64}
        ReadRegDWORD $3 HKLM "${key64}" "Install"
        StrCpy $4 ${url64}
    ${Else}
        ReadRegDWORD $3 HKLM "${key32}" "Install"
        StrCpy $4 ${url32}
    ${EndIf}
    
    ${If} $3 == 0
    ${OrIf} $3 == ""
        # If required, install the redistributable.
        StrCpy $1 "$TEMP\${name}"
        
        NSISdl::download $4 $1
        
        pop $0
        ${If} $0 != "success"
            MessageBox mb_iconstop "Failed to download the ${friendlyname} redistributable. Please verify connection to the Internet and try again."
            SetErrorLevel 2
            Quit
        ${EndIf}
        
        # Run the redistributable installer.
        ExecWait "$1 /install /passive" $0
        
        # Delete the installer once we are done.
        Delete $1
        
        ${If} $0 != 0
            MessageBox mb_iconstop "Installation of the ${friendlyname} redistributable was not successful. Please try installing again."
            Quit
        ${EndIf}
    ${EndIf}
!macroend

Section "-Magic.TXD core"
    # Install all required redistributables.
    !insertmacro INSTALL_REDIST "vcredist_2013.exe" "VS2013" ${REDIST32_120} ${REDIST64_120} ${REDIST32_120_KEY} ${REDIST64_120_KEY}

    # Install the main program.
    setOutPath $INSTDIR
    ${If} ${RunningX64}
        File /oname=magictxd.exe "..\..\output\magictxd_x64.exe"
    ${Else}
        File /oname=magictxd.exe "..\..\output\magictxd.exe"
    ${EndIf}
    !insertmacro SHARED_INSTALL
    WriteUninstaller $INSTDIR\uinst.exe
    
    # Information for add/remove programs.
    WriteRegStr HKLM ${INST_REG_KEY} "DisplayName" "Magic.TXD"
    WriteRegStr HKLM ${INST_REG_KEY} "DisplayIcon" "$INSTDIR\magictxd.exe"
    WriteRegStr HKLM ${INST_REG_KEY} "DisplayVersion" "1.0"
    WriteRegDWORD HKLM ${INST_REG_KEY} "NoModify" 1
    WriteRegDWORD HKLM ${INST_REG_KEY} "NoRepair" 1
    WriteRegStr HKLM ${INST_REG_KEY} "Publisher" "GTA community"
    WriteRegStr HKLM ${INST_REG_KEY} "UninstallString" "$\"$INSTDIR\uinst.exe$\""
    
    # Write meta information.
    WriteRegStr HKLM ${COMPONENT_REG_PATH} "InstallDir" "$INSTDIR"
SectionEnd

Section /o "Startmenu Shortcuts" STARTMEN_SEC_ID
    !insertmacro MUI_STARTMENU_WRITE_BEGIN SM_PAGE_ID
        createDirectory "${SM_PATH}"
        createShortcut "${SM_PATH}\Magic.TXD.lnk" "$INSTDIR\magictxd.exe"
        createShortcut "${SM_PATH}\Remove Magic.TXD.lnk" "$INSTDIR\uinst.exe"
    !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

!define TXD_ASSOC_KEY ".txd"
!define MGTXD_TXD_ASSOC "${APPNAME}.txd"

Section "Associate .txd files" ASSOC_TXD_ID
    # KILL previous UserChoice config.
    DeleteRegKey HKCU SOFTWARE\Microsoft\Windows\Roaming\OpenWith\FileExts\.TXD
    DeleteRegKey HKCU Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.TXD

    # Write some registry settings for .TXD files.
    WriteRegStr HKCR "${TXD_ASSOC_KEY}" "" "${MGTXD_TXD_ASSOC}"
    WriteRegStr HKCR "${TXD_ASSOC_KEY}" "Content Type" "image/dict"
    WriteRegStr HKCR "${TXD_ASSOC_KEY}" "PerceivedType" "image"
    WriteRegStr HKCR "${TXD_ASSOC_KEY}\OpenWithProgids" "${MGTXD_TXD_ASSOC}" ""
    
    # Now write Magic.TXD association information.
    WriteRegStr HKCR "${MGTXD_TXD_ASSOC}\DefaultIcon" "" "$INSTDIR\magictxd.exe"
    WriteRegStr HKCR "${MGTXD_TXD_ASSOC}\shell\open\command" "" "$\"$INSTDIR\magictxd.exe$\" $\"%1$\""
SectionEnd

Section "Shell Integration" SHELL_INT_ID
    ${If} ${RunningX64}
        File /oname=rwshell.dll "..\..\thumbnail\bin\rwshell_x64.dll"
    ${Else}
        File /oname=rwshell.dll "..\..\thumbnail\bin\rwshell.dll"
    ${EndIf}
    
    ; Do the intergration.
    ExecWait 'regsvr32.exe /s "$INSTDIR\rwshell.dll"'
SectionEnd

LangString DESC_SMShortcut ${LANG_ENGLISH} "Creates shortcuts to Magic.TXD in the startmenu."
LangString DESC_TXDAssoc ${LANG_ENGLISH} "Associates .txd files in Windows Explorer with Magic.TXD."
LangString DESC_ShellInt ${LANG_ENGLISH} "Provides thumbnails and context menu extensions for TXD files."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${STARTMEN_SEC_ID} $(DESC_SMShortcut)
    !insertmacro MUI_DESCRIPTION_TEXT ${ASSOC_TXD_ID} $(DESC_TXDAssoc)
    !insertmacro MUI_DESCRIPTION_TEXT ${SHELL_INT_ID} $(DESC_ShellInt)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Function .onInit
    !insertmacro CHECK_ADMIN
    !insertmacro REG_INIT
    !insertmacro UPDATE_INSTDIR
    
    # Check through the registry whether Magic.TXD is installed already.
    # We do not want to install twice.
    StrCpy $0 ""
    ReadRegStr $0 HKLM ${COMPONENT_REG_PATH} "InstallDir"
    
    ${If} $0 != ""
        MessageBox MB_ICONINFORMATION|MB_YESNO "Setup has detected through the registry that Magic.TXD has already been installed at $\"$0$\".$\nSetup cannot continue unless the old version is uninstalled.$\n$\nWould you like to uninstall now?" /SD IDYES IDNO quitInstall
        
        # The user agreed to uninstall, so we run the uninstaller.
        # If the uninstaller suceeded, we continue setup.
        # Otherwise we show an error and quit.
        StrCpy $1 "$0\uinst.exe"
        
        ExecWait "$1 _?=$0" $1
        
        ${If} $1 != 0
            MessageBox MB_ICONSTOP|MB_OK "Failed to uninstall Magic.TXD. Please try uninstalling Magic.TXD again and running this installer afterward,"
            Quit
        ${EndIf}
        
        # Clean up after the uninstaller.
        # It could not complete a few steps.
        Delete "$0\uinst.exe"
        RMDIR "$0"
        
        # We were successful, so continue installation :)
        goto proceedInstall
quitInstall:
        Quit
proceedInstall:
    ${EndIf}
    
    StrCpy $DoStartMenu "false"
FunctionEnd

Function un.onInit
    !insertmacro CHECK_ADMIN
    !insertmacro REG_INIT

    # Try to fetch installation directory.
    StrCpy $0 ""
    ReadRegStr $0 HKLM ${COMPONENT_REG_PATH} "InstallDir"
    
    ${If} $0 != ""
        StrCpy $INSTDIR $0
    ${EndIf}
FunctionEnd

!macro IS_SHELL_RUNNING
    System::Call 'Kernel32::WaitForSingleObject( i r3, i 0 ) i.R2'
    StrCpy $0 0
    ${If} $R2 == 258
        # Shell is active.
        StrCpy $0 1
    ${EndIf}
!macroend

Section un.defUninst
    ; Unregister file association if present.
    DeleteRegKey HKCR "${MGTXD_TXD_ASSOC}"
    DeleteRegValue HKCR "${TXD_ASSOC_KEY}\OpenWithProgids" "${MGTXD_TXD_ASSOC}"
    
    IfFileExists $INSTDIR\rwshell.dll 0 uninstmain
    
    ; Unregister shell extension.
    ExecWait 'regsvr32.exe /u /s "$INSTDIR\rwshell.dll"'
    
    ; To properly unregister the shell extension we must restart the shell.
    ; We have to do this in the shell DLL that we travel with.
    StrCpy $2 0
    
    MessageBox MB_ICONQUESTION|MB_YESNO "Would you like to close Explorer shell to unregister the shell extension?$\nClosing the shell causes all Explorer windows to be lost.$\n$\nIt is RECOMMENDED to click no (only for experienced users)." /SD IDYES IDNO dontCloseExplorer
    StrCpy $2 1
dontCloseExplorer:

    # So maybe the user agreed to it.
    ${If} $2 == 1
        ReadRegDWORD $6 HKLM "Software\Microsoft\Windows NT\CurrentVersion\WinLogon" "AutoRestartShell"
        WriteRegDWORD HKLM "Software\Microsoft\Windows NT\CurrentVersion\WinLogon" "AutoRestartShell" 0
    
        # Close that bitch.
        FindWindow $0 "Shell_TrayWnd"
        System::Call 'User32::GetWindowThreadProcessId( i r0, *i .r1 )'
        System::Call 'Kernel32::OpenProcess( i 1048576, i 0, i r1 ) i.r3'
        SendMessage $0 0x5B4 0 0
        # SendMessage $0 WM_QUIT 0 0
        
        # We should wait until that friggin shell is dead.
waitSomeMoreForShellTermination:
        !insertmacro IS_SHELL_RUNNING
        StrCmp $0 0 proceedAfterShellTerm
        Sleep 1000
        goto waitSomeMoreForShellTermination
proceedAfterShellTerm:
        System::Call 'Kernel32::CloseHandle( i r3 )'
    ${EndIf}
    
    Delete /REBOOTOK $INSTDIR\rwshell.dll
    
    ${If} $2 == 1
        ; Restart the shell again, for the user :)
        ReadEnvStr $0 SYSTEMROOT
        Exec "$0\explorer.exe"
        
        Sleep 1500
        
        WriteRegDWORD HKLM "Software\Microsoft\Windows NT\CurrentVersion\WinLogon" "AutoRestartShell" $6
    ${EndIf}

uninstmain:
    RMDIR /r $INSTDIR\resources
    RMDIR /r $INSTDIR\data
    RMDIR /r $INSTDIR\licenses
    RMDIR /r $INSTDIR\languages
    RMDIR /r /REBOOTOK $INSTDIR\formats
    RMDIR /r /REBOOTOK $INSTDIR\formats_x64
    Delete /REBOOTOK $INSTDIR\PVRTexLib.dll
    Delete $INSTDIR\magictxd.exe
    Delete $INSTDIR\uinst.exe
    RMDIR $INSTDIR
    
    !insertmacro MUI_STARTMENU_GETFOLDER SM_PAGE_ID $StartMenuFolder
    
    ${If} $StartMenuFolder != ""
        ; Delete shortcut stuff.
        Delete "${SM_PATH}\Magic.TXD.lnk"
        Delete "${SM_PATH}\Remove Magic.TXD.lnk"
        RMDIR "${SM_PATH}"
    ${EndIf}
    
    # Remove the registry key that is responsible for shell stuff.
    DeleteRegKey /ifempty HKCR "${TXD_ASSOC_KEY}"
        
    DeleteRegKey HKLM ${INST_REG_KEY}
    DeleteRegKey HKLM ${COMPONENT_REG_PATH}
SectionEnd

Function pre_startmenu
    SectionGetFlags ${STARTMEN_SEC_ID} $R0
    IntOp $R0 $R0 & ${SF_SELECTED}

    ${If} $R0 != ${SF_SELECTED}
        abort
    ${EndIf}
FunctionEnd