Unicode true

; 64-bit plugin
	!include x64.nsh
	!include nsDialogs.nsh

; folders
	!define FOLDERS_DIST "dist"
	!define FOLDERS_SCHEMA "schema"
	!define FOLDERS_SCRIPTS "scripts"
	!define FOLDERS_BIN "bin"
	!define FOLDERS_PLATFORM "platform\windows\installer"
	!define FOLDERS_IMAGES "${FOLDERS_PLATFORM}\images"

	!include "${FOLDERS_PLATFORM}\pages\InstallOptions.nsh"
	!include "${FOLDERS_PLATFORM}\pages\ReadyToInstall.nsh"

	!addplugindir "${FOLDERS_PLATFORM}\plugins"

; files
	!define FILES_MAIN "chalet.exe"
	!define FILES_UNINSTALLER "uninst.exe"
	!define FILES_LICENSE "LICENSE.txt"
	!define FILES_README "README.md"
	!define FILES_CMD "chalet.cmd"
	!define FILES_SCHEMA_BUILD "${FOLDERS_SCHEMA}\chalet.schema.json"
	!define FILES_SCHEMA_SETTINGS "${FOLDERS_SCHEMA}\chalet-settings.schema.json"
	!define FILES_SCRIPTS_BASH_COMPLETION "${FOLDERS_SCRIPTS}\chalet-completion.sh"

; output
	!define OUTPUT_FILE "${FOLDERS_DIST}\chalet-windows-installer.exe"

; helper defines
	!define PRODUCT_NAME "Chalet"
	!define PRODUCT_VERSION "0.3.0"
	!define PRODUCT_PUBLISHER "TBD LLC."
	!define PRODUCT_WEB_SITE "www.github.com/chalet-org"
	!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\${FILES_MAIN}"
	!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
	!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; MUI 2.x compatible ------
	!include "MUI2.nsh"

; MUI Settings
	!define MUI_HEADERIMAGE
	!define MUI_WELCOMEFINISHPAGE_BITMAP "${FOLDERS_IMAGES}\welcome_install.bmp"
	; !define MUI_UNWELCOMEFINISHPAGE_BITMAP "${FOLDERS_IMAGES}\welcome_uninstall.bmp"
	!define MUI_HEADERIMAGE_BITMAP "${FOLDERS_IMAGES}\header_install.bmp"
	!define MUI_HEADERIMAGE_BITMAP_RTL "${FOLDERS_IMAGES}\header_install_right.bmp"
	!define MUI_HEADERIMAGE_UNBITMAP "${FOLDERS_IMAGES}\header_install.bmp"
	!define MUI_HEADERIMAGE_UNBITMAP_RTL "${FOLDERS_IMAGES}\header_install_right.bmp"
	!define MUI_ABORTWARNING
	!define MUI_ICON "${FOLDERS_IMAGES}\msi.ico"
	!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\win-uninstall.ico"
	!define MUI_FINISHPAGE_NOAUTOCLOSE
	!define MUI_UNFINISHPAGE_NOAUTOCLOSE


; Language Selection Dialog Settings
	!define MUI_LANGDLL_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
	!define MUI_LANGDLL_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
	!define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"

; Welcome page
	!insertmacro MUI_PAGE_WELCOME
; License page
	!insertmacro MUI_PAGE_LICENSE "${FILES_LICENSE}"
; Directory page
	!insertmacro MUI_PAGE_DIRECTORY
; Install Options page (custom)
	!insertmacro CHALET_PAGE_INSTALLOPTIONS
; Ready to Install page (custom
	!insertmacro CHALET_PAGE_READYTOINSTALL
; Instfiles page
	!insertmacro MUI_PAGE_INSTFILES

; Finish page
	; !define MUI_FINISHPAGE_RUN "$INSTDIR\${FILES_CMD}"
	!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\${FILES_README}"
	!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
	!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
	; !insertmacro MUI_UNPAGE_WELCOME
	!insertmacro MUI_UNPAGE_INSTFILES

; Language files
	; !insertmacro MUI_LANGUAGE "Afrikaans"
	; !insertmacro MUI_LANGUAGE "Albanian"
	; !insertmacro MUI_LANGUAGE "Arabic"
	; !insertmacro MUI_LANGUAGE "Armenian"
	; !insertmacro MUI_LANGUAGE "Asturian"
	; !insertmacro MUI_LANGUAGE "Basque"
	; !insertmacro MUI_LANGUAGE "Belarusian"
	; !insertmacro MUI_LANGUAGE "Bosnian"
	; !insertmacro MUI_LANGUAGE "Breton"
	; !insertmacro MUI_LANGUAGE "Bulgarian"
	; !insertmacro MUI_LANGUAGE "Catalan"
	; !insertmacro MUI_LANGUAGE "Corsican"
	; !insertmacro MUI_LANGUAGE "Croatian"
	; !insertmacro MUI_LANGUAGE "Czech"
	; !insertmacro MUI_LANGUAGE "Danish"
	; !insertmacro MUI_LANGUAGE "Dutch"
	!insertmacro MUI_LANGUAGE "English"
	; !insertmacro MUI_LANGUAGE "Esperanto"
	; !insertmacro MUI_LANGUAGE "Estonian"
	; !insertmacro MUI_LANGUAGE "Farsi"
	; !insertmacro MUI_LANGUAGE "Finnish"
	; !insertmacro MUI_LANGUAGE "French"
	; !insertmacro MUI_LANGUAGE "Galician"
	; !insertmacro MUI_LANGUAGE "Georgian"
	; !insertmacro MUI_LANGUAGE "German"
	; !insertmacro MUI_LANGUAGE "Greek"
	; !insertmacro MUI_LANGUAGE "Hebrew"
	; !insertmacro MUI_LANGUAGE "Hindi"
	; !insertmacro MUI_LANGUAGE "Hungarian"
	; !insertmacro MUI_LANGUAGE "Icelandic"
	; !insertmacro MUI_LANGUAGE "Indonesian"
	; !insertmacro MUI_LANGUAGE "Irish"
	; !insertmacro MUI_LANGUAGE "Italian"
	; !insertmacro MUI_LANGUAGE "Japanese"
	; !insertmacro MUI_LANGUAGE "Korean"
	; !insertmacro MUI_LANGUAGE "Kurdish"
	; !insertmacro MUI_LANGUAGE "Latvian"
	; !insertmacro MUI_LANGUAGE "Lithuanian"
	; !insertmacro MUI_LANGUAGE "Luxembourgish"
	; !insertmacro MUI_LANGUAGE "Macedonian"
	; !insertmacro MUI_LANGUAGE "Malay"
	; !insertmacro MUI_LANGUAGE "Mongolian"
	; !insertmacro MUI_LANGUAGE "Norwegian"
	; !insertmacro MUI_LANGUAGE "NorwegianNynorsk"
	; !insertmacro MUI_LANGUAGE "Pashto"
	; !insertmacro MUI_LANGUAGE "Polish"
	; !insertmacro MUI_LANGUAGE "Portuguese"
	; !insertmacro MUI_LANGUAGE "PortugueseBR"
	; !insertmacro MUI_LANGUAGE "Romanian"
	; !insertmacro MUI_LANGUAGE "Russian"
	; !insertmacro MUI_LANGUAGE "ScotsGaelic"
	; !insertmacro MUI_LANGUAGE "Serbian"
	; !insertmacro MUI_LANGUAGE "SerbianLatin"
	; !insertmacro MUI_LANGUAGE "SimpChinese"
	; !insertmacro MUI_LANGUAGE "Slovak"
	; !insertmacro MUI_LANGUAGE "Slovenian"
	; !insertmacro MUI_LANGUAGE "Spanish"
	; !insertmacro MUI_LANGUAGE "SpanishInternational"
	; !insertmacro MUI_LANGUAGE "Swedish"
	; !insertmacro MUI_LANGUAGE "Tatar"
	; !insertmacro MUI_LANGUAGE "Thai"
	; !insertmacro MUI_LANGUAGE "TradChinese"
	; !insertmacro MUI_LANGUAGE "Turkish"
	; !insertmacro MUI_LANGUAGE "Ukrainian"
	; !insertmacro MUI_LANGUAGE "Uzbek"
	; !insertmacro MUI_LANGUAGE "Vietnamese"
	; !insertmacro MUI_LANGUAGE "Welsh"

	!include "${FOLDERS_PLATFORM}\pages\LanguageStrings.nsh"

; MUI end ------

	Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
	OutFile "${OUTPUT_FILE}"
	InstallDir "$PROGRAMFILES\${PRODUCT_NAME}"
	InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
	ShowInstDetails show
	ShowUnInstDetails show
	RequestExecutionLevel user

; TODO: Compile dist\chalet.exe in 32-bits and support it here

Function .onInit
	!insertmacro MUI_LANGDLL_DISPLAY
${If} ${RunningX64}
	; disable registry redirection (enable access to 64-bit portion of registry)
	SetRegView 64
	StrCpy $INSTDIR "$PROGRAMFILES64\${PRODUCT_NAME}"
${EndIf}
FunctionEnd

; Add Chalet to Path page
LangString PAGE_TITLE ${LANG_ENGLISH} "Title"
LangString PAGE_SUBTITLE ${LANG_ENGLISH} "Subtitle"

!define OUT_BIN "$INSTDIR\${FOLDERS_BIN}"

Section "MainSection" SEC01
	SetOverwrite on
	SetOutPath "$INSTDIR"
	File "${FILES_README}"
	File "${FILES_LICENSE}"

	SetOutPath "${OUT_BIN}"
	File "${FOLDERS_PLATFORM}\${FILES_CMD}"
	File "${FOLDERS_DIST}\${FILES_MAIN}"

	SetOutPath "$INSTDIR\${FOLDERS_SCHEMA}"
	File "${FILES_SCHEMA_BUILD}"
	File "${FILES_SCHEMA_SETTINGS}"
	File "${FILES_SCRIPTS_BASH_COMPLETION}"

; Shortcuts
	; CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
	; CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\${FILES_MAIN}"
	; CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\${FILES_MAIN}"

	${If} $chalet.InstallOptionsPage.RadioState == 2 ; PATH for all users
		; Set to HKLM (set Path for all users)
		EnVar::SetHKLM
		; Check for write access
		EnVar::Check "NULL" "NULL"
		Pop $0
		${If} $0 == 0
			EnVar::AddValue "Path" "${OUT_BIN}"
			DetailPrint "Added '${OUT_BIN}' to 'Path' system environment variable."
		${Else}
			DetailPrint "Error adding '${OUT_BIN}' to 'Path' system environment variable."
		${EndIf}
		EnVar::SetHKCU
	${ElseIf} $chalet.InstallOptionsPage.RadioState == 3 ; PATH for current user
		; Set to HKCU (set Path for current user)
		EnVar::SetHKCU
		; Check for write access
		EnVar::Check "NULL" "NULL"
		Pop $0
		${If} $0 == 0
			EnVar::AddValue "Path" "${OUT_BIN}"
			DetailPrint "Added '${OUT_BIN}' to 'Path' environment variable."
		${Else}
			DetailPrint "Error adding '${OUT_BIN}' to 'Path' environment variable."
		${EndIf}
	${EndIf}

	; MessageBox MB_OK "????"
SectionEnd

Section -AdditionalIcons
	WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\${FILES_UNINSTALLER}"
SectionEnd

Section -Post
	WriteUninstaller "$INSTDIR\${FILES_UNINSTALLER}"
	WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\${FILES_MAIN}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\${FILES_UNINSTALLER}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\${FILES_MAIN}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd


Function un.onUninstSuccess
	HideWindow
	; MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
!insertmacro MUI_UNGETLANGUAGE
	MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
	Abort
FunctionEnd

Section Uninstall
	; Set to HKLM (set Path for all users)
	EnVar::SetHKLM
	; Check for write access
	EnVar::Check "NULL" "NULL"
	Pop $0
	${If} $0 == 0
		EnVar::DeleteValue "Path" "${OUT_BIN}"
		DetailPrint "Removed '${OUT_BIN}' from 'Path' system environment variable."
		EnVar::SetHKCU
		EnVar::DeleteValue "Path" "${OUT_BIN}"
		DetailPrint "Removed '${OUT_BIN}' from 'Path' environment variable."
	${Else}
		DetailPrint "Error removing '${OUT_BIN}' from 'Path' environment variable."
		EnVar::SetHKCU
	${EndIf}

	Delete "$INSTDIR\${PRODUCT_NAME}.url"
	Delete "$INSTDIR\${FILES_UNINSTALLER}"
	Delete "$INSTDIR\${FILES_LICENSE}"
	Delete "$INSTDIR\${FILES_README}"
	Delete "$INSTDIR\${FILES_SCRIPTS_BASH_COMPLETION}"
	Delete "$INSTDIR\${FILES_SCHEMA_SETTINGS}"
	Delete "$INSTDIR\${FILES_SCHEMA_BUILD}"
	Delete "${OUT_BIN}\${FILES_MAIN}"
	Delete "${OUT_BIN}\${FILES_CMD}"

	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Website.lnk"

	; Shortcuts
	; Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
	; Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
	; RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

	RMDir "${OUT_BIN}"
	RMDir "$INSTDIR\${FOLDERS_SCHEMA}"
	RMDir "$INSTDIR"

	DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
	DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
	SetAutoClose false
SectionEnd
