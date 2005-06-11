; !packhdr "x264.dat" "upx.exe --best x264.dat"
; ---------------------------------------------------------------------------
; ---------------------------------------------------------------------------

!define NAME "x264 H.264/AVC Video Codec"
!define OUTFILE "x264vfw.exe"
!define INPUT_PATH ""
!define VFW_FILE1 "x264vfw.dll"
!define VFW_FILE2 "x264vfw.ico"
!define UNINST_NAME "x264-uninstall.exe"
!define MUI_WELCOMEFINISHPAGE_BITMAP win.bmp
!define MUI_UNWELCOMEFINISHPAGE_BITMAP win.bmp
!define MUI_COMPONENTSPAGE_SMALLDESC
!include "MUI.nsh"
!include "Sections.nsh"
!include "LogicLib.nsh"

; ---------------------------------------------------------------------------
; NOTE: this .NSI script is designed for NSIS v1.8+
; ---------------------------------------------------------------------------

Name "${NAME}"
OutFile "${OUTFILE}"
SetCompressor lzma

SetOverwrite ifnewer
SetDatablockOptimize on ; (can be off)
CRCCheck on ; (can be off)
AutoCloseWindow false ; (can be true for the window go away automatically at end)
ShowInstDetails hide ; (can be show to have them shown, or nevershow to disable)
SetDateSave off ; (can be on to have files restored to their orginal date)

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "English"

; ---------------------------------------------------------------------------

;Section ""
;  MessageBox MB_YESNO "This will install ${NAME}. Do you wish to continue?" IDYES gogogo
;    Abort
;  gogogo:
;SectionEnd

Section "-hidden section"
;Section "" ; (default section)
	SetOutPath "$SYSDIR"
	; add files / whatever that need to be installed here.
	File "${INPUT_PATH}${VFW_FILE1}"
	File "${INPUT_PATH}${VFW_FILE2}"

	CreateDirectory $SMPROGRAMS\x264
  	CreateShortcut "$SMPROGRAMS\x264\x264 configuration.lnk" $SYSDIR\rundll32.exe x264vfw.dll,Configure x264vfw.ico
	CreateShortcut "$SMPROGRAMS\x264\x264 Uninstall.lnk" $SYSDIR\x264-uninstall.exe

	; write out uninstaller
	WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "DisplayName" "${NAME} (remove only)"
	WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "UninstallString" '"$SYSDIR\${UNINST_NAME}"'
	WriteUninstaller "$SYSDIR\${UNINST_NAME}"

	DeleteRegKey HKCU "Software\GNU\x264"

	WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows NT\CurrentVersion\Drivers32" "vidc.X264" "x264vfw.dll"
	WriteINIStr system.ini drivers32 "vidc.X264" "x264vfw.dll"
	WriteRegStr HKEY_LOCAL_MACHINE "SYSTEM\CurrentControlSet\Control\MediaResources\icm\vidc.X264" "Description" "x264 H.264 Video Codec"
	WriteRegStr HKEY_LOCAL_MACHINE "SYSTEM\CurrentControlSet\Control\MediaResources\icm\vidc.X264" "Driver" "x264vfw.dll"
	WriteRegStr HKEY_LOCAL_MACHINE "SYSTEM\CurrentControlSet\Control\MediaResources\icm\vidc.X264" "FriendlyName" "x264"



SectionEnd ; end of default section

  

; ---------------------------------------------------------------------------

; begin uninstall settings/section
UninstallText "This will uninstall ${NAME} from your system"

Section Uninstall
	
	; add delete commands to delete whatever files/registry keys/etc you installed here.
	Delete /REBOOTOK "$SYSDIR\${VFW_FILE1}"
	Delete /REBOOTOK "$SYSDIR\${VFW_FILE2}"
	Delete "$SYSDIR\${UNINST_NAME}"
   
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}"
	DeleteRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows NT\CurrentVersion\Drivers32\vidc.X264"
	DeleteRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Drivers32\vidc.X264"
	DeleteRegKey HKEY_LOCAL_MACHINE "SYSTEM\CurrentControlSet\Control\MediaResources\icm\vidc.X264"
	DeleteINIStr system.ini drivers32 "vidc.X264"
 	Delete "$SMPROGRAMS\x264\x264 configuration.lnk"
  	Delete "$SMPROGRAMS\x264\x264 Uninstall.lnk"
	RMDir  "$SMPROGRAMS\x264"


SectionEnd ; end of uninstall section

; ---------------------------------------------------------------------------

Function un.onUninstSuccess
	IfRebootFlag 0 NoReboot
		MessageBox MB_OK \ 
			"A file couldn't be deleted. It will be deleted at next reboot."
	NoReboot:
FunctionEnd

; ---------------------------------------------------------------------------
; eof
; ---------------------------------------------------------------------------
