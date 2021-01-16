/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/PlatformFile.hpp"

namespace chalet
{
/*****************************************************************************/
constexpr Constant PlatformFile::linuxDesktopEntry()
{
	return R"([Desktop Entry]
Version=1.0
Type=Application
Categories=Application;
Terminal=false
Exec=${mainProject}
Path=${path}
Name=${appName}
Comment=${shortDescription}
Icon=${icon}
)";
}

/*****************************************************************************/
constexpr Constant PlatformFile::macosInfoPlist()
{
	return R"({
	"CFBundleName": "${bundleName}",
	"CFBundleDisplayName": "${appName}",
	"CFBundleIdentifier": "${bundleIdentifier}",
	"CFBundleVersion": "${version}",
	"CFBundleDevelopmentRegion": "en",
	"CFBundleInfoDictionaryVersion": "6.0",
	"CFBundlePackageType": "APPL",
	"CFBundleSignature": "????",
	"CFBundleExecutable": "${mainProject}",
	"CFBundleIconFile": "${icon}",
	"NSHighResolutionCapable": true
}
)";
}

/*****************************************************************************/
constexpr Constant PlatformFile::macosDmgApplescript()
{
	return R"(set appName to system attribute "appName"
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
)";
}

/*****************************************************************************/
constexpr Constant PlatformFile::windowsAppManifest()
{
	return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly manifestVersion="1.0" xmlns="urn:schemas-microsoft-com:asm.v1">
	<assemblyIdentity
		name="${appName}"
		processorArchitecture="ia64"
		version="1.0.0.0"
		type="win32" />
	<description>${longDescription}</description>
	<trustInfo xmlns="urn:schemas-microsoft-com:asm.v2">
		<security>
			<requestedPrivileges>
				<requestedExecutionLevel level="asInvoker" uiAccess="false" />
			</requestedPrivileges>
		</security>
	</trustInfo>
	<application xmlns="urn:schemas-microsoft-com:asm.v3">
		<windowsSettings xmlns="http://schemas.microsoft.com/SMI/2011/WindowsSettings">
			<disableWindowFiltering>true</disableWindowFiltering>
		</windowsSettings>
		<windowsSettings xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">
			<dpiAwareness>permonitorv2, permonitor, unaware</dpiAwareness>
      		<longPathAware>true</longPathAware>
		</windowsSettings>
		<windowsSettings xmlns="http://schemas.microsoft.com/SMI/2017/WindowsSettings">
			<gdiScaling>false</gdiScaling>
		</windowsSettings>
		<windowsSettings xmlns="http://schemas.microsoft.com/SMI/2020/WindowsSettings">
			<heapType>SegmentHeap</heapType>
		</windowsSettings>
	</application>
	<compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
		<application>
			<supportedOS Id="{e2011457-1546-43c5-a5fe-008deee3d3f0}" /> <!-- Windows Vista/Server 2008 -->
			<supportedOS Id="{35138b9a-5d96-4fbd-8e2d-a2440225f93a}" /> <!-- Windows 7/Server 2008 R2 -->
			<supportedOS Id="{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}" /> <!-- Windows 8/Server 2012 -->
			<supportedOS Id="{1f676c76-80e1-4239-95bb-83d0f6d0da78}" /> <!-- Windows 8.1/Server 2012 R2 -->
			<supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}" /> <!-- Windows 10 -->
		</application>
	</compatibility>
</assembly>

)";
}
}
