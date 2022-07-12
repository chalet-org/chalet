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
#include "Utility/Uuid.hpp"
#include "Xml/XmlFile.hpp"

namespace chalet
{
/*****************************************************************************/
VSVCXProjGen::VSVCXProjGen(const BuildState& inState, const std::string& inCwd, const std::string& inProjectTypeGuid, const OrderedDictionary<std::string>& inTargetGuids) :
	m_state(inState),
	m_cwd(inCwd),
	m_projectTypeGuid(inProjectTypeGuid),
	m_targetGuids(inTargetGuids)
{
	UNUSED(m_cwd, m_targetGuids);
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
			auto extensions = String::join(getSourceExtensions(), ';');
			auto guid = Uuid::v5(extensions, m_projectTypeGuid).toUpperCase();
			node2.addAttribute("Include", "Source Files");
			node2.addElementWithText("UniqueIdentifier", fmt::format("{{{}}}", guid));
			node2.addElementWithText("Extensions", extensions);
		});
		node.addElement("Filter", [this](XmlElement& node2) {
			auto extensions = String::join(getHeaderExtensions(), ';');
			auto guid = Uuid::v5(extensions, m_projectTypeGuid).toUpperCase();
			node2.addAttribute("Include", "Header Files");
			node2.addElementWithText("UniqueIdentifier", fmt::format("{{{}}}", guid));
			node2.addElementWithText("Extensions", extensions);
		});
		node.addElement("Filter", [this](XmlElement& node2) {
			auto extensions = String::join(getResourceExtensions(), ';');
			auto guid = Uuid::v5(extensions, m_projectTypeGuid).toUpperCase();
			node2.addAttribute("Include", "Resource Files");
			node2.addElementWithText("UniqueIdentifier", fmt::format("{{{}}}", guid));
			node2.addElementWithText("Extensions", extensions);
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
