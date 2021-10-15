/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "FileTemplates/PlatformFileTemplates.hpp"

namespace chalet
{
namespace
{
const char* getWindowsManifestArch(const Arch::Cpu inCpu)
{
	switch (inCpu)
	{
		case Arch::Cpu::X86:
			return "x86";

		case Arch::Cpu::ARM64:
			return "arm64";

		case Arch::Cpu::ARM:
			return "arm";

		case Arch::Cpu::X64:
		default:
			return "amd64";
	}
}
}
/*****************************************************************************/
std::string PlatformFileTemplates::linuxDesktopEntry()
{
	return R"([Desktop Entry]
Version=1.0
Type=Application
Categories=Application;
Terminal=false
Exec=${mainExecutable}
Path=${path}
Name=${name}
Comment=
Icon=${icon}
)";
}

/*****************************************************************************/
std::string PlatformFileTemplates::macosInfoPlist()
{
	return R"json({
	"CFBundleDevelopmentRegion": "en-US",
	"CFBundleDisplayName": "${name}",
	"CFBundleExecutable": "${mainExecutable}",
	"CFBundleIconFile": "${icon}",
	"CFBundleIdentifier": "com.developer.application",
	"CFBundleInfoDictionaryVersion": "6.0",
	"CFBundleName": "${bundleName}",
	"CFBundlePackageType": "APPL",
	"CFBundleShortVersionString": "${version}",
	"CFBundleVersion": "${version}",
	"CFBundleSignature": "????",
	"LSMinimumSystemVersion": "10.8"
}
)json";
}

/*****************************************************************************/
std::string PlatformFileTemplates::macosDmgApplescript(const std::string& inAppName, const bool inHasBackground)
{
	std::string background;
	if (inHasBackground)
	{
		background = R"applescript(
  set background picture of viewOptions to file ".background:background.tiff"
  set position of item ".background" of container window to {{120, 388}})applescript";
	}
	return fmt::format(R"applescript(set appNameExt to "{appName}.app"
tell application "Finder"
 tell disk "{appName}"
  open
  set current view of container window to icon view
  set toolbar visible of container window to false
  set statusbar visible of container window to false
  set the bounds of container window to {{0, 0, 512, 342}}
  set viewOptions to the icon view options of container window
  set arrangement of viewOptions to not arranged
  set icon size of viewOptions to 80
  set position of item appNameExt of container window to {{120, 188}}
  set position of item "Applications" of container window to {{392, 188}}{background}
  close
  update without registering applications
  delay 2
 end tell
end tell)applescript",
		fmt::arg("appName", inAppName),
		FMT_ARG(background));
}

/*****************************************************************************/
// Note: This is the default for Visual Studio projects
//
std::string PlatformFileTemplates::minimumWindowsAppManifest()
{
	return R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
	<trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
		<security>
			<requestedPrivileges>
				<requestedExecutionLevel level="asInvoker" uiAccess="false" />
			</requestedPrivileges>
		</security>
	</trustInfo>
</assembly>
)xml";
}

/*****************************************************************************/
std::string PlatformFileTemplates::minimumWindowsAppManifestWithCompatibility()
{
	return R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
	<trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
		<security>
			<requestedPrivileges>
				<requestedExecutionLevel level="asInvoker" uiAccess="false" />
			</requestedPrivileges>
		</security>
	</trustInfo>
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
)xml";
}

/*****************************************************************************/
// Note: The default for Visual Studio projects omits assemblyIdentity & description
//   The msys2 package 'mingw-w64-x86_64-windows-default-manifest' also includes
//   supportedOS
//
std::string PlatformFileTemplates::generalWindowsAppManifest(const std::string& inName, const Arch::Cpu inCpu)
{
	auto arch = getWindowsManifestArch(inCpu);
	return fmt::format(R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
	<assemblyIdentity name="{name}" processorArchitecture="{arch}" version="1.0.0.0" type="win32" />
	<description></description>
	<trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
		<security>
			<requestedPrivileges>
				<requestedExecutionLevel level="asInvoker" uiAccess="false" />
			</requestedPrivileges>
		</security>
	</trustInfo>
	<compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
		<application>
			<supportedOS Id="{{e2011457-1546-43c5-a5fe-008deee3d3f0}}" /> <!-- Windows Vista/Server 2008 -->
			<supportedOS Id="{{35138b9a-5d96-4fbd-8e2d-a2440225f93a}}" /> <!-- Windows 7/Server 2008 R2 -->
			<supportedOS Id="{{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}}" /> <!-- Windows 8/Server 2012 -->
			<supportedOS Id="{{1f676c76-80e1-4239-95bb-83d0f6d0da78}}" /> <!-- Windows 8.1/Server 2012 R2 -->
			<supportedOS Id="{{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}}" /> <!-- Windows 10 -->
		</application>
	</compatibility>
</assembly>
)xml",
		fmt::arg("name", inName),
		FMT_ARG(arch));
}

/*****************************************************************************/
std::string PlatformFileTemplates::loadedWindowsAppManifest(const std::string& inName, const Arch::Cpu inCpu)
{
	auto arch = getWindowsManifestArch(inCpu);
	return fmt::format(R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
	<assemblyIdentity name="{name}" processorArchitecture="{arch}" version="1.0.0.0" type="win32" />
	<description></description>
	<trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
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
			<supportedOS Id="{{e2011457-1546-43c5-a5fe-008deee3d3f0}}" /> <!-- Windows Vista/Server 2008 -->
			<supportedOS Id="{{35138b9a-5d96-4fbd-8e2d-a2440225f93a}}" /> <!-- Windows 7/Server 2008 R2 -->
			<supportedOS Id="{{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}}" /> <!-- Windows 8/Server 2012 -->
			<supportedOS Id="{{1f676c76-80e1-4239-95bb-83d0f6d0da78}}" /> <!-- Windows 8.1/Server 2012 R2 -->
			<supportedOS Id="{{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}}" /> <!-- Windows 10 -->
		</application>
	</compatibility>
</assembly>
)xml",
		fmt::arg("name", inName),
		FMT_ARG(arch));
}

/*****************************************************************************/
// https://docs.microsoft.com/en-us/cpp/build/reference/manifest-create-side-by-side-assembly-manifest?view=msvc-160#remarks
// Note: Use a value of 1 for an executable file. Use a value of 2 for a DLL to enable it to specify private dependencies.
//   If the ID parameter is not specified, the default value is 2 if the /DLL option is set; otherwise, the default value is 1.
std::string PlatformFileTemplates::windowsManifestResource(const std::string& inManifestFile, const bool inDllPrivateDeps)
{
	int id = inDllPrivateDeps ? 2 : 1;
	auto macroName = inDllPrivateDeps ? "ISOLATIONAWARE_MANIFEST_RESOURCE_ID" : "CREATEPROCESS_MANIFEST_RESOURCE_ID";
	return fmt::format(R"rc(#pragma code_page(65001)
{id} /* {macroName} */ 24 /* RT_MANIFEST */ "{manifestFile}"
)rc",
		FMT_ARG(id),
		FMT_ARG(macroName),
		fmt::arg("manifestFile", inManifestFile));
}

/*****************************************************************************/
std::string PlatformFileTemplates::windowsIconResource(const std::string& inIconFile)
{
	return fmt::format(R"rc(#pragma code_page(65001)
2 ICON "{iconFile}"
)rc",
		fmt::arg("iconFile", inIconFile));
}
}
