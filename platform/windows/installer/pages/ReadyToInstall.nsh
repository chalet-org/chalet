/*

NSIS Modern User Interface
ReadyToInstall page

*/

;--------------------------------
;Page interface settings and variables

!macro CHALET_READYTOINSTALLPAGE_INTERFACE
	!ifndef CHALET_READYTOINSTALLPAGE_INTERFACE
		!define CHALET_READYTOINSTALLPAGE_INTERFACE
		Var chalet.ReadyToInstallPage

		Var chalet.ReadyToInstallPage.Label
	!endif
!macroend


;--------------------------------
;Page declaration

!macro CHALET_PAGEDECLARATION_READYTOINSTALL
	!insertmacro MUI_SET CHALET_READYTOINSTALLPAGE ""
	!insertmacro CHALET_READYTOINSTALLPAGE_INTERFACE

	PageEx custom
		PageCallbacks chalet.ReadyToInstallCreate_${MUI_UNIQUEID} chalet.ReadyToInstallLeave_${MUI_UNIQUEID}

		Caption " " ; (Title bar caption)
	PageExEnd

	!insertmacro CHALET_FUNCTION_READYTOINSTALLPAGE chalet.ReadyToInstallCreate_${MUI_UNIQUEID} chalet.ReadyToInstallLeave_${MUI_UNIQUEID}
!macroend

!macro CHALET_PAGE_READYTOINSTALL
	!verbose push
	!verbose ${MUI_VERBOSE}

	!insertmacro MUI_PAGE_INIT
	!insertmacro CHALET_PAGEDECLARATION_READYTOINSTALL

	!verbose pop
!macroend

!macro CHALET_UNPAGE_READYTOINSTALL
	!verbose push
	!verbose ${MUI_VERBOSE}

	!insertmacro MUI_UNPAGE_INIT
	!insertmacro CHALET_PAGEDECLARATION_READYTOINSTALL

	!verbose pop
!macroend


;--------------------------------
;Page functions

!macro CHALET_FUNCTION_READYTOINSTALLPAGE CREATE LEAVE
	Function "${CREATE}"
		!insertmacro MUI_HEADER_TEXT_PAGE $(CHALET_TEXT_READYTOINSTALL_TITLE) " "

		nsDialogs::Create 1018
		Pop $chalet.ReadyToInstallPage

		${If} $chalet.ReadyToInstallPage == error
			Abort
		${EndIf}

		${NSD_CreateLabel} 0 0 100% 18u $(CHALET_TEXT_READYTOINSTALL_LABEL)
		Pop $chalet.ReadyToInstallPage.Label

		nsDialogs::Show

		!insertmacro MUI_PAGE_FUNCTION_CUSTOM CREATE
	FunctionEnd

	Function "${LEAVE}"
		!insertmacro MUI_PAGE_FUNCTION_CUSTOM LEAVE
	FunctionEnd
!macroend
