; Script generated by the HM NIS Edit Script Wizard.

!include LogicLib.nsh

RequestExecutionLevel admin

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "Samoyed IDE"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "Samoyed"
!define PRODUCT_WEB_SITE "http://samoyed.sourceforge.net"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\samoyed.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

!define PREINSTDIR "D:\gtk"

!define MIME_REG_KEY "MIME\Database\Content Type"

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "..\..\COPYING"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\samoyed.exe"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "Setup.exe"
InstallDir "$PROGRAMFILES\Samoyed IDE"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  File /r "${PREINSTDIR}\"
  CreateDirectory "$SMPROGRAMS\Samoyed IDE"
  CreateShortCut "$SMPROGRAMS\Samoyed IDE\Samoyed IDE.lnk" "$INSTDIR\bin\samoyed.exe"
  CreateShortCut "$DESKTOP\Samoyed IDE.lnk" "$INSTDIR\bin\samoyed.exe"
SectionEnd

Section -Post
  ; Register file types.
  WriteRegStr HKCR ".c" "" "cfile"
  WriteRegStr HKCR "cfile" "" "C Source File"
  WriteRegStr HKCR ".c" "Content Type" "text/x-csrc"
  WriteRegStr HKCR ".c" "PerceivedType" ".txt"
  WriteRegStr HKCR ".h" "" "hfile"
  WriteRegStr HKCR "hfile" "" "C Header File"
  WriteRegStr HKCR ".h" "Content Type" "text/x-chdr"
  WriteRegStr HKCR ".h" "PerceivedType" ".txt"
  WriteRegStr HKCR ".cpp" "" "cppfile"
  WriteRegStr HKCR "cppfile" "" "C++ Source File"
  WriteRegStr HKCR ".cpp" "Content Type" "text/x-c++src"
  WriteRegStr HKCR ".cpp" "PerceivedType" ".txt"
  WriteRegStr HKCR ".hpp" "" "hppfile"
  WriteRegStr HKCR "hppfile" "" "C++ Header File"
  WriteRegStr HKCR ".hpp" "Content Type" "text/x-c++hdr"
  WriteRegStr HKCR ".hpp" "PerceivedType" ".txt"
  WriteRegStr HKCR "{MIME_REG_KEY}\text/x-csrc" "Extension" ".c"
  WriteRegStr HKCR "{MIME_REG_KEY}\text/x-chdr" "Extension" ".h"
  WriteRegStr HKCR "{MIME_REG_KEY}\text/x-c++src" "Extension" ".cpp"
  WriteRegStr HKCR "{MIME_REG_KEY}\text/x-c++hdr" "Extension" ".hpp"

  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateShortCut "$SMPROGRAMS\Samoyed IDE\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\bin\samoyed.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\bin\samoyed.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

Function .onInit
  UserInfo::GetAccountType
  pop $0
  ${If} $0 != "admin"
    MessageBox MB_ICONSTOP "Administrator rights required!"
    SetErrorLevel 740 ; ERROR_ELEVATION_REQUIRED
    Quit
  ${EndIf}
FunctionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "Samoyed was successfully removed from your computer."
FunctionEnd

Function un.onInit
  UserInfo::GetAccountType
  pop $0
  ${If} $0 != "admin"
    MessageBox MB_ICONSTOP "Administrator rights required!"
    SetErrorLevel 740 ; ERROR_ELEVATION_REQUIRED
    Quit
  ${EndIf}
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove Samoyed and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  Delete "$SMPROGRAMS\Samoyed IDE\Uninstall.lnk"
  Delete "$DESKTOP\Samoyed IDE.lnk"
  Delete "$SMPROGRAMS\Samoyed IDE\Samoyed IDE.lnk"

  RMDir "$SMPROGRAMS\Samoyed IDE"
  RMDir /r "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
