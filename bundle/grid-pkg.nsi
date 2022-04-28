############################################################################################
#      NSIS Installation Script created by NSIS Quick Setup Script Generator v1.09.18
#               Entirely Edited with NullSoft Scriptable Installation System                
#              by Vlasis K. Barkas aka Red Wine red_wine@freemail.gr Sep 2006               
############################################################################################
!define APP_NAME "Summit Grid"
!define COMP_NAME "CentrillionTech"
!define WEB_SITE "www.centrilliontech.com.tw"
!define COPYRIGHT "CentrillionTech"
!define DESCRIPTION "Application"
!define INSTALLER_NAME "summit-grid-setup.exe"
!define MAIN_APP_EXE "summit-app-grid.exe"
!define INSTALL_TYPE "SetShellVarContext current"
!define REG_ROOT "HKCU"
!define REG_HKLM "HKLM"
!define REG_ENV_PATH "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"
!define REG_APP_PATH "Software\Microsoft\Windows\CurrentVersion\App Paths\${MAIN_APP_EXE}"
!define UNINSTALL_PATH "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
!define INPUT_DIR_PATH "..\stage"
!define INSTALL_DIR "$PROGRAMFILES64\${COMP_NAME}\${APP_NAME}"
!define ENV_PATH_BIN "${INSTALL_DIR}\bin;"

######################################################################

SetCompressor ZLIB
Name "${APP_NAME}"
Caption "${APP_NAME}"
OutFile "${INSTALLER_NAME}"
BrandingText "${APP_NAME}"
XPStyle on
InstallDirRegKey "${REG_ROOT}" "${REG_APP_PATH}" ""
InstallDir "${INSTALL_DIR}"

######################################################################

!include "x64.nsh"
!include "MUI.nsh"
!include "nsDialogs.nsh"
!include "StrFunc.nsh"
${StrRep}
${UnStrRep}

!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

!insertmacro MUI_PAGE_WELCOME

!ifdef LICENSE_TXT
!insertmacro MUI_PAGE_LICENSE "${LICENSE_TXT}"
!endif

!insertmacro MUI_PAGE_DIRECTORY

# Page custom magpieConfigPage magpieConfigLeave

!ifdef REG_START_MENU
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "${APP_NAME}"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${REG_ROOT}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${UNINSTALL_PATH}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${REG_START_MENU}"
!insertmacro MUI_PAGE_STARTMENU Application $SM_Folder
!endif

!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "English"

######################################################################

Function .onInit
    ${If} ${RunningX64}
    ${Else}
        MessageBox MB_OK "Summit project only support x64 Windows system."
        Abort 
    ${EndIf}
FunctionEnd

######################################################################

Section -MainProgram
${INSTALL_TYPE}
SetOverwrite ifnewer
SetOutPath "$INSTDIR"
File /r "${INPUT_DIR_PATH}\*"
Var /GLOBAL double_slash_instdir
${StrRep} $double_slash_instdir $INSTDIR '\' '\\'
SectionEnd

######################################################################

Var ICON_PATH 
Section -Icons_Reg
StrCpy $ICON_PATH "$INSTDIR\icon.ico"
SetOutPath "$INSTDIR"
WriteUninstaller "$INSTDIR\uninstall.exe"

!ifdef REG_START_MENU
!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
CreateDirectory "$SMPROGRAMS\$SM_Folder"
CreateShortCut "$SMPROGRAMS\$SM_Folder\${APP_NAME}.lnk" "C:\Windows\System32\cmd.exe" "/K summit-app-grid" "$ICON_PATH" 0
!insertmacro MUI_STARTMENU_WRITE_END
!endif

!ifndef REG_START_MENU
CreateDirectory "$SMPROGRAMS\${COMP_NAME}"
CreateShortCut "$SMPROGRAMS\${COMP_NAME}\${APP_NAME}.lnk" "C:\Windows\System32\cmd.exe" "/K summit-app-grid" "$ICON_PATH" 0
!endif

WriteRegStr ${REG_ROOT} "${REG_APP_PATH}" "" "$INSTDIR\${MAIN_APP_EXE}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}" "DisplayName" "${APP_NAME}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}" "UninstallString" "$INSTDIR\uninstall.exe"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}" "DisplayIcon" "$ICON_PATH"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}" "DisplayVersion" "${VERSION}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}" "Publisher" "${COMP_NAME}"

!ifdef WEB_SITE
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}" "URLInfoAbout" "${WEB_SITE}"
!endif
SectionEnd

######################################################################

Section -SetPATH
ReadRegStr $0 ${REG_HKLM} "${REG_ENV_PATH}" "Path"
WriteRegSTR ${REG_HKLM} "${REG_ENV_PATH}" "Path" "$0;$INSTDIR\bin;"
WriteRegSTR ${REG_HKLM} "${REG_ENV_PATH}" "GRID" '"$INSTDIR\bin\summit-app-grid.exe"'
nsExec::Exec 'setx /m GRID_CHECK 0'
SectionEnd

######################################################################

Section Uninstall
${INSTALL_TYPE}

RMDir /r /REBOOTOK "$INSTDIR"
Delete "$INSTDIR\uninstall.exe"
RmDir "$INSTDIR"

!ifdef REG_START_MENU
!insertmacro MUI_STARTMENU_GETFOLDER "Application" $SM_Folder
Delete "$SMPROGRAMS\$SM_Folder\${APP_NAME}.lnk"
RmDir "$SMPROGRAMS\$SM_Folder"
!endif

!ifndef REG_START_MENU
Delete "$SMPROGRAMS\${COMP_NAME}\${APP_NAME}.lnk"
RmDir "$SMPROGRAMS\${COMP_NAME}"
!endif

ReadRegStr $0 ${REG_HKLM} "${REG_ENV_PATH}" "Path"
${UnStrRep} $1 $0 ";$INSTDIR\bin;" ""
WriteRegSTR ${REG_HKLM} "${REG_ENV_PATH}" "Path" $1

DeleteRegValue ${REG_HKLM} "${REG_ENV_PATH}" "GRID"
nsExec::Exec 'setx /m GRID_CHECK 1'

DeleteRegKey ${REG_ROOT} "${REG_APP_PATH}"
DeleteRegKey ${REG_ROOT} "${UNINSTALL_PATH}"
DeleteRegValue ${REG_HKLM} "${REG_ENV_PATH}" "GRID_CHECK"
SectionEnd

######################################################################

