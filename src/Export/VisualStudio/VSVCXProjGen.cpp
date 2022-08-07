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
#include "State/CentralState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Xml/XmlFile.hpp"

namespace chalet
{
/*****************************************************************************/
VSVCXProjGen::VSVCXProjGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inCwd, const std::string& inProjectTypeGuid, const OrderedDictionary<Uuid>& inTargetGuids) :
	m_states(inStates),
	m_cwd(inCwd),
	m_projectTypeGuid(inProjectTypeGuid),
	m_targetGuids(inTargetGuids)
{
	UNUSED(m_cwd, m_targetGuids);
}

/*****************************************************************************/
bool VSVCXProjGen::saveProjectFiles(const BuildState& inState, const SourceTarget& inProject)
{
	const auto& name = inProject.name();
	if (m_targetGuids.find(name) == m_targetGuids.end())
		return false;

	m_currentTarget = name;
	m_currentGuid = m_targetGuids.at(name).str();

	auto projectFile = fmt::format("{name}/{name}.vcxproj", FMT_ARG(name));

	if (!saveProjectFile(inState, inProject, projectFile))
		return false;

	if (!saveFiltersFile(inState, fmt::format("{}.filters", projectFile)))
		return false;

	if (!saveUserFile(fmt::format("{}.user", projectFile)))
		return false;

	return true;
}

/*****************************************************************************/
bool VSVCXProjGen::saveProjectFile(const BuildState& inState, const SourceTarget& inProject, const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xml = xmlFile.xml;
	auto& xmlRoot = xml.root();
	xmlRoot.setName("Project");
	xmlRoot.addAttribute("DefaultTargets", "Build");
	xmlRoot.addAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");

	const SourceTarget* project = nullptr;
	for (auto& state : m_states)
	{
		for (auto& target : state->targets)
		{
			if (target->isSources() && String::equals(target->name(), inProject.name()))
			{
				project = static_cast<const SourceTarget*>(target.get());
			}
		}
	}

	xmlRoot.addElement("ItemGroup", [this](XmlElement& node) {
		node.addAttribute("Label", "ProjectConfigurations");
		for (auto& state : m_states)
		{
			const auto& name = state->configuration.name();
			auto arch = Arch::toVSArch2(state->info.targetArchitecture());
			node.addElement("ProjectConfiguration", [&name, &arch](XmlElement& node2) {
				node2.addAttribute("Include", fmt::format("{}|{}", name, arch));
				node2.addElementWithText("Configuration", name);
				node2.addElementWithText("Platform", arch);
			});
		}
	});

	xmlRoot.addElement("PropertyGroup", [this, &inState](XmlElement& node) {
		std::string visualStudioVersion = inState.environment->detectedVersion();
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

	for (auto& state : m_states)
	{
		const auto& config = state->configuration;
		const auto& name = config.name();
		xmlRoot.addElement("PropertyGroup", [this, &name, &config, project](XmlElement& node) {
			node.addAttribute("Condition", fmt::format("'$(Configuration)|$(Platform)'=='{}|x64'", name));
			node.addAttribute("Label", "Configuration");
			node.addElementWithText("ConfigurationType", "Application");
			node.addElementWithText("UseDebugLibraries", getBooleanValue(config.debugSymbols()));
			node.addElementWithText("PlatformToolset", "v143");

			if (config.interproceduralOptimization())
			{
				node.addElementWithText("WholeProgramOptimization", getBooleanValue(true));
			}

			if (String::equals({ "UTF-8", "utf-8" }, project->executionCharset()))
			{
				node.addElementWithText("CharacterSet", "Unicode");
			}
		});
	}

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

	for (auto& state : m_states)
	{
		const auto& name = state->configuration.name();
		xmlRoot.addElement("ImportGroup", [&name](XmlElement& node) {
			node.addAttribute("Label", "PropertySheets");
			node.addAttribute("Condition", fmt::format("'$(Configuration)|$(Platform)'=='{}|x64'", name));
			node.addElement("Import", [](XmlElement& node2) {
				node2.addAttribute("Project", "$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props");
				node2.addAttribute("Condition", "exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')");
				node2.addAttribute("Label", "LocalAppDataPlatform");
			});
		});
	}

	xmlRoot.addElement("PropertyGroup", [](XmlElement& node) {
		node.addAttribute("Label", "UserMacros");
	});

	for (auto& state : m_states)
	{
		const auto& config = state->configuration;
		const auto& name = config.name();
		xmlRoot.addElement("PropertyGroup", [this, &name, &config](XmlElement& node) {
			node.addAttribute("Condition", fmt::format("'$(Configuration)|$(Platform)'=='{}|x64'", name));

			if (config.debugSymbols() && !config.enableSanitizers() && !config.enableProfiling())
			{
				node.addElementWithText("LinkIncremental", getBooleanValue(true));
			}
		});
	}

	for (auto& state : m_states)
	{
		const auto& config = state->configuration;
		const auto& name = config.name();
		uint versionMajorMinor = state->toolchain.compilerCxx(inProject.language()).versionMajorMinor;

		if (project != nullptr)
		{
			xmlRoot.addElement("ItemDefinitionGroup", [this, project, &config, &name, &versionMajorMinor](XmlElement& node) {
				node.addAttribute("Condition", fmt::format("'$(Configuration)|$(Platform)'=='{}|x64'", name));
				node.addElement("ClCompile", [this, project, &config, &versionMajorMinor](XmlElement& node2) {
					bool debugSymbols = config.debugSymbols();
					auto warningLevel = getWarningLevel(project->getMSVCWarningLevel());
					if (!warningLevel.empty())
					{
						node2.addElementWithText("WarningLevel", warningLevel);
					}
					if (!debugSymbols)
					{
						node2.addElementWithText("FunctionLevelLinking", "true");
						node2.addElementWithText("IntrinsicFunctions", "true");
					}
					node2.addElementWithText("SDLCheck", "true");
					{
						auto defines = String::join(project->defines(), ';');
						if (!defines.empty())
							defines += ';';

						node2.addElementWithText("PreprocessorDefinitions", fmt::format("{}%(PreprocessorDefinitions)", defines));
					}
					if (versionMajorMinor >= 1910) // VS 2017+
					{
						node2.addElementWithText("ConformanceMode", "true");
					}
				});

				node.addElement("Link", [this, project, &config](XmlElement& node2) {
					node2.addElementWithText("SubSystem", getMsvcCompatibleSubSystem(*project));
					if (!config.debugSymbols())
					{
						node2.addElementWithText("EnableCOMDATFolding", "true");
						node2.addElementWithText("OptimizeReferences", "true");
					}
					node2.addElementWithText("GenerateDebugInformation", "true");
				});
			});
		}
	}

	auto headerFiles = inProject.getHeaderFiles();
	if (!inProject.precompiledHeader().empty())
		headerFiles.push_back(inProject.precompiledHeader());

	if (!headerFiles.empty())
	{
		xmlRoot.addElement("ItemGroup", [this, &headerFiles](XmlElement& node) {
			node.addElement("ClInclude", [this, &headerFiles](XmlElement& node2) {
				for (auto& file : headerFiles)
				{
					node2.addAttribute("Include", fmt::format("{}/{}", m_cwd, file));
				}
			});
		});
	}
	const auto& files = inProject.files();
	if (!files.empty())
	{
		xmlRoot.addElement("ItemGroup", [this, &files](XmlElement& node) {
			node.addElement("ClCompile", [this, &files](XmlElement& node2) {
				for (auto& file : files)
				{
					node2.addAttribute("Include", fmt::format("{}/{}", m_cwd, file));
				}
			});
		});
	}
	// Resource files
	/*xmlRoot.addElement("ItemGroup", [&inProject](XmlElement& node) {
		node.addElement("ResourceCompile", [&inProject](XmlElement& node2) {
			node2.addAttribute("Include", file);
		});
	});*/
	// Ico files
	/*xmlRoot.addElement("ItemGroup", [&inProject](XmlElement& node) {
		node.addElement("Image", [&inProject](XmlElement& node2) {
			node2.addAttribute("Include", file);
		});
	});*/

	xmlRoot.addElement("Import", [](XmlElement& node) {
		node.addAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.targets");
	});
	xmlRoot.addElement("ImportGroup", [](XmlElement& node) {
		node.addAttribute("Label", "ExtensionTargets");
	});

	return xmlFile.save();
}

/*****************************************************************************/
bool VSVCXProjGen::saveFiltersFile(const BuildState& inState, const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xml = xmlFile.xml;
	auto& xmlRoot = xml.root();
	xmlRoot.setName("Project");
	xmlRoot.addAttribute("ToolsVersion", "4.0");
	xmlRoot.addAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
	xmlRoot.addElement("ItemGroup", [this, &inState](XmlElement& node) {
		node.addElement("Filter", [this, &inState](XmlElement& node2) {
			auto extensions = String::join(getSourceExtensions(inState), ';');
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
std::string VSVCXProjGen::getWarningLevel(const MSVCWarningLevel inLevel) const
{
	std::string ret;

	switch (inLevel)
	{
		case MSVCWarningLevel::Level1:
			ret = "Level1";
			break;

		case MSVCWarningLevel::Level2:
			ret = "Level2";
			break;

		case MSVCWarningLevel::Level3:
			ret = "Level3";
			break;

		case MSVCWarningLevel::Level4:
			ret = "Level4";
			break;

		case MSVCWarningLevel::LevelAll:
			ret = "LevelAll";
			break;

		default:
			break;
	}

	return ret;
}

/*****************************************************************************/
std::string VSVCXProjGen::getMsvcCompatibleSubSystem(const SourceTarget& inProject) const
{
	const WindowsSubSystem subSystem = inProject.windowsSubSystem();
	switch (subSystem)
	{
		case WindowsSubSystem::Windows:
			return "Windows";
		case WindowsSubSystem::Native:
			return "Native";
		case WindowsSubSystem::Posix:
			return "POSIX";
		case WindowsSubSystem::EfiApplication:
			return "EFI Application";
		case WindowsSubSystem::EfiBootServiceDriver:
			return "EFI Boot Service Driver";
		case WindowsSubSystem::EfiRom:
			return "EFI ROM";
		case WindowsSubSystem::EfiRuntimeDriver:
			return "EFI Runtime";

		case WindowsSubSystem::BootApplication:
		default:
			break;
	}

	return "Console";
}

/*****************************************************************************/
std::string VSVCXProjGen::getBooleanValue(const bool inValue) const
{
	return std::string(inValue ? "true" : "false");
}

/*****************************************************************************/
StringList VSVCXProjGen::getSourceExtensions(const BuildState& inState) const
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

	const auto& fileExtensions = inState.paths.allFileExtensions();
	const auto& resourceExtensions = inState.paths.resourceExtensions();

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
