/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
 */

#include "Export/VisualStudio/VSVCXProjGen.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Xml/XmlFile.hpp"

namespace chalet
{
/*****************************************************************************/
VSVCXProjGen::VSVCXProjGen(const BuildState& inState, const std::string& inCwd) :
	m_state(inState),
	m_cwd(inCwd)
{
	UNUSED(m_cwd);
}

/*****************************************************************************/
bool VSVCXProjGen::saveToFile(const std::string& inTargetName)
{
	if (!saveFiltersFile(fmt::format("{name}/{name}.vcxproj.filters", fmt::arg("name", inTargetName))))
		return false;

	if (!saveUserFile(fmt::format("{name}/{name}.vcxproj.user", fmt::arg("name", inTargetName))))
		return false;

	return true;
}

/*****************************************************************************/
bool VSVCXProjGen::saveFiltersFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xml = xmlFile.xml;
	auto& xmlRoot = xml.root();
	xmlRoot.setName("Project");
	xmlRoot.addAttribute("ToolsVersion", "4.0");
	xmlRoot.addAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
	xmlRoot.addElement("ItemGroup", [this](XmlElement& node) {
		node.addElement("Filter", [this](XmlElement& node2) {
			node2.addAttribute("Include", "Source Files");
			node2.addElementWithText("UniqueIdentifier", "{4FC737F1-C7A5-4376-A066-2A32D752A2FF}");
			node2.addElementWithText("Extensions", String::join(getSourceExtensions(), ';'));
		});
		node.addElement("Filter", [this](XmlElement& node2) {
			node2.addAttribute("Include", "Header Files");
			node2.addElementWithText("UniqueIdentifier", "{93995380-89BD-4b04-88EB-625FBE52EBFB}");
			node2.addElementWithText("Extensions", String::join(getHeaderExtensions(), ';'));
		});
		node.addElement("Filter", [this](XmlElement& node2) {
			node2.addAttribute("Include", "Resource Files");
			node2.addElementWithText("UniqueIdentifier", "{67DA6AB6-F800-4c08-8B7A-83BB121AAD01}");
			node2.addElementWithText("Extensions", String::join(getResourceExtensions(), ';'));
		});
	});

	xmlFile.save();

	return true;
}

/*****************************************************************************/
bool VSVCXProjGen::saveUserFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xml = xmlFile.xml;
	auto& xmlRoot = xml.root();
	xmlRoot.setName("Project");
	xmlRoot.addAttribute("ToolsVersion", "Current");
	xmlRoot.addAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
	xmlRoot.addElement("PropertyGroup");

	xmlFile.save();

	return true;
}

/*****************************************************************************/
StringList VSVCXProjGen::getSourceExtensions() const
{
	StringList ret{
		"asmx",
		"asm",
		"bat",
		"hpj",
		"idl",
		"odl",
		"def",
		"ixx",
		"cppm",
		"c++",
		"cxx",
		"cc",
		"c",
		"cpp",
	};

	const auto& fileExtensions = m_state.paths.allFileExtensions();
	const auto& resourceExtensions = m_state.paths.resourceExtensions();

	for (auto& ext : fileExtensions)
	{
		if (String::equals(resourceExtensions, ext))
			continue;

		List::addIfDoesNotExist(ret, ext);
	}

	// MS defaults
	std::reverse(ret.begin(), ret.end());

	return ret;
}

/*****************************************************************************/
StringList VSVCXProjGen::getHeaderExtensions() const
{
	// MS defaults
	return {
		"h",
		"hh",
		"hpp",
		"hxx",
		"h++",
		"hm",
		"inl",
		"inc",
		"ipp",
		"xsd",
	};
}

/*****************************************************************************/
StringList VSVCXProjGen::getResourceExtensions() const
{
	// MS defaults
	return {
		"rc",
		"ico",
		"cur",
		"bmp",
		"dlg",
		"rc2",
		"rct",
		"bin",
		"rgs",
		"gif",
		"jpg",
		"jpeg",
		"jpe",
		"resx",
		"tiff",
		"tif",
		"png",
		"wav",
		"mfcribbon-ms",
	};
}
}
