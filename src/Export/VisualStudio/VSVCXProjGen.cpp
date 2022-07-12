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
VSVCXProjGen::VSVCXProjGen(const BuildState& inState, const std::string& inCwd, const std::string& inProjectTypeGuid, const OrderedDictionary<Uuid>& inTargetGuids) :
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
	if (m_targetGuids.find(inTargetName) == m_targetGuids.end())
		return false;

	m_currentTarget = inTargetName;
	m_currentGuid = m_targetGuids.at(inTargetName).str();

	if (!saveProjectFile(fmt::format("{name}/{name}.vcxproj", fmt::arg("name", inTargetName))))
		return false;

	if (!saveFiltersFile(fmt::format("{name}/{name}.vcxproj.filters", fmt::arg("name", inTargetName))))
		return false;

	if (!saveUserFile(fmt::format("{name}/{name}.vcxproj.user", fmt::arg("name", inTargetName))))
		return false;

	return true;
}

/*****************************************************************************/
bool VSVCXProjGen::saveProjectFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xml = xmlFile.xml;
	auto& xmlRoot = xml.root();
	xmlRoot.setName("Project");
	xmlRoot.addAttribute("DefaultTargets", "Build");
	xmlRoot.addAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");

	xmlRoot.addElement("ItemGroup", [](XmlElement& node) {
		node.addAttribute("Label", "ProjectConfigurations");
		node.addElement("ProjectConfiguration", [](XmlElement& node2) {
			node2.addAttribute("Include", "Debug|Win32");
			node2.addElementWithText("Configuration", "Debug");
			node2.addElementWithText("Platform", "Win32");
		});
	});

	xmlRoot.addElement("PropertyGroup", [this](XmlElement& node) {
		std::string visualStudioVersion = m_state.environment->detectedVersion();
		{
			auto decimal = visualStudioVersion.find('.');
			if (decimal != std::string::npos)
			{
				decimal = visualStudioVersion.find('.', decimal + 1);
				if (decimal != std::string::npos)
					visualStudioVersion = visualStudioVersion.substr(0, decimal);
				else
					visualStudioVersion.clear();
			}
			else
				visualStudioVersion.clear();
		}

		node.addAttribute("Label", "Globals");
		node.addElementWithText("VCProjectVersion", visualStudioVersion);
		node.addElementWithText("Keyword", "Win32Proj");
		node.addElementWithText("ProjectGuid", fmt::format("{{{}}}", m_currentGuid));
		node.addElementWithText("RootNamespace", m_currentTarget);
		node.addElementWithText("WindowsTargetPlatformVersion", "10.0");
	});

	xmlRoot.addElement("Import", [](XmlElement& node) {
		node.addAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props");
	});
	xmlRoot.addElement("PropertyGroup", [](XmlElement& node) {
		node.addAttribute("Condition", "'$(Configuration)|$(Platform)'=='Release|x64'");
		node.addAttribute("Label", "Configuration");
		node.addElementWithText("ConfigurationType", "Application");
		node.addElementWithText("UseDebugLibraries", "false");
		node.addElementWithText("PlatformToolset", "v143");
		node.addElementWithText("WholeProgramOptimization", "true");
		node.addElementWithText("CharacterSet", "Unicode");
	});

	xmlRoot.addElement("Import", [](XmlElement& node) {
		node.addAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.props");
	});

	xmlRoot.addElement("ImportGroup", [](XmlElement& node) {
		node.addAttribute("Label", "ExtensionSettings");
		node.setText(std::string());
	});

	xmlRoot.addElement("ImportGroup", [](XmlElement& node) {
		node.addAttribute("Label", "Shared");
		node.setText(std::string());
	});

	// TODO: For each build configuration
	xmlRoot.addElement("ImportGroup", [](XmlElement& node) {
		node.addAttribute("Label", "PropertySheets");
		node.addAttribute("Condition", "'$(Configuration)|$(Platform)'=='Release|x64'");
		node.addElement("Import", [](XmlElement& node2) {
			node2.addAttribute("Project", "$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props");
			node2.addAttribute("Condition", "exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')");
			node2.addAttribute("Label", "LocalAppDataPlatform");
		});
	});

	xmlRoot.addElement("PropertyGroup", [](XmlElement& node) {
		node.addAttribute("Label", "UserMacros");
	});

	// TODO: For each build configuration
	xmlRoot.addElement("PropertyGroup", [](XmlElement& node) {
		node.addAttribute("Condition", "'$(Configuration)|$(Platform)'=='Release|x64'");
		node.addElementWithText("LinkIncremental", "false");
	});
	xmlRoot.addElement("ItemDefinitionGroup", [](XmlElement& node) {
		node.addAttribute("Condition", "'$(Configuration)|$(Platform)'=='Release|x64'");
		node.addElement("ClCompile");
		node.addElement("Link");
	});

	xmlRoot.addElement("ItemGroup", [](XmlElement& node) {
		node.addElement("ClCompile", [](XmlElement& node2) {
			//
			node2.addAttribute("Include", "main.cpp");
		});
	});
	xmlRoot.addElement("Import", [](XmlElement& node) {
		node.addAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.targets");
	});
	xmlRoot.addElement("ImportGroup", [](XmlElement& node) {
		node.addAttribute("Label", "ExtensionTargets");
	});

	return xmlFile.save();
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

	return xmlFile.save();
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

	return xmlFile.save();
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
