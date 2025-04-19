/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "FileTemplates/PlatformFileTemplates.hpp"

#include "System/Files.hpp"

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
		case Arch::Cpu::ARMHF:
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
	"CFBundleShortVersionString": "${versionShort}",
	"CFBundleVersion": "${version}",
	"CFBundleSignature": "????",
	"LSMinimumSystemVersion": "${osTargetVersion}"
}
)json";
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
		<windowsSettings>
			<disableWindowFiltering xmlns="http://schemas.microsoft.com/SMI/2011/WindowsSettings">true</disableWindowFiltering>
			<dpiAwareness xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">permonitorv2, permonitor, unaware</dpiAwareness>
      		<longPathAware xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">true</longPathAware>
			<gdiScaling xmlns="http://schemas.microsoft.com/SMI/2017/WindowsSettings">false</gdiScaling>
			<activeCodePage xmlns="http://schemas.microsoft.com/SMI/2019/WindowsSettings">UTF-8</activeCodePage>
			<heapType xmlns="http://schemas.microsoft.com/SMI/2020/WindowsSettings">SegmentHeap</heapType>
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
	auto file = Files::getCanonicalPath(inManifestFile);
	i32 id = inDllPrivateDeps ? 2 : 1;
	auto macroName = inDllPrivateDeps ? "ISOLATIONAWARE_MANIFEST_RESOURCE_ID" : "CREATEPROCESS_MANIFEST_RESOURCE_ID";
	return fmt::format(R"rc(#pragma code_page(65001)
{id} /* {macroName} */ 24 /* RT_MANIFEST */ "{manifestFile}"
)rc",
		FMT_ARG(id),
		FMT_ARG(macroName),
		fmt::arg("manifestFile", file));
}

/*****************************************************************************/
std::string PlatformFileTemplates::windowsIconResource(const std::string& inIconFile)
{
	auto file = Files::getCanonicalPath(inIconFile);
	return fmt::format(R"rc(#pragma code_page(65001)
2 ICON "{iconFile}"
)rc",
		fmt::arg("iconFile", file));
}
}
