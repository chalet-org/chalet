/*

NSIS Modern User Interface
InstallOptions page

*/

;--------------------------------
;Page interface settings and variables

!macro CHALET_INSTALLOPTIONSPAGE_INTERFACE
	!ifndef CHALET_INSTALLOPTIONSPAGE_INTERFACE
		!define CHALET_INSTALLOPTIONSPAGE_INTERFACE
		Var chalet.InstallOptionsPage

		Var chalet.InstallOptionsPage.Label
		Var chalet.InstallOptionsPage.PathRadioA
		Var chalet.InstallOptionsPage.PathRadioB
		Var chalet.InstallOptionsPage.PathRadioC
		Var chalet.InstallOptionsPage.RadioState
		; Var chalet.InstallOptionsPage.DesktopIcon
	!endif
!macroend


;--------------------------------
;Page declaration

!macro CHALET_PAGEDECLARATION_INSTALLOPTIONS
	!insertmacro MUI_SET CHALET_INSTALLOPTIONSPAGE ""
	!insertmacro CHALET_INSTALLOPTIONSPAGE_INTERFACE

	PageEx custom
		PageCallbacks chalet.InstallOptionsCreate_${MUI_UNIQUEID} chalet.InstallOptionsLeave_${MUI_UNIQUEID}

		Caption " " ; (Title bar caption)
	PageExEnd

	!insertmacro CHALET_FUNCTION_INSTALLOPTIONSPAGE chalet.InstallOptionsCreate_${MUI_UNIQUEID} chalet.InstallOptionsLeave_${MUI_UNIQUEID}
!macroend

!macro CHALET_PAGE_INSTALLOPTIONS
	!verbose push
	!verbose ${MUI_VERBOSE}

	!insertmacro MUI_PAGE_INIT
	!insertmacro CHALET_PAGEDECLARATION_INSTALLOPTIONS

	!verbose pop
!macroend

!macro CHALET_UNPAGE_INSTALLOPTIONS
	!verbose push
	!verbose ${MUI_VERBOSE}

	!insertmacro MUI_UNPAGE_INIT
	!insertmacro CHALET_PAGEDECLARATION_INSTALLOPTIONS

	!verbose pop
!macroend


;--------------------------------
;Page functions

!macro CHALET_FUNCTION_INSTALLOPTIONSPAGE CREATE LEAVE
	Function "${CREATE}"
		!insertmacro MUI_HEADER_TEXT_PAGE $(CHALET_TEXT_INSTALLOPTIONS_TITLE) $(CHALET_TEXT_INSTALLOPTIONS_SUBTITLE)

		nsDialogs::Create 1018
		Pop $chalet.InstallOptionsPage

		${If} $chalet.InstallOptionsPage == error
			Abort
		${EndIf}

		${NSD_CreateLabel} 0 0 100% 18u $(CHALET_TEXT_INSTALLOPTIONS_LABEL)
		Pop $chalet.InstallOptionsPage.Label

		${NSD_CreateRadioButton} 0 32u 100% 12u $(CHALET_TEXT_INSTALLOPTIONS_PATH_RADIOA)
		Pop $chalet.InstallOptionsPage.PathRadioA

		${NSD_CreateRadioButton} 0 44u 100% 12u $(CHALET_TEXT_INSTALLOPTIONS_PATH_RADIOB)
		Pop $chalet.InstallOptionsPage.PathRadioB
		${NSD_Check} $chalet.InstallOptionsPage.PathRadioB

		${NSD_CreateRadioButton} 0 56u 100% 12u $(CHALET_TEXT_INSTALLOPTIONS_PATH_RADIOC)
		Pop $chalet.InstallOptionsPage.PathRadioC

		; ${NSD_CreateCheckBox} 0 88u 100% 12u $(CHALET_TEXT_INSTALLOPTIONS_PATH_DESKTOP_ICON)
		; Pop $chalet.InstallOptionsPage.DesktopIcon
		; ${NSD_Check} $chalet.InstallOptionsPage.DesktopIcon

		nsDialogs::Show

		!insertmacro MUI_PAGE_FUNCTION_CUSTOM CREATE
	FunctionEnd

	Function "${LEAVE}"
		var /GLOBAL chalet.InstallOptionsPage._RadioStateA
		var /GLOBAL chalet.InstallOptionsPage._RadioStateB
		var /GLOBAL chalet.InstallOptionsPage._RadioStateC

		${NSD_GetState} $chalet.InstallOptionsPage.PathRadioA $chalet.InstallOptionsPage._RadioStateA
		${NSD_GetState} $chalet.InstallOptionsPage.PathRadioB $chalet.InstallOptionsPage._RadioStateB
		${NSD_GetState} $chalet.InstallOptionsPage.PathRadioC $chalet.InstallOptionsPage._RadioStateC

		${If} $chalet.InstallOptionsPage._RadioStateA == ${BST_CHECKED}
			StrCpy $chalet.InstallOptionsPage.RadioState 1
		${ElseIf} $chalet.InstallOptionsPage._RadioStateB == ${BST_CHECKED}
			StrCpy $chalet.InstallOptionsPage.RadioState 2
		${ElseIf} $chalet.InstallOptionsPage._RadioStateC == ${BST_CHECKED}
			StrCpy $chalet.InstallOptionsPage.RadioState 3
		${Else}
			StrCpy $chalet.InstallOptionsPage.RadioState 0
		${Endif}

		!insertmacro MUI_PAGE_FUNCTION_CUSTOM LEAVE
	FunctionEnd
!macroend
