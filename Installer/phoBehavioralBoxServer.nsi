; Turn off old selected section
; 10 09 2019: Pho Hale
; Template to generate an installer
 
; -------------------------------
; Start
  !define PRODUCT_VERSION "1.0.0.6s"
  !define VERSION "1.0.0.6s"

  !define MUI_PRODUCT "phoBehavioralBoxServer"
  !define MUI_FILE "phoBehavioralBoxServer"
  !define MUI_VERSION ""
  !define MUI_BRANDINGTEXT "phoBehavioralBoxServer Ver. ${PRODUCT_VERSION}"

  VIProductVersion "${PRODUCT_VERSION}"
  VIFileVersion "${VERSION}"
  VIAddVersionKey "FileVersion" "${VERSION}"
  VIAddVersionKey "LegalCopyright" "(C) Pho Hale, Brendon Watson Lab, 2020"
  VIAddVersionKey "FileDescription" "Server for loading the historical data from phoBehavioralBoxLabjackController."


  !define PHO_COMMONDIR "C:\Common"
  !define PHO_COMMON_NAME_BIN "bin"
  !define PHO_COMMON_DIR_BIN "C:\Common\bin"
  !define PHO_COMMON_NAME_CONFIG "config"
  !define PHO_COMMON_DIR_CONFIG "C:\Common\config"
  ;!define PHO_BUILD_PATH "../x64/Release"
  !define PHO_BUILD_PATH "C:\Common\repo\phoBehavioralBoxLabjackController\x64\Release"
  CRCCheck On

  ; Installer silent by default:
  SilentInstall silent
  ; Prevent skipping files that are in use. Fails instead.
  AllowSkipFiles off
 
  ; We should test if we must use an absolute path 
  ; !include "${NSISDIR}\Contrib\Modern UI\System.nsh"
 
;---------------------------------
;General
 
  OutFile "Install phoBehavioralBoxServer.exe"
  ShowInstDetails "nevershow"
  ShowUninstDetails "nevershow"
  ;SetCompressor "bzip2"
 
  ;!define MUI_ICON "icon.ico"
  ;!define MUI_UNICON "icon.ico"
  ;!define MUI_SPECIALBITMAP "Bitmap.bmp"
 
 
;--------------------------------
;Folder selection page
 
  InstallDir "${PHO_COMMONDIR}\${PHO_COMMON_NAME_BIN}\${MUI_PRODUCT}"
 
 
;--------------------------------
;Modern UI Configuration
 
;   !define MUI_WELCOMEPAGE  
;   !define MUI_LICENSEPAGE
;   !define MUI_DIRECTORYPAGE
;   !define MUI_ABORTWARNING
;   !define MUI_UNINSTALLER
;   !define MUI_UNCONFIRMPAGE
;   !define MUI_FINISHPAGE  
 
 
;--------------------------------
;Language
 
  ;!insertmacro MUI_LANGUAGE "English"
 
 
;-------------------------------- 
;Modern UI System
 
;   !insertmacro MUI_SYSTEM 
 
 
;--------------------------------
;Data
 
;   LicenseData "Read_me.txt"
 
 
;-------------------------------- 
;Installer Sections     
;Section "install" Installation info
 Section "install"

;Add files
  ; SetOutPath "${PHO_COMMON_DIR_CONFIG}"
  
  ; IfFileExists "${PHO_COMMON_DIR_CONFIG}\phoBehavioralBoxLabjackController-WT_config.xml" CopyWtConfigScript skip
  ; CopyWtConfigScript:
  ; File "${PHO_COMMON_DIR_CONFIG}\phoBehavioralBoxLabjackController-WT_config.xml"
  ; Skip:

  ; IfFileExists "${PHO_COMMON_DIR_CONFIG}\phoBehavioralBoxLabjackController-Config.ini" CopyMainConfigScript SkipMainConfigCopy
  ; CopyMainConfigScript:
  ; File "${PHO_COMMON_DIR_CONFIG}\phoBehavioralBoxLabjackController-Config.ini"
  ; SkipMainConfigCopy:

  SetOutPath "$INSTDIR"
 
  File "${PHO_BUILD_PATH}\${MUI_FILE}.exe"
  File "${PHO_BUILD_PATH}\libcrypto-1_1.dll"
  File "${PHO_BUILD_PATH}\libhpdf.dll"
  File "${PHO_BUILD_PATH}\libssl-1_1.dll"
  File "${PHO_BUILD_PATH}\wt.dll"
  File "${PHO_BUILD_PATH}\wthttp.dll"
  File "${PHO_BUILD_PATH}\wttest.dll"
;  File "${PHO_BUILD_PATH}\phoBehavioralBoxLabjackController.res"

  ;SetOutPath "$INSTDIR\resources"
  file /r ..\resources
  ;file /r mpg
  ;file /r mpg
  ;file /r mpg
  ;File "*"

  ;File "Read_me.txt"
  ;SetOutPath "$INSTDIR\bin"
  ;file "playlists\${MUI_FILE}.epp"
  ;SetOutPath "$INSTDIR\data"
  ;file "data\*.cst"
  ;file "data\errorlog.txt"
  ; Here follow the files that will be in the playlist
  ;SetOutPath "$INSTDIR"  
  ;file /r mpg
  ;SetOutPath "$INSTDIR"  
  ;file /r xtras  
 
;create desktop shortcut
  CreateShortCut "$DESKTOP\${MUI_FILE}.lnk" "$INSTDIR\${MUI_FILE}.exe" ""
 
;create start-menu items
  CreateDirectory "$SMPROGRAMS\${MUI_PRODUCT}"
  CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\Uninstall.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\${MUI_PRODUCT}.lnk" "$INSTDIR\${MUI_FILE}.exe" "" "$INSTDIR\${MUI_FILE}.exe" 0
 
;write uninstall information to the registry
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "DisplayName" "${MUI_PRODUCT} (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "UninstallString" "$INSTDIR\Uninstall.exe"
 
  WriteUninstaller "$INSTDIR\Uninstall.exe"
 
SectionEnd
 
 
;--------------------------------    
;Uninstaller Section  
Section "Uninstall"
 
;Delete Files 
  RMDir /r "$INSTDIR\*.*"    
 
;Remove the installation directory
  RMDir "$INSTDIR"
 
;Delete Start Menu Shortcuts
  Delete "$DESKTOP\${MUI_PRODUCT}.lnk"
  Delete "$SMPROGRAMS\${MUI_PRODUCT}\*.*"
  RmDir  "$SMPROGRAMS\${MUI_PRODUCT}"
 
;Delete Uninstaller And Unistall Registry Entries
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${MUI_PRODUCT}"
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}"  
 
SectionEnd
 
 
;--------------------------------    
;MessageBox Section
 
 
;Function that calls a messagebox when installation finished correctly
Function .onInstSuccess
  MessageBox MB_OK "You have successfully installed ${MUI_PRODUCT}. Use the desktop icon to start the program." /SD IDOK
FunctionEnd
 
Function un.onUninstSuccess
  MessageBox MB_OK "You have successfully uninstalled ${MUI_PRODUCT}." /SD IDOK
FunctionEnd
 
 
;eof