############################################################################################
#      NSIS Installation Script created by NSIS Quick Setup Script Generator v1.09.18
#               Entirely Edited with NullSoft Scriptable Installation System                
#              by Vlasis K. Barkas aka Red Wine red_wine@freemail.gr Sep 2006               
############################################################################################
!define APP_NAME "Summit-Grid"
!define COMP_NAME "Centrilliontech"
!define WEB_SITE "www.centrilliontech.com.tw"
!define VERSION "<%version%>"
!define COPYRIGHT "Centrillion"
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
!define ENV_PATH_BIN "%ProgramFiles%\${COMP_NAME}\${APP_NAME}\bin;"

######################################################################

VIProductVersion  "${VERSION}"
VIAddVersionKey "ProductName"  "${APP_NAME}"
VIAddVersionKey "CompanyName"  "${COMP_NAME}"
VIAddVersionKey "LegalCopyright"  "${COPYRIGHT}"
VIAddVersionKey "FileDescription"  "${DESCRIPTION}"
VIAddVersionKey "FileVersion"  "${VERSION}"

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

Section -SetPATH
ReadRegStr $0 "${REG_HKLM}" "${REG_ENV_PATH}" "Path"

MessageBox MB_OK $INSTDIR
MessageBox MB_OK "${INSTALL_DIR}"

${If} $INSTDIR == "${INSTALL_DIR}"
    MessageBox MB_OK "AA"
    MessageBox MB_OK "$0${ENV_PATH_BIN}"
${Else}
    MessageBox MB_OK "BB"
    MessageBox MB_OK "$0$INSTDIR\bin;"
${EndIf}

SectionEnd

######################################################################

Var SERVER_ICON_PATH 
Section -Icons_Reg
StrCpy $SERVER_ICON_PATH "$INSTDIR\icon.ico"
SetOutPath "$INSTDIR"
WriteUninstaller "$INSTDIR\uninstall.exe"

WriteRegStr ${REG_ROOT} "${REG_APP_PATH}" "" "$INSTDIR\${MAIN_APP_EXE}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}"  "DisplayName" "${APP_NAME}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}"  "UninstallString" "$INSTDIR\uninstall.exe"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}"  "DisplayIcon" "$INSTDIR\icon.ico"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}"  "DisplayVersion" "${VERSION}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}"  "Publisher" "${COMP_NAME}"

!ifdef WEB_SITE
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}"  "URLInfoAbout" "${WEB_SITE}"
!endif
SectionEnd

######################################################################

Section Uninstall
${INSTALL_TYPE}
RMDir /r /REBOOTOK "$INSTDIR"
 
Delete "$INSTDIR\uninstall.exe"
!ifdef WEB_SITE
Delete "$INSTDIR\${APP_NAME} website.url"
!endif

RmDir "$INSTDIR"

ReadRegStr $0 "${REG_HKLM}" "${REG_ENV_PATH}" "Path"

${If} $INSTDIR == "${INSTALL_DIR}"
    MessageBox MB_OK "AA"
    StrCpy $0 "$0${ENV_PATH_BIN}"
    MessageBox MB_OK $0
    ${UnStrRep} $1 $0 ${ENV_PATH_BIN} ""
    MessageBox MB_OK $1
${Else}
    MessageBox MB_OK "BB"
    StrCpy $0 "$0$INSTDIR\bin;"
    MessageBox MB_OK $0
    ${UnStrRep} $1 $0 "$INSTDIR\bin;" ""
    MessageBox MB_OK $1
${EndIf}

DeleteRegKey ${REG_ROOT} "${REG_APP_PATH}"
DeleteRegKey ${REG_ROOT} "${UNINSTALL_PATH}"
SectionEnd

######################################################################

