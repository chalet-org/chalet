/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "FileTemplates/PlatformFileTemplates.hpp"

#include "System/Files.hpp"
#include "Xml/Xml.hpp"

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
			return "amd64";
		case Arch::Cpu::Unknown:
		default:
			return "*";
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
// Note: The default for Visual Studio projects only has trustInfo -> security
//
// Note: The msys2 package 'mingw-w64-x86_64-windows-default-manifest' also includes
//   supportedOS
//
std::string PlatformFileTemplates::windowsAppManifest(const WindowsManifestGenSettings& inSettings)
{
	static const char kXmlns[] = "xmlns";

	Xml manifest;
	manifest.setEncoding("UTF-8");
	manifest.setStandalone(true);

	auto& root = manifest.root();
	root.setName("assembly");
	root.addAttribute(kXmlns, "urn:schemas-microsoft-com:asm.v1");
	root.addAttribute("manifestVersion", "1.0");

	if (!inSettings.name.empty())
	{
		root.addElement("assemblyIdentity", [&inSettings](XmlElement& node) {
			node.addAttribute("name", inSettings.name);
			node.addAttribute("type", "win32");
			node.addAttribute("version", inSettings.version.majorMinorPatchTweak());
			node.addAttribute("processorArchitecture", getWindowsManifestArch(inSettings.cpu));
		});
		root.addElementWithText("description", "");
	}

	/*
		<assemblyIdentity name="{name}" processorArchitecture="{arch}" version="1.0.0.0" type="win32" />
		<description></description>
	*/

	root.addElement("trustInfo", [](XmlElement& node) {
		node.addAttribute(kXmlns, "urn:schemas-microsoft-com:asm.v2");
		node.addElement("security", [](XmlElement& node2) {
			node2.addElement("requestedPrivileges", [](XmlElement& node3) {
				node3.addAttribute(kXmlns, "urn:schemas-microsoft-com:asm.v3");
				node3.addElement("requestedExecutionLevel", [](XmlElement& node4) {
					node4.addAttribute("level", "asInvoker");
					node4.addAttribute("uiAccess", "false");
				});
			});
		});
	});

	bool hasWindowsSettings = inSettings.disableWindowFiltering
		|| inSettings.dpiAwareness
		|| inSettings.longPathAware
		|| inSettings.disableGdiScaling
		|| inSettings.unicode
		|| inSettings.segmentHeap;

	if (hasWindowsSettings)
	{
		root.addElement("application", [&inSettings](XmlElement& node) {
			node.addAttribute(kXmlns, "urn:schemas-microsoft-com:asm.v3");
			node.addElement("windowsSettings", [&inSettings](XmlElement& node2) {
				if (inSettings.disableWindowFiltering)
				{
					node2.addElement("disableWindowFiltering", [](XmlElement& node3) {
						node3.addAttribute(kXmlns, "http://schemas.microsoft.com/SMI/2011/WindowsSettings");
						node3.setText("true");
					});
				}
				if (inSettings.dpiAwareness)
				{
					node2.addElement("dpiAwareness", [](XmlElement& node3) {
						node3.addAttribute(kXmlns, "http://schemas.microsoft.com/SMI/2016/WindowsSettings");
						node3.setText("permonitorv2, permonitor, unaware");
					});
				}
				if (inSettings.longPathAware)
				{
					node2.addElement("longPathAware", [](XmlElement& node3) {
						node3.addAttribute(kXmlns, "http://schemas.microsoft.com/SMI/2016/WindowsSettings");
						node3.setText("true");
					});
				}
				if (inSettings.disableGdiScaling)
				{
					node2.addElement("gdiScaling", [](XmlElement& node3) {
						node3.addAttribute(kXmlns, "http://schemas.microsoft.com/SMI/2017/WindowsSettings");
						node3.setText("false");
					});
				}
				if (inSettings.unicode)
				{
					node2.addElement("activeCodePage", [](XmlElement& node3) {
						node3.addAttribute(kXmlns, "http://schemas.microsoft.com/SMI/2019/WindowsSettings");
						node3.setText("UTF-8");
					});
				}
				if (inSettings.segmentHeap)
				{
					node2.addElement("heapType", [](XmlElement& node3) {
						node3.addAttribute(kXmlns, "http://schemas.microsoft.com/SMI/2020/WindowsSettings");
						node3.setText("SegmentHeap");
					});
				}
			});
		});
	}

	if (inSettings.compatibility)
	{
		root.addElement("compatibility", [](XmlElement& node) {
			node.addAttribute(kXmlns, "urn:schemas-microsoft-com:compatibility.v1");
			node.addElement("application", [](XmlElement& node2) {
				node2.addElement("supportedOS", [](XmlElement& node3) {
					node3.addAttribute("Id", "{e2011457-1546-43c5-a5fe-008deee3d3f0}");
					node3.setComment("Windows Vista/Server 2008");
				});
				node2.addElement("supportedOS", [](XmlElement& node3) {
					node3.addAttribute("Id", "{35138b9a-5d96-4fbd-8e2d-a2440225f93a}");
					node3.setComment("Windows 7/Server 2008 R2");
				});
				node2.addElement("supportedOS", [](XmlElement& node3) {
					node3.addAttribute("Id", "{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}");
					node3.setComment("Windows 8/Server 2012");
				});
				node2.addElement("supportedOS", [](XmlElement& node3) {
					node3.addAttribute("Id", "{1f676c76-80e1-4239-95bb-83d0f6d0da78}");
					node3.setComment("Windows 8.1/Server 2012 R2");
				});
				node2.addElement("supportedOS", [](XmlElement& node3) {
					node3.addAttribute("Id", "{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}");
					node3.setComment("Windows 10");
				});
			});
		});
	}

	return manifest.dump(1, ' ');
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
