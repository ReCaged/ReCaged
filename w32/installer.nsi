; ReCaged - a Free Software, Futuristic, Racing Simulator
;
; Copyright (C) 2012, 2013 Mats Wahlberg
;
; This file is part of ReCaged.
;
; ReCaged is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; ReCaged is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with ReCaged.  If not, see <http://www.gnu.org/licenses/>.

;
;includes
;
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "nsDialogs.nsh"
!include "version.nsh"


;
;general info
;

Name "ReCaged ${VERSION}"
OutFile "ReCaged-${VERSION}-Setup.exe"

;default installation directory
InstallDir "$PROGRAMFILES\ReCaged"

;but if already installed (reg string), use this instead
InstallDirRegKey HKLM "Software\ReCaged" "Installed"

;need higher privileges
RequestExecutionLevel admin


;
;interface tweaks
;

;TODO:
;!define MUI_ICON "icon.ico"
;!define MUI_UNICON "icon.ico"

!define MUI_WELCOMEFINISHPAGE_BITMAP "side.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "side.bmp"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "header.bmp"

!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

BrandingText " "
XPStyle on


;
;installation pages
;

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "Copying.txt"
page Custom SelectionPage SelectionLeave

;run only if installing
!define MUI_PAGE_CUSTOMFUNCTION_PRE COMPONENTS_PRE
!insertmacro MUI_PAGE_COMPONENTS

;run always
Var DIR_BUTTON_TEXT
Var DIR_TOP_TEXT
InstallButtonText $DIR_BUTTON_TEXT
!define MUI_DIRECTORYPAGE_TEXT_TOP $DIR_TOP_TEXT
!define MUI_DIRECTORYPAGE_TEXT_DESTINATION "Destination Directory"
!define MUI_PAGE_HEADER_TEXT "Choose Location"
!define MUI_PAGE_HEADER_SUBTEXT "Choose the directory for ReCaged ${VERSION}"
!insertmacro MUI_PAGE_DIRECTORY

!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH


;
;set language and meta info
;
!insertmacro MUI_LANGUAGE "English"

VIProductVersion "${W32VERSION}"
VIAddVersionKey "ProductName" "ReCaged"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "Comments" "A Free Software, Futuristic, Racing Simulator"
VIAddVersionKey "FileDescription" "A Free Software, Futuristic, Racing Simulator"
VIAddVersionKey "LegalCopyright" "Copyright (C) Mats Wahlberg"


;
;customized skipping of components pages by user choice (install or extract)
;
Var Install
Function COMPONENTS_PRE
	${if} $Install == "0"
		abort
	${endif}
Functionend


;
;custom page for selecting extraction or installation
;
Var Dialog
Var Box
Var Label
Var B1
Var B2
Function SelectionPage
	!insertmacro MUI_HEADER_TEXT "Choose Operation" "Choose what you want to do with ReCaged ${VERSION}"
	nsDialogs::Create 1018
	Pop $Dialog

	${If} $Dialog == error
		Abort
	${EndIf}

	${NSD_CreateLabel} 0 0 100% 12u "Please select what you want to do:"
	Pop $Label

	${NSD_CreateGroupBox} 0 12u 100% 78u "Options"
	Pop $Box

	${NSD_CreateRadioButton} 12u 24u 50% 15u "Install"
	Pop $B1

	${NSD_CreateLabel} 24u 39u 50% 12u "Install on this system"
	Pop $Label

	${NSD_CreateRadioButton} 12u 51u 50% 15u "Extract"
	Pop $B2

	${NSD_CreateLabel} 24u 66u 50% 12u "Extract to a directory for portable usage"
	Pop $Label

	${NSD_SetState} $B1 "1"

	nsDialogs::Show
FunctionEnd

Var UNINST
Var INSTBACKUP
Function SelectionLeave
	${NSD_GetState} $B1 $0
	${if} $0 == "1"
		ReadRegStr $UNINST HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ReCaged" "UninstallString"
		${ifnot} $UNINST == ""
			MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION "ReCaged is already installed and needs to be uninstalled before a new installation. Press Ok to uninstall ReCaged." IDOK remove
			abort

			remove:
			ClearErrors
			ExecWait $UNINST

			abort
		${endif}

		strcpy $DIR_BUTTON_TEXT "Install"
		strcpy $DIR_TOP_TEXT "ReCaged will be installed to the following directory."
		${if} $Install == "0"
			strcpy $INSTDIR $INSTBACKUP
		${endif}
		StrCpy $Install "1"
	${else} ;assume B2 == 1
		strcpy $DIR_BUTTON_TEXT "Extract"
		strcpy $DIR_TOP_TEXT "ReCaged will be extracted to the following directory. Consider choosing one with user write access, like your desktop or a removable drive."
		${ifnot} $Install == "0"
			strcpy $INSTBACKUP $INSTDIR
			strcpy $INSTDIR $DESKTOP\ReCaged
		${endif}
		StrCpy $Install "0"
	${endif}
FunctionEnd


;
;installation sections (if not extracting)
;
;required
Section "ReCaged (required)" SecCore
SectionIn RO

	SetOutPath "$INSTDIR"

	;should cover all
	File /r "data"
	File /r "config"
	File "README.txt"
	File "COPYING.txt"
	File "ReCaged.exe"

	;only if installing
	${if} $Install == "1"
		;write path to install dir to registry string
		WriteRegStr HKLM "Software\ReCaged" "Installed" "$INSTDIR"
  
		;uninstall info (TODO: could add much more here, like version and size)
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ReCaged" "DisplayName" "ReCaged"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ReCaged" "UninstallString" '"$INSTDIR\Uninstall.exe"'
		WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ReCaged" "NoModify" 1
		WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ReCaged" "NoRepair" 1
		WriteUninstaller "Uninstall.exe"

		;uninstaller
		WriteUninstaller "$INSTDIR\Uninstall.exe"
	${endif}

SectionEnd

;optional
Section "Start Menu Shortcuts" SecStart

	;make sure only when installing
	${if} $Install == "1"
		CreateDirectory "$SMPROGRAMS\ReCaged"
		CreateShortCut "$SMPROGRAMS\ReCaged\ReCaged.lnk" "$INSTDIR\ReCaged.exe" "" "$INSTDIR\ReCaged.exe"
		CreateShortCut "$SMPROGRAMS\ReCaged\Readme.lnk" "$INSTDIR\README.txt" "" "$INSTDIR\README.txt"
		CreateShortCut "$SMPROGRAMS\ReCaged\Copying.lnk" "$INSTDIR\COPYING.txt" "" "$INSTDIR\COPYING.txt"
		CreateShortCut "$SMPROGRAMS\ReCaged\Uninstall.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe"
	${endif}

SectionEnd

;optional
Section "Desktop Shortcut" SecDesktop

	;safetycheck again
	${if} $Install == "1"
		CreateShortCut "$DESKTOP\ReCaged.lnk" "$INSTDIR\ReCaged.exe" "" "$INSTDIR\ReCaged.exe" 0
	${endif}

SectionEnd

;descriptions for above sections
LangString DESC_SecCore ${LANG_ENGLISH} "ReCaged itself"
LangString DESC_SecStart ${LANG_ENGLISH} "Adds icons to the start menu"
LangString DESC_SecDesktop ${LANG_ENGLISH} "Adds an icon to your desktop"

;assign to sections
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SecCore} $(DESC_SecCore)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecStart} $(DESC_SecStart)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} $(DESC_SecDesktop)
!insertmacro MUI_FUNCTION_DESCRIPTION_END


;
;uninstaller
;
Section "Uninstall"

	;remove from registry
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ReCaged"
	DeleteRegKey HKLM "Software\ReCaged"

	;any shortcuts
	Delete "$SMPROGRAMS\ReCaged\*.*"
	RMDir "$SMPROGRAMS\ReCaged"
	Delete "$DESKTOP\ReCaged.lnk"

	;files (plus uninstaller)
	RMDir /r "$INSTDIR\data"
	RMDir /r "$INSTDIR\config"
	Delete "$INSTDIR\ReadMe.txt"
	Delete "$INSTDIR\Copying.txt"
	Delete "$INSTDIR\ReCaged.exe"
	Delete "$INSTDIR\Uninstall.exe"

	;and dir itself if possible
	RMDir "$INSTDIR"

SectionEnd
