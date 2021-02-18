set appName to system attribute "CHALET_MACOS_BUNDLE_NAME"
set appNameExt to appName & ".app"

tell application "Finder"
	tell disk appName
		open
		set current view of container window to icon view
		set toolbar visible of container window to false
		set statusbar visible of container window to false
		set the bounds of container window to {0, 0, 512, 342}
		set viewOptions to the icon view options of container window
		set arrangement of viewOptions to not arranged
		set icon size of viewOptions to 80
		set background picture of viewOptions to file ".background:background.tiff"
		set position of item appNameExt of container window to {120, 188}
		set position of item "Applications" of container window to {392, 188}
		set position of item ".background" of container window to {120, 388}
		-- set name of item "Applications" to " "
		close
		update without registering applications
		delay 2
	end tell
end tell
