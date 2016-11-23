; LAN Messenger Installation Script
; Version: 2.2
; Copyright (C) 2010-2011 Dilip Radhakrishnan
; Version History
; 1.0
;   -Initial release version
; 2.0
;   -Updated for new LAN Messenger(1.1.5 and above)
; 2.1
;   -Streamlined to include variable number of setup files
; 2.2
;   -Added support for silent install/uninstall
; 2.3
;	-Uses the /silent switch to execute the application exe silently
; 2.4
;	-Setup now takes care of the log folder while install/uninstall
; 2.5
;	-Added support for the /config switch to specify a config file during installation


;Define the compression to be used (lzma gives best compression)
SetCompressor /SOLID lzma

;------------------------------------------------------------------------------
;Includes

  !include MUI2.nsh
  !include nsDialogs.nsh
  !include "FileFunc.nsh"

  
;------------------------------------------------------------------------------
;Constants
  
  !define ProductName "LAN Messenger"
  !define CompanyName "LAN Messenger"
  !define ProductVersion "1.2.37"
  !define InstallerVersion "1.2.3.7"
  !define ProductUrl "http://lanmessenger.github.io"
  !define CompanyRegKey "SOFTWARE\${CompanyName}"
  !define AppRegKey "${CompanyRegKey}\${ProductName}"
  !define UninstKey "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${ProductName}"
  !define AppExec "lmc.exe"
  !define Uninstaller "uninst.exe"
  !define CompanyDataDir "$LOCALAPPDATA\${CompanyName}"
  !define AppDataDir "${CompanyDataDir}\${ProductName}"
  !define AppSettingsDir "$APPDATA\${CompanyName}"
  !define StartMenuDir "$SMPROGRAMS\${ProductName}"


;------------------------------------------------------------------------------
;General

  ;Name and file
  Name "${ProductName}"
  BrandingText $BrandText
  OutFile "lmc-${ProductVersion}-win32.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\${ProductName}"

  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "${AppRegKey}" "InstallDir"

  ;Request application privileges for Windows Vista and above
  RequestExecutionLevel admin

  ;Show details in the installation/uninstallation pages by default
  ShowInstDetails show
  ShowUninstDetails show
  
;------------------------------------------------------------------------------
;Interface Settings

  !define MUI_ICON "package\images\setup.ico"
  !define MUI_UNICON "package\images\setup.ico"
  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_BITMAP "package\images\header-r.bmp"
  !define MUI_HEADERIMAGE_UNBITMAP "package\images\header-r.bmp"
  !define MUI_HEADERIMAGE_RIGHT
  !define MUI_WELCOMEFINISHPAGE_BITMAP "package\images\banner.bmp"
  !define MUI_UNWELCOMEFINISHPAGE_BITMAP "package\images\banner.bmp"
  !define MUI_ABORTWARNING


;------------------------------------------------------------------------------
;Pages

  !define MUI_WELCOMEPAGE_TITLE_3LINES
  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "package\eula\license.txt"
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !define MUI_FINISHPAGE_TITLE_3LINES
  !define MUI_FINISHPAGE_RUN "$INSTDIR\${AppExec}"
  !define MUI_FINISHPAGE_RUN_TEXT "Start ${ProductName}"
  !define MUI_PAGE_CUSTOMFUNCTION_SHOW FinishPageShow
  !insertmacro MUI_PAGE_FINISH


  !define MUI_WELCOMEPAGE_TITLE_3LINES
  !define MUI_PAGE_CUSTOMFUNCTION_LEAVE un.WelcomePageLeave
  !insertmacro MUI_UNPAGE_WELCOME
  UninstPage custom un.OptionsPageShow un.OptionsPageLeave
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH


;------------------------------------------------------------------------------
;Languages

  ;Define Uninstall confirmation constant for fetching strings from language file.
  !insertmacro MUI_SET MUI_UNCONFIRMPAGE ""
  !insertmacro MUI_LANGUAGE "English"


;------------------------------------------------------------------------------
;Version Information

  VIProductVersion "${InstallerVersion}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${ProductName}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${CompanyName}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Copyright (c) 2010-2011 Dilip Radhakrishnan."
  VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${ProductName} Installer"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${ProductVersion}"


;------------------------------------------------------------------------------
;Global variables

  Var DeleteHistory
  Var DeleteSettings
  Var BrandText
  Var SilentMode
  Var ConfigFile
  
;------------------------------------------------------------------------------
;Installer Sections

Section
  
  DetailPrint "Installing ${ProductName}"
  
  DetailPrint "Copying new files"

  ;Set output path to the installation directory
  SetOutPath $INSTDIR
  
  ;Copy application files to the installation directory  
  File "..\..\release\${AppExec}"
  File "..\..\release\libeay32.dll"
  File "..\..\release\COPYING.txt"
  CreateDirectory "$INSTDIR\sounds"
  SetOutPath "$INSTDIR\sounds"
  File /r "..\..\release\sounds\*.*"
  CreateDirectory "$INSTDIR\lang"
  SetOutPath "$INSTDIR\lang"
  File /r "..\..\release\lang\*.*"
  CreateDirectory "$INSTDIR\themes"
  SetOutPath "$INSTDIR\themes"
  File /r "..\..\release\themes\*.*"
  
  ;Create directories in user's Application Data folder
  CreateDirectory "${AppDataDir}\cache"
  CreateDirectory "${AppDataDir}\lang"
  CreateDirectory "${AppDataDir}\themes"
  CreateDirectory "${AppDataDir}\logs"
  
  ;Create directory in user's Documents folder storing received files
  CreateDirectory "$DOCUMENTS\Received Files"
  
  ;Create uninstaller
  SetOutPath $INSTDIR
  WriteUninstaller "$INSTDIR\${Uninstaller}"

  ;Write application information into the registry
  DetailPrint "Creating Registry entries and keys..."
  WriteRegStr HKLM "${AppRegKey}" "InstallDir" $INSTDIR
  WriteRegStr HKLM "${AppRegKey}" "Version" "${ProductVersion}"
  WriteRegStr HKLM "${AppRegkey}" "FirstRun" "0"
  
  StrCpy $ConfigFile ""
  ${GetParameters} $R0
  ${GetOptions} $R0 "/config=" $R1
  IfErrors +2 +1
  StrCpy $ConfigFile "/config=$\"$R1$\""
  
  ;Write the application path into the startup application group in registry
  ;based on the value in the settings file. Calling application with /sync
  ;switch will handle this.
  ExecWait "$INSTDIR\${AppExec} $ConfigFile /silent /sync /quit"

  ;Write the uninstall keys for Windows  
  WriteRegStr HKLM "${UninstKey}" "DisplayName" "${ProductName}"
  WriteRegStr HKLM "${UninstKey}" "UninstallString" "$INSTDIR\${Uninstaller}"
  WriteRegStr HKLM "${UninstKey}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${UninstKey}" "DisplayIcon" "$INSTDIR\${AppExec},0"
  WriteRegStr HKLM "${UninstKey}" "DisplayVersion" "${ProductVersion}"
  WriteRegStr HKLM "${UninstKey}" "Publisher" "${CompanyName}"
  WriteRegStr HKLM "${UninstKey}" "URLInfoAbout" "${ProductUrl}"
  WriteRegStr HKLM "${UninstKey}" "URLUpdateInfo" "${ProductUrl}"
  WriteRegDWORD HKLM "${UninstKey}" "NoModify" 1
  WriteRegDWORD HKLM "${UninstKey}" "NoRepair" 1  
  
  DetailPrint "Adding Windows Firewall Exception '${ProductName}'"
  nsisFirewall::AddAuthorizedApplication "$INSTDIR\${AppExec}" "${ProductName}"
  Pop $0

  ;Create Start menu shortcuts (for all users)
  DetailPrint "Adding Start menu shortcuts"
  SetShellVarContext all
  CreateDirectory "${StartMenuDir}"
  CreateShortCut "${StartMenuDir}\${ProductName}.lnk" "$INSTDIR\${AppExec}" "" "$INSTDIR\${AppExec}" \
    0 SW_SHOWNORMAL "" "Send or receive instant messages."
  ;CreateShortCut "${StartMenuDir}\${ProductName} - Loopback.lnk" "$INSTDIR\${AppExec}" "/loopback" "$INSTDIR\${AppExec}" \
  ;  0 SW_SHOWNORMAL "" "Start ${ProductName} in loopback mode."
  CreateShortCut "${StartMenuDir}\Uninstall ${ProductName}.lnk" "$INSTDIR\${Uninstaller}" "" "$INSTDIR\${Uninstaller}" \
    0 SW_SHOWNORMAL "" "Uninstall ${ProductName}"
  SetShellVarContext current

SectionEnd


;------------------------------------------------------------------------------
;Uninstaller Section

Section "Uninstall"

  DetailPrint "Uninstalling ${ProductName}"
  
  ;Delete Start menu shortcuts
  DetailPrint "Removing Start menu shortcuts"
  SetShellVarContext all
  Delete "${StartMenuDir}\*.*"
  RMDir "${StartMenuDir}"
  SetShellVarContext current
  
  DetailPrint "Removing Windows Firewall Exception '${ProductName}'"
  nsisFirewall::RemoveAuthorizedApplication "$INSTDIR\${AppExec}"
  Pop $0
  
  ;Remove registry keys
  DetailPrint "Deleting registry entries and keys..."
  DeleteRegKey HKLM "${UninstKey}"
  DeleteRegKey HKLM "${AppRegKey}"
  DeleteRegKey /ifempty HKLM "${CompanyRegKey}"
  
  DetailPrint "Removing files and folders"
  ;Delete settings and history files if selected by user
  ExecWait "$INSTDIR\${AppExec} $DeleteHistory $DeleteSettings /silent /unsync /quit"

  RMDir "$DOCUMENTS\Received Files"
  
  RMDir /r "${AppDataDir}\cache"
  RMDir /r "${AppDataDir}\themes"
  RMDir /r "${AppDataDir}\lang"
  RMDir /r "${AppDataDir}\logs"
  
  RMDir "${AppDataDir}"
  RMDir "${CompanyDataDir}"
  RMDir "${AppSettingsDir}"
  
  ;Delete application files and folder
  Delete "$INSTDIR\${AppExec}"
  Delete "$INSTDIR\libeay32.dll"
  Delete "$INSTDIR\COPYING.txt"
  RMDir /r "$INSTDIR\sounds"
  RMDir /r "$INSTDIR\lang"
  RMDir /r "$INSTDIR\themes"
  Delete "$INSTDIR\${Uninstaller}"
  RMDir "$INSTDIR"
  
SectionEnd


;------------------------------------------------------------------------------
;Functions

  Function .onInit
  
    StrCpy $BrandText "${ProductName} v${ProductVersion} Installer"
    Call IsAlreadyInstalled
    
  FunctionEnd
  
  
  ;Check if the application is already installed. It must be uninstalled first.
  Function IsAlreadyInstalled
    
    ;Get uninstaller path of installed version. if any
    ReadRegStr $R0 HKLM "${UninstKey}" "UninstallString"
    StrCmp $R0 "" NotInstalled
    ;Ask user whether current installation should be removed or not. If not, abort installation
    MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
      "${ProductName} has already been installed on your computer.$\n$\n\
      Click OK to remove the previous version and continue this installation or Cancel to quit." \
      /SD IDOK IDOK Uninst
    Abort
    
    Uninst:
    ;Get previous install directory
    ReadRegStr $R1 HKLM "${UninstKey}" "InstallLocation"
    ClearErrors
    StrCpy $SilentMode ""
    ;If silent mode, jump to next line, else jump to line after next
    IfSilent +1 +2
    StrCpy $SilentMode "/S"
    ExecWait "$R0 $SilentMode _?=$R1" $0
    ;Check exit code of uninstaller to see if uninstallation was succesful, else abort installation
    IntCmp $0 0 0 UninstCancelled UninstCancelled
    IfErrors UninstError
    ;Delete uninstaller and install directory of previous version
    Delete "$R0"
    RMDir "$R1"
    goto NotInstalled
    
    UninstError:
    
    UninstCancelled:
    ;Abort setup after showing an error message
    MessageBox MB_OK|MB_ICONSTOP "Uninstallation of the previous version was cancelled.$\n$\nSetup will now exit." /SD IDOK
    Abort
    
    NotInstalled:
    
  FunctionEnd
  
  
  Function un.onInit
  
    StrCpy $BrandText "${ProductName} v${ProductVersion} Uninstaller"
    ;Page callback function will not be called for silent uninstall, so check for App running here
    IfSilent +1 +2
  	Call un.IsAppRunning
    
  FunctionEnd
  
  
  ;Check if the application is currently running. If running ask for user confirmation to close app.
  Function un.IsAppRunning
  
    ;Check if application is running.
    ;Run the application executable with /inst switch. Returns 0 if not running, 1 otherwise.
    ExecWait "$INSTDIR\${AppExec} /inst" $0
    IntCmp $0 0 NotRunning
    
    MessageBox MB_OKCANCEL|MB_ICONQUESTION \
      "${ProductName} must be closed to proceed with the uninstall.$\n$\n\
      Click OK to close ${ProductName} or Cancel to quit." \
      /SD IDOK IDOK CloseApp
    ;Set exit code to 1 (user cancel) and quit setup.
    SetErrorLevel 1
    Quit
    
    CloseApp:
    ;Run the application with /term switch to terminate the running instance.
    ExecWait "$INSTDIR\${AppExec} /silent /term"
    
    NotRunning:
    
  FunctionEnd
  
  
;------------------------------------------------------------------------------
;Custom Page Functions
  
  ;Interface variables
  Var OptionsPage
  Var OptionsPage.DirectoryText
  Var OptionsPage.Directory
  Var OptionsPage.DeleteHistory
  Var OptionsPage.DeleteHistory_State
  Var OptionsPage.DeleteSettings
  Var OptionsPage.DeleteSettings_State
  Var OptionsPage.Text
  Var FinishPage.FinishText
  
  
  ;Function called when installation finish page is shown
  Function FinishPageShow
  
    IntCmp $R8 0 NotAborted
    ${NSD_SetText} $mui.FinishPage.Title "${ProductName} was not installed"
    ${NSD_SetText} $mui.FinishPage.Text "The wizard was interrupted before ${ProductName} could be completely installed.$\n$\n\
        Your system has not been modified. To install this program at a later time, please run the installation again."
        
    SendMessage $mui.FinishPage.Run ${BM_SETCHECK} ${BST_UNCHECKED} 0
    ShowWindow $mui.FinishPage.Run ${SW_HIDE}
    
    ${NSD_CreateLabel} 120u 120u 195u 10u "Click Finish to close this wizard."
    Pop $FinishPage.FinishText
    SetCtlColors $FinishPage.FinishText "" "${MUI_BGCOLOR}"
    
    SetErrorLevel 1
    
    NotAborted:
    
  FunctionEnd
  
  
  ;Function called when user clicks Next on uninstall welcome page
  Function un.WelcomePageLeave
    
    Call un.IsAppRunning

  FunctionEnd  
  
  
  ;Displays a custom uninstallation confirm page.
  ;Checkboxes are shown giving the user the option to delete history and preferences when uninstalling.
  Function un.OptionsPageShow
    
    nsDialogs::Create 1018
    Pop $OptionsPage
    GetFunctionAddress $0 un.OptionsPageLeave
    nsDialogs::OnBack $0
    
    ${If} $OptionsPage == error
      Abort
    ${EndIf}
    
    ;Set header text and sub text
    !insertmacro MUI_HEADER_TEXT_PAGE $(MUI_UNTEXT_CONFIRM_TITLE) $(MUI_UNTEXT_CONFIRM_SUBTITLE)
    
    ;Add controls to the page
    ${NSD_CreateLabel} 0 0 100% 12u "${ProductName} will be uninstalled from the following location:"
    Pop $OptionsPage.DirectoryText
    
    ${NSD_CreateText} 0 13u 100% 12u "$INSTDIR"
    Pop $OptionsPage.Directory
    SendMessage $OptionsPage.Directory ${EM_SETREADONLY} 1 0
    
    ${NSD_CreateCheckBox} 0 40u 100% 10u "Delete history"
    Pop $OptionsPage.DeleteHistory
    ;Set checked state based on previous user input
    ${NSD_SetState} $OptionsPage.DeleteHistory $OptionsPage.DeleteHistory_State
    
    ${NSD_CreateCheckBox} 0 55u 100% 10u "Delete preferences"
    Pop $OptionsPage.DeleteSettings
    ;Set checked state based on previous user input
    ${NSD_SetState} $OptionsPage.DeleteSettings $OptionsPage.DeleteSettings_State
    
    ${NSD_CreateLabel} 0 -13u 100% 12u "Click Uninstall to continue."
    Pop $OptionsPage.Text
    
    nsDialogs::Show

  FunctionEnd

  
  ;Remembers the state of checkboxes when user navigates backward or forward from the page
  Function un.OptionsPageLeave

    ${NSD_GetState} $OptionsPage.DeleteHistory $OptionsPage.DeleteHistory_State
    ${If} $OptionsPage.DeleteHistory_State == ${BST_CHECKED}
      StrCpy $DeleteHistory "/nohistory /nofilehistory"
    ${Else}
      StrCpy $DeleteHistory ""
    ${EndIf}
    
    ${NSD_GetState} $OptionsPage.DeleteSettings $OptionsPage.DeleteSettings_State
    ${If} $OptionsPage.DeleteSettings_State == ${BST_CHECKED}
      StrCpy $DeleteSettings "/noconfig"
    ${Else}
      StrCpy $DeleteSettings ""
    ${EndIf}

  FunctionEnd