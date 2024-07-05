/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
 */

#include "Export/VisualStudio/VSVCXProjGen.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/TargetExportAdapter.hpp"
#include "Export/VisualStudio/ProjectAdapterVCXProj.hpp"
#include "Process/Environment.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Xml/XmlFile.hpp"

namespace chalet
{
/*****************************************************************************/
VSVCXProjGen::VSVCXProjGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inExportPath, const std::string& inProjectTypeGuid, const OrderedDictionary<Uuid>& inTargetGuids) :
	m_states(inStates),
	m_exportPath(inExportPath),
	m_projectTypeGuid(inProjectTypeGuid),
	m_targetGuids(inTargetGuids)
{
}

/*****************************************************************************/
VSVCXProjGen::~VSVCXProjGen() = default;

/*****************************************************************************/
std::vector<VSVCXProjGen::VisualStudioConfig> VSVCXProjGen::getVisualStudioConfigs() const
{
	std::vector<VisualStudioConfig> ret;

	for (auto& state : m_states)
	{
		ret.emplace_back(VisualStudioConfig{
			state.get(),
			getDictionaryKey(*state),
			getCondition(*state),
		});
	}

	return ret;
}

/*****************************************************************************/
bool VSVCXProjGen::saveSourceTargetProjectFiles(const std::string& name)
{
	if (m_targetGuids.find(name) == m_targetGuids.end())
		return false;

	m_currentTarget = name;
	m_currentGuid = m_targetGuids.at(name).str();

	m_vsConfigs = getVisualStudioConfigs();

	m_adapters.clear();

	for (auto& conf : m_vsConfigs)
	{
		auto project = getProjectFromStateContext(*conf.state, name);
		if (project != nullptr)
		{
			StringList fileCache;
			m_outputs.emplace(conf.key, conf.state->paths.getOutputs(*project, fileCache));

			auto [it, _] = m_adapters.emplace(conf.key, std::make_unique<ProjectAdapterVCXProj>(*conf.state, *project));
			if (!it->second->createPrecompiledHeaderSource())
			{
				Diagnostic::error("Error generating the precompiled header.");
				return false;
			}
			if (!it->second->createWindowsResources())
			{
				Diagnostic::error("Error generating windows resources.");
				return false;
			}
		}
	}

	if (m_adapters.empty())
		return false;

	auto projectFile = makeSubDirectoryAndGetProjectFile(name);

	XmlFile filtersFile(fmt::format("{}.filters", projectFile));
	if (!saveFiltersFile(filtersFile, BuildTargetType::Source))
		return false;

	if (!saveSourceTargetProjectFile(name, projectFile, filtersFile))
		return false;

	if (!saveUserFile(fmt::format("{}.user", projectFile), name))
		return false;

	if (!filtersFile.save())
		return false;

	return true;
}

/*****************************************************************************/
bool VSVCXProjGen::saveScriptTargetProjectFiles(const std::string& name)
{
	if (m_targetGuids.find(name) == m_targetGuids.end())
		return false;

	m_currentTarget = name;
	m_currentGuid = m_targetGuids.at(name).str();

	m_vsConfigs = getVisualStudioConfigs();

	m_targetAdapters.clear();

	for (auto& conf : m_vsConfigs)
	{
		auto target = getTargetFromStateContext(*conf.state, name);
		if (target != nullptr)
		{
			m_targetAdapters.emplace(conf.key, std::make_unique<TargetExportAdapter>(*conf.state, *target));
		}
	}

	if (m_targetAdapters.empty())
		return false;

	auto projectFile = makeSubDirectoryAndGetProjectFile(name);

	XmlFile filtersFile(fmt::format("{}.filters", projectFile));
	if (!saveFiltersFile(filtersFile, BuildTargetType::Script))
		return false;

	if (!saveScriptTargetProjectFile(name, projectFile, filtersFile))
		return false;

	if (!saveUserFile(fmt::format("{}.user", projectFile), name))
		return false;

	if (!filtersFile.save())
		return false;

	return true;
}

/*****************************************************************************/
bool VSVCXProjGen::saveAllBuildTargetProjectFiles(const std::string& name)
{
	if (m_targetGuids.find(name) == m_targetGuids.end())
		return false;

	m_currentTarget = name;
	m_currentGuid = m_targetGuids.at(name).str();

	m_vsConfigs = getVisualStudioConfigs();

	// m_targetAdapters.clear();

	// for (auto& conf : m_vsConfigs)
	// {
	// 	auto target = getTargetFromStateContext(*conf.state, name);
	// 	if (target != nullptr)
	// 	{
	// 		m_targetAdapters.emplace(conf.key, std::make_unique<TargetExportAdapter>(*conf.state, *target));
	// 	}
	// }

	// if (m_targetAdapters.empty())
	// 	return false;

	auto projectFile = makeSubDirectoryAndGetProjectFile(name);

	XmlFile filtersFile(fmt::format("{}.filters", projectFile));
	if (!saveFiltersFile(filtersFile, BuildTargetType::Unknown))
		return false;

	if (!saveAllTargetProjectFile(name, projectFile))
		return false;

	if (!saveUserFile(fmt::format("{}.user", projectFile), name))
		return false;

	if (!filtersFile.save())
		return false;

	return true;
}

/*****************************************************************************/
std::string VSVCXProjGen::makeSubDirectoryAndGetProjectFile(const std::string& inName) const
{
	auto path = fmt::format("{}/vcxproj", m_exportPath);
	if (!Files::pathExists(path))
		Files::makeDirectory(path);

	return fmt::format("{}/{}.vcxproj", path, inName);
}

/*****************************************************************************/
bool VSVCXProjGen::saveSourceTargetProjectFile(const std::string& inName, const std::string& inFilename, XmlFile& outFiltersFile)
{
	XmlFile xmlFile(inFilename);

	auto& xmlRoot = xmlFile.getRoot();

	addProjectHeader(xmlRoot);

	addProjectConfiguration(xmlRoot);
	addGlobalProperties(xmlRoot, BuildTargetType::Source);
	addMsCppDefaultProps(xmlRoot);
	addConfigurationProperties(xmlRoot, BuildTargetType::Source);
	addMsCppProps(xmlRoot);
	addExtensionSettings(xmlRoot);
	addShared(xmlRoot);
	addPropertySheets(xmlRoot);
	addUserMacros(xmlRoot);
	addGeneralProperties(xmlRoot, inName, BuildTargetType::Source);
	addCompileProperties(xmlRoot);
	addSourceFiles(xmlRoot, inName, outFiltersFile);
	addProjectReferences(xmlRoot, inName);
	addImportMsCppTargets(xmlRoot);
	addExtensionTargets(xmlRoot);

	return xmlFile.save();
}

/*****************************************************************************/
bool VSVCXProjGen::saveScriptTargetProjectFile(const std::string& inName, const std::string& inFilename, XmlFile& outFiltersFile)
{
	XmlFile xmlFile(inFilename);

	auto& xmlRoot = xmlFile.getRoot();

	addProjectHeader(xmlRoot);

	addProjectConfiguration(xmlRoot);
	addGlobalProperties(xmlRoot, BuildTargetType::Script);
	addMsCppDefaultProps(xmlRoot);
	addConfigurationProperties(xmlRoot, BuildTargetType::Script);
	addMsCppProps(xmlRoot);
	addUserMacros(xmlRoot);
	addGeneralProperties(xmlRoot, inName, BuildTargetType::Script);
	addScriptProperties(xmlRoot);
	addTargetFiles(xmlRoot, inName, outFiltersFile);
	addProjectReferences(xmlRoot, inName);
	addImportMsCppTargets(xmlRoot);

	return xmlFile.save();
}

/*****************************************************************************/
bool VSVCXProjGen::saveAllTargetProjectFile(const std::string& inName, const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xmlRoot = xmlFile.getRoot();

	addProjectHeader(xmlRoot);

	addProjectConfiguration(xmlRoot);
	addGlobalProperties(xmlRoot, BuildTargetType::Unknown);
	addMsCppDefaultProps(xmlRoot);
	addConfigurationProperties(xmlRoot, BuildTargetType::Unknown);
	addMsCppProps(xmlRoot);
	addUserMacros(xmlRoot);
	addGeneralProperties(xmlRoot, inName, BuildTargetType::Unknown);
	//
	addAllTargetFiles(xmlRoot);
	addAllProjectReferences(xmlRoot);
	addImportMsCppTargets(xmlRoot);

	return xmlFile.save();
}

/*****************************************************************************/
bool VSVCXProjGen::saveFiltersFile(XmlFile& outFile, const BuildTargetType inType)
{
	auto& xmlRoot = outFile.getRoot();

	xmlRoot.setName("Project");
	xmlRoot.addAttribute("ToolsVersion", "4.0");
	xmlRoot.addAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");

	if (inType == BuildTargetType::Source)
	{
		xmlRoot.addElement("ItemGroup", [this](XmlElement& node) {
			node.addElement("Filter", [this](XmlElement& node2) {
				std::string filterName{ "Source Files" };
				auto guid = Uuid::v5(filterName, m_projectTypeGuid).toUpperCase();
				node2.addAttribute("Include", filterName);
				node2.addElementWithText("UniqueIdentifier", fmt::format("{{{}}}", guid));
			});
			node.addElement("Filter", [this](XmlElement& node2) {
				std::string filterName{ "Header Files" };
				auto guid = Uuid::v5(filterName, m_projectTypeGuid).toUpperCase();
				node2.addAttribute("Include", filterName);
				node2.addElementWithText("UniqueIdentifier", fmt::format("{{{}}}", guid));
			});
			node.addElement("Filter", [this](XmlElement& node2) {
				std::string filterName{ "Resource Files" };
				auto guid = Uuid::v5(filterName, m_projectTypeGuid).toUpperCase();
				node2.addAttribute("Include", filterName);
				node2.addElementWithText("UniqueIdentifier", fmt::format("{{{}}}", guid));
			});
			node.addElement("Filter", [this](XmlElement& node2) {
				std::string filterName{ "Precompile Header Files" };
				auto guid = Uuid::v5(filterName, m_projectTypeGuid).toUpperCase();
				node2.addAttribute("Include", filterName);
				node2.addElementWithText("UniqueIdentifier", fmt::format("{{{}}}", guid));
			});
		});
	}
	/*else if (inType == BuildTargetType::Script)
	{
		xmlRoot.addElement("ItemGroup", [this](XmlElement& node) {
			node.addElement("Filter", [this](XmlElement& node2) {
				std::string filterName{ "Files" };
				auto guid = Uuid::v5(filterName, m_projectTypeGuid).toUpperCase();
				node2.addAttribute("Include", filterName);
				node2.addElementWithText("UniqueIdentifier", fmt::format("{{{}}}", guid));
			});
		});
	}*/
	// we'll use this for the all target
	else if (inType == BuildTargetType::Unknown)
	{
		xmlRoot.addElement("ItemGroup", [this](XmlElement& node) {
			node.addElement("None", [this](XmlElement& node2) {
				node2.addAttribute("Include", getResolvedInputFile());
			});
		});
	}

	return true;
}

/*****************************************************************************/
bool VSVCXProjGen::saveUserFile(const std::string& inFilename, const std::string& name)
{
	XmlFile xmlFile(inFilename);

	auto& xmlRoot = xmlFile.getRoot();

	xmlRoot.setName("Project");
	xmlRoot.addAttribute("ToolsVersion", "Current");
	xmlRoot.addAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");

	StringList arguments;
	for (auto& conf : m_vsConfigs)
	{
		auto project = getProjectFromStateContext(*conf.state, name);
		if (project != nullptr)
		{
			if (!conf.state->getRunTargetArguments(arguments, project))
				return false;
		}
	}

	if (!arguments.empty())
	{
		xmlRoot.addElement("PropertyGroup", [&arguments](XmlElement& node2) {
			node2.addElementWithText("LocalDebuggerCommandArguments", String::join(arguments));
			node2.addElementWithText("DebuggerFlavor", "WindowsLocalDebugger");
		});
	}
	else
	{
		xmlRoot.addElement("PropertyGroup");
	}

	return xmlFile.save();
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
// .vcxproj file
/*****************************************************************************/
void VSVCXProjGen::addProjectHeader(XmlElement& outNode) const
{
	outNode.setName("Project");
	outNode.addAttribute("ToolsVersion", "4.0");
	outNode.addAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
}

/*****************************************************************************/
void VSVCXProjGen::addProjectConfiguration(XmlElement& outNode) const
{
	outNode.addElement("ItemGroup", [this](XmlElement& node) {
		node.addAttribute("Label", "ProjectConfigurations");
		for (auto& conf : m_vsConfigs)
		{
			const auto& name = conf.state->configuration.name();
			auto arch = Arch::toVSArch2(conf.state->info.targetArchitecture());

			node.addElement("ProjectConfiguration", [&name, &arch](XmlElement& node2) {
				node2.addAttribute("Include", fmt::format("{}|{}", name, arch));
				node2.addElementWithText("Configuration", name);
				node2.addElementWithText("Platform", arch);
			});
		}
	});
}

/*****************************************************************************/
void VSVCXProjGen::addGlobalProperties(XmlElement& outNode, const BuildTargetType inType) const
{
	outNode.addElement("PropertyGroup", [this, inType](XmlElement& node) {
		auto visualStudioVersion = getVisualStudioVersion();

		node.addAttribute("Label", "Globals");
		node.addElementWithText("VCProjectVersion", visualStudioVersion);
		node.addElementWithText("ProjectGuid", fmt::format("{{{}}}", m_currentGuid));
		node.addElementWithText("RootNamespace", m_currentTarget);

		if (inType == BuildTargetType::Source)
		{
			node.addElementWithText("WindowsTargetPlatformVersion", getWindowsTargetPlatformVersion());
			node.addElementWithText("Keyword", "Win32Proj");
			node.addElementWithText("ProjectName", m_currentTarget);
			node.addElementWithText("VCProjectUpgraderObjectName", "NoUpgrade");
		}
		else if (inType == BuildTargetType::Script)
		{
			node.addElementWithText("DisableFastUpToDateCheck", "true");
		}
		else if (inType == BuildTargetType::Unknown)
		{
			node.addElementWithText("DisableFastUpToDateCheck", "true");
		}
	});
}

/*****************************************************************************/
void VSVCXProjGen::addMsCppDefaultProps(XmlElement& outNode) const
{
	outNode.addElement("Import", [](XmlElement& node) {
		node.addAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props");
	});
}

/*****************************************************************************/
void VSVCXProjGen::addConfigurationProperties(XmlElement& outNode, const BuildTargetType inType) const
{
	if (inType == BuildTargetType::Source)
	{
		for (auto& conf : m_vsConfigs)
		{
			if (m_adapters.find(conf.key) != m_adapters.end())
			{
				const auto& vcxprojAdapter = *m_adapters.at(conf.key);

				outNode.addElement("PropertyGroup", [&conf, &vcxprojAdapter](XmlElement& node) {
					node.addAttribute("Condition", conf.condition);
					node.addAttribute("Label", "Configuration");

					// General Tab
					node.addElementWithTextIfNotEmpty("ConfigurationType", vcxprojAdapter.getConfigurationType());
					node.addElementWithTextIfNotEmpty("UseDebugLibraries", vcxprojAdapter.getUseDebugLibraries());
					node.addElementWithTextIfNotEmpty("PlatformToolset", vcxprojAdapter.getPlatformToolset());

					// Advanced Tab
					node.addElementWithTextIfNotEmpty("WholeProgramOptimization", vcxprojAdapter.getWholeProgramOptimization());
					node.addElementWithTextIfNotEmpty("CharacterSet", vcxprojAdapter.getCharacterSet());
					// VCToolsVersion - ex, 14.30.30705, 14.32.31326 (get from directory? env?)
					// PreferredToolArchitecture - x86/x64/arm64 (get from toolchain)
					node.addElementWithTextIfNotEmpty("EnableUnitySupport", vcxprojAdapter.getEnableUnitySupport());
					// CLRSupport - NetCore // ..others
					// UseOfMfc - Dynamic

					// C/C++ Settings
					node.addElementWithTextIfNotEmpty("EnableASAN", vcxprojAdapter.getEnableAddressSanitizer());
				});
			}
			else
			{
				outNode.addElement("PropertyGroup", [&conf](XmlElement& node) {
					node.addAttribute("Condition", conf.condition);
					node.addAttribute("Label", "Configuration");

					auto toolset = fmt::format("v{}", CommandAdapterMSVC::getPlatformToolset(*conf.state));
					node.addElementWithTextIfNotEmpty("PlatformToolset", toolset);
				});
			}
		}
	}
	else if (inType == BuildTargetType::Script || inType == BuildTargetType::Unknown)
	{
		auto& state = *m_states.front();
		auto toolset = fmt::format("v{}", CommandAdapterMSVC::getPlatformToolset(state));
		outNode.addElement("PropertyGroup", [&toolset](XmlElement& node) {
			node.addAttribute("Label", "Configuration");

			// General Tab
			node.addElementWithTextIfNotEmpty("ConfigurationType", "Utility");
			node.addElementWithTextIfNotEmpty("PlatformToolset", toolset);
		});
	}
	// else if (inType == BuildTargetType::Unknown)
	// {
	// }
}

/*****************************************************************************/
void VSVCXProjGen::addMsCppProps(XmlElement& outNode) const
{
	outNode.addElement("Import", [](XmlElement& node) {
		node.addAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.props");
	});
}

/*****************************************************************************/
void VSVCXProjGen::addExtensionSettings(XmlElement& outNode) const
{
	outNode.addElement("ImportGroup", [](XmlElement& node) {
		node.addAttribute("Label", "ExtensionSettings");
		node.setText(std::string());
	});
}

/*****************************************************************************/
void VSVCXProjGen::addShared(XmlElement& outNode) const
{
	outNode.addElement("ImportGroup", [](XmlElement& node) {
		node.addAttribute("Label", "Shared");
		node.setText(std::string());
	});
}

/*****************************************************************************/
void VSVCXProjGen::addPropertySheets(XmlElement& outNode) const
{
	for (auto& conf : m_vsConfigs)
	{
		outNode.addElement("ImportGroup", [&conf](XmlElement& node) {
			node.addAttribute("Label", "PropertySheets");
			node.addAttribute("Condition", conf.condition);
			node.addElement("Import", [](XmlElement& node2) {
				node2.addAttribute("Project", "$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props");
				node2.addAttribute("Condition", "exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')");
				node2.addAttribute("Label", "LocalAppDataPlatform");
			});
		});
	}
}

/*****************************************************************************/
void VSVCXProjGen::addUserMacros(XmlElement& outNode) const
{
	outNode.addElement("PropertyGroup", [](XmlElement& node) {
		node.addAttribute("Label", "UserMacros");
	});
}

/*****************************************************************************/
void VSVCXProjGen::addGeneralProperties(XmlElement& outNode, const std::string& inName, const BuildTargetType inType) const
{
	if (inType == BuildTargetType::Source)
	{
		for (auto& conf : m_vsConfigs)
		{
			if (m_adapters.find(conf.key) != m_adapters.end())
			{
				const auto& vcxprojAdapter = *m_adapters.at(conf.key);

				outNode.addElement("PropertyGroup", [&conf, &vcxprojAdapter](XmlElement& node) {
					node.addAttribute("Condition", conf.condition);
					node.addElementWithText("TargetName", vcxprojAdapter.getTargetName());
					node.addElementWithText("OutDir", vcxprojAdapter.getBuildDir());
					node.addElementWithText("IntDir", vcxprojAdapter.getObjectDir());
					node.addElementWithText("EmbedManifest", vcxprojAdapter.getEmbedManifest());
					node.addElementWithTextIfNotEmpty("LinkIncremental", vcxprojAdapter.getLinkIncremental());

					// Advanced Tab
					// CopyLocalDeploymentContent - true/false
					// CopyLocalProjectReference - true/false
					// CopyLocalDebugSymbols - true/false
					// CopyCppRuntimeToOutputDir - true/false
					// EnableManagedIncrementalBuild - true/false
					// ManagedAssembly - true/false

					// Explicitly add to disable default manifest generation from linker cli
					node.addElement("GenerateManifest");
					node.addElementWithText("DebuggerFlavor", "WindowsLocalDebugger");
					node.addElementWithText("LocalDebuggerWorkingDirectory", vcxprojAdapter.workingDirectory());
					node.addElementWithTextIfNotEmpty("LocalDebuggerEnvironment", vcxprojAdapter.getLocalDebuggerEnvironment());
				});
			}
			else
			{
				const auto& vcxprojAdapter = *m_adapters.begin()->second;
				outNode.addElement("PropertyGroup", [&conf, &vcxprojAdapter](XmlElement& node) {
					node.addAttribute("Condition", conf.condition);
					node.addElementWithText("TargetName", vcxprojAdapter.getTargetName());
					node.addElementWithText("OutDir", vcxprojAdapter.getBuildDir());
					node.addElementWithText("IntDir", vcxprojAdapter.getObjectDir());
				});
			}
		}
	}
	else if (inType == BuildTargetType::Script || inType == BuildTargetType::Unknown)
	{
		for (auto& conf : m_vsConfigs)
		{
			outNode.addElement("PropertyGroup", [&inName, &conf](XmlElement& node) {
				node.addAttribute("Condition", conf.condition);
				node.addElementWithText("TargetName", inName);

				auto buildOutputDir = Files::getCanonicalPath(conf.state->paths.buildOutputDir());
				auto logDir = fmt::format("{}/logs/{}/", buildOutputDir, inName);
				node.addElementWithText("OutDir", logDir);
				node.addElementWithText("IntDir", logDir);
			});
		}
	}
}

/*****************************************************************************/
void VSVCXProjGen::addCompileProperties(XmlElement& outNode) const
{
	for (auto& conf : m_vsConfigs)
	{
		if (m_adapters.find(conf.key) != m_adapters.end())
		{
			const auto& vcxprojAdapter = *m_adapters.at(conf.key);

			outNode.addElement("ItemDefinitionGroup", [&conf, &vcxprojAdapter](XmlElement& node) {
				node.addAttribute("Condition", conf.condition);
				node.addElement("ClCompile", [&vcxprojAdapter](XmlElement& node2) {
					node2.addElementWithTextIfNotEmpty("ConformanceMode", vcxprojAdapter.getConformanceMode());
					node2.addElementWithTextIfNotEmpty("LanguageStandard", vcxprojAdapter.getLanguageStandardCpp());
					node2.addElementWithTextIfNotEmpty("LanguageStandard_C", vcxprojAdapter.getLanguageStandardC());
					node2.addElementWithTextIfNotEmpty("MultiProcessorCompilation", vcxprojAdapter.getMultiProcessorCompilation()); // /MP

					// C/C++ Settings
					node2.addElementWithTextIfNotEmpty("AdditionalIncludeDirectories", vcxprojAdapter.getAdditionalIncludeDirectories());

					if (vcxprojAdapter.usesPrecompiledHeader())
					{
						node2.addElementWithTextIfNotEmpty("PrecompiledHeader", "Use");
						node2.addElementWithTextIfNotEmpty("PrecompiledHeaderFile", vcxprojAdapter.getPrecompiledHeaderMinusLocation());
						node2.addElementWithTextIfNotEmpty("PrecompiledHeaderOutputFile", vcxprojAdapter.getPrecompiledHeaderOutputFile());
						node2.addElementWithTextIfNotEmpty("ForcedIncludeFiles", vcxprojAdapter.getPrecompiledHeaderMinusLocation());
					}

					if (vcxprojAdapter.usesModules())
					{
						node2.addElementWithTextIfNotEmpty("EnableModules", "true");
						node2.addElementWithTextIfNotEmpty("CompileAs", "CompileAsCppModule");
					}

					node2.addElementWithTextIfNotEmpty("SDLCheck", vcxprojAdapter.getSDLCheck());
					node2.addElementWithTextIfNotEmpty("WarningLevel", vcxprojAdapter.getWarningLevel());
					node2.addElementWithTextIfNotEmpty("ExternalWarningLevel", vcxprojAdapter.getExternalWarningLevel());
					node2.addElementWithText("PreprocessorDefinitions", vcxprojAdapter.getPreprocessorDefinitions());
					node2.addElementWithTextIfNotEmpty("FunctionLevelLinking", vcxprojAdapter.getFunctionLevelLinking());
					node2.addElementWithTextIfNotEmpty("IntrinsicFunctions", vcxprojAdapter.getIntrinsicFunctions());
					node2.addElementWithTextIfNotEmpty("TreatWarningsAsError", vcxprojAdapter.getTreatWarningsAsError());
					node2.addElementWithTextIfNotEmpty("DiagnosticsFormat", vcxprojAdapter.getDiagnosticsFormat());
					node2.addElementWithTextIfNotEmpty("DebugInformationFormat", vcxprojAdapter.getDebugInformationFormat());
					node2.addElementWithTextIfNotEmpty("SupportJustMyCode", vcxprojAdapter.getSupportJustMyCode());
					node2.addElementWithTextIfNotEmpty("Optimization", vcxprojAdapter.getOptimization());
					node2.addElementWithTextIfNotEmpty("InlineFunctionExpansion", vcxprojAdapter.getInlineFunctionExpansion());
					node2.addElementWithTextIfNotEmpty("FavorSizeOrSpeed", vcxprojAdapter.getFavorSizeOrSpeed());
					// OmitFramePointers - true (Oy) / false (Oy-)
					node2.addElementWithTextIfNotEmpty("WholeProgramOptimization", vcxprojAdapter.getWholeProgramOptimizationCompileFlag());
					// EnableFiberSafeOptimizations - true/false (/GT)
					node2.addElementWithTextIfNotEmpty("BufferSecurityCheck", vcxprojAdapter.getBufferSecurityCheck());
					node2.addElementWithTextIfNotEmpty("FloatingPointModel", vcxprojAdapter.getFloatingPointModel());
					node2.addElementWithTextIfNotEmpty("BasicRuntimeChecks", vcxprojAdapter.getBasicRuntimeChecks());
					node2.addElementWithTextIfNotEmpty("RuntimeLibrary", vcxprojAdapter.getRuntimeLibrary());
					node2.addElementWithTextIfNotEmpty("ExceptionHandling", vcxprojAdapter.getExceptionHandling());
					node2.addElementWithTextIfNotEmpty("RuntimeTypeInfo", vcxprojAdapter.getRunTimeTypeInfo());
					node2.addElementWithTextIfNotEmpty("TreatWChar_tAsBuiltInType", vcxprojAdapter.getTreatWChartAsBuiltInType());
					node2.addElementWithTextIfNotEmpty("ForceConformanceInForLoopScope", vcxprojAdapter.getForceConformanceInForLoopScope());
					node2.addElementWithTextIfNotEmpty("RemoveUnreferencedCodeData", vcxprojAdapter.getRemoveUnreferencedCodeData());
					node2.addElementWithTextIfNotEmpty("CallingConvention", vcxprojAdapter.getCallingConvention());
					node2.addElementWithTextIfNotEmpty("ProgramDataBaseFileName", vcxprojAdapter.getProgramDataBaseFileName());
					node2.addElementWithTextIfNotEmpty("AssemblerOutput", vcxprojAdapter.getAssemblerOutput());
					node2.addElementWithTextIfNotEmpty("AssemblerListingLocation", vcxprojAdapter.getAssemblerListingLocation());

					node2.addElementWithText("AdditionalOptions", vcxprojAdapter.getAdditionalCompilerOptions());
				});

				if (vcxprojAdapter.usesLibManager())
				{
					node.addElement("Lib", [&vcxprojAdapter](XmlElement& node2) {
						node2.addElementWithTextIfNotEmpty("LinkTimeCodeGeneration", vcxprojAdapter.getLinkTimeCodeGeneration());
						node2.addElementWithTextIfNotEmpty("TargetMachine", vcxprojAdapter.getTargetMachine());
						node2.addElementWithTextIfNotEmpty("TreatLibWarningAsErrors", vcxprojAdapter.getTreatWarningsAsError());
					});
				}
				else
				{
					node.addElement("Link", [&vcxprojAdapter](XmlElement& node2) {
						node2.addElementWithText("GenerateDebugInformation", vcxprojAdapter.getGenerateDebugInformation());
						node2.addElementWithText("AdditionalLibraryDirectories", vcxprojAdapter.getAdditionalLibraryDirectories());
						node2.addElementWithText("TreatLinkerWarningAsErrors", vcxprojAdapter.getTreatLinkerWarningAsErrors());
						node2.addElementWithText("RandomizedBaseAddress", vcxprojAdapter.getRandomizedBaseAddress());
						node2.addElementWithText("DataExecutionPrevention", vcxprojAdapter.getDataExecutionPrevention());

						// Explicitly add these to disable default manifest generation from linker cli
						node2.addElement("ManifestFile");
						node2.addElement("AllowIsolation");
						node2.addElement("EnableUAC");
						node2.addElement("UACExecutionLevel");
						node2.addElement("UACUIAccess");

						node2.addElementWithTextIfNotEmpty("OptimizeReferences", vcxprojAdapter.getOptimizeReferences());
						node2.addElementWithTextIfNotEmpty("EnableCOMDATFolding", vcxprojAdapter.getEnableCOMDATFolding());
						node2.addElementWithTextIfNotEmpty("SubSystem", vcxprojAdapter.getSubSystem());
						node2.addElementWithTextIfNotEmpty("IncrementalLinkDatabaseFile", vcxprojAdapter.getIncrementalLinkDatabaseFile());
						node2.addElementWithTextIfNotEmpty("FixedBaseAddress", vcxprojAdapter.getFixedBaseAddress());
						node2.addElementWithTextIfNotEmpty("ImportLibrary", vcxprojAdapter.getImportLibrary());
						node2.addElementWithTextIfNotEmpty("ProgramDatabaseFile", vcxprojAdapter.getProgramDatabaseFile());
						node2.addElementWithTextIfNotEmpty("StripPrivateSymbols", vcxprojAdapter.getStripPrivateSymbols());
						node2.addElementWithTextIfNotEmpty("LinkTimeCodeGeneration", vcxprojAdapter.getLinkerLinkTimeCodeGeneration());
						node2.addElementWithTextIfNotEmpty("LinkTimeCodeGenerationObjectFile", vcxprojAdapter.getLinkTimeCodeGenerationObjectFile());
						node2.addElementWithTextIfNotEmpty("EntryPointSymbol", vcxprojAdapter.getEntryPointSymbol());
						node2.addElementWithTextIfNotEmpty("TargetMachine", vcxprojAdapter.getTargetMachine());
						node2.addElementWithTextIfNotEmpty("Profile", vcxprojAdapter.getProfile());
						node2.addElementWithTextIfNotEmpty("AdditionalOptions", vcxprojAdapter.getAdditionalLinkerOptions());
						node2.addElementWithTextIfNotEmpty("AdditionalDependencies", vcxprojAdapter.getAdditionalDependencies());
					});
				}

				node.addElement("ResourceCompile", [&vcxprojAdapter](XmlElement& node2) {
					node2.addElementWithText("PreprocessorDefinitions", vcxprojAdapter.getPreprocessorDefinitions());
					node2.addElementWithTextIfNotEmpty("AdditionalIncludeDirectories", vcxprojAdapter.getAdditionalIncludeDirectories(true));
				});
			});
		}
		else
		{
			outNode.addElement("ItemDefinitionGroup", [&conf](XmlElement& node) {
				node.addAttribute("Condition", conf.condition);
			});
		}
	}
}

/*****************************************************************************/
void VSVCXProjGen::addScriptProperties(XmlElement& outNode) const
{
	for (auto& conf : m_vsConfigs)
	{
		if (m_targetAdapters.find(conf.key) != m_targetAdapters.end())
		{
			const auto& config = conf.state->configuration.name();
			auto arch = Arch::toVSArch(conf.state->info.targetArchitecture());
			auto outPath = fmt::format("{}/scripts/{}-{}_{}.bat", m_exportPath, m_currentTarget, arch, config);

			const auto& targetAdapter = *m_targetAdapters.at(conf.key);
			auto command = targetAdapter.getCommand();

			std::string outputChecks;
			// Revisit
			/*auto files = targetAdapter.getOutputFiles();
			for (auto& file : files)
			{
				outputChecks += fmt::format("if exist \"{}\" ( exit /b 0 )\n", file);
			}*/

			auto outCommand = fmt::format(R"batch(if "%BUILD_FROM_CHALET%"=="1" echo *== script start ==*
{outputChecks}{command}if "%BUILD_FROM_CHALET%"=="1" echo *== script end ==*)batch",
				FMT_ARG(outputChecks),
				FMT_ARG(command));

			Files::createFileWithContents(outPath, outCommand);

			outNode.addElement("ItemDefinitionGroup", [&conf, &outPath](XmlElement& node) {
				node.addAttribute("Condition", conf.condition);

				node.addElement("PreBuildEvent", [&outPath](XmlElement& node2) {
					node2.addElementWithText("Command", fmt::format("call \"{}\"", outPath));
				});
			});
		}
		else
		{
			outNode.addElement("ItemDefinitionGroup", [&conf](XmlElement& node) {
				node.addAttribute("Condition", conf.condition);
			});
		}
	}
}

/*****************************************************************************/
void VSVCXProjGen::addSourceFiles(XmlElement& outNode, const std::string& inName, XmlFile& outFiltersFile) const
{
	OrderedDictionary<bool> headers;
	OrderedDictionary<StringList> sources;
	OrderedDictionary<StringList> resources;
	OrderedDictionary<StringList> pchSource;
	std::pair<std::string, StringList> manifest;
	std::pair<std::string, StringList> icon;
	std::string pchFile;
	std::vector<BuildState*> tempStates;

	for (auto& conf : m_vsConfigs)
	{
		auto projectPtr = getProjectFromStateContext(*conf.state, inName);
		if (projectPtr != nullptr)
		{
			const auto& project = *projectPtr;
			if (m_adapters.find(conf.key) != m_adapters.end())
			{
				const auto& vcxprojAdapter = *m_adapters.at(conf.key);

				tempStates.emplace_back(conf.state);

				auto headerFiles = project.getHeaderFiles();
				const auto& pch = project.precompiledHeader();
				if (!pch.empty())
				{
					headerFiles.push_back(pch);
					if (pchFile.empty())
					{
						pchFile = vcxprojAdapter.getPrecompiledHeaderFile();
					}

					const auto& pchSourceFile = vcxprojAdapter.getPrecompiledHeaderSourceFile();
					if (pchSource.find(pchSourceFile) == pchSource.end())
						pchSource[pchSourceFile] = StringList{ conf.key };
					else
						pchSource[pchSourceFile].emplace_back(conf.key);
				}

				bool isModulesTarget = project.cppModules();
				for (auto& header : headerFiles)
				{
					if (isModulesTarget && String::endsWith(".ixx", header))
						continue;

					auto file = Files::getCanonicalPath(header);
					if (Files::pathExists(file))
						headers[file] = true;
					else
						headers[header] = true;
				}

				SourceOutputs& outputs = *m_outputs.at(conf.key).get();

				for (auto& group : outputs.groups)
				{
					auto file = Files::getCanonicalPath(group->sourceFile);
					if (!Files::pathExists(file))
						file = group->sourceFile;

					switch (group->type)
					{
						case SourceType::C:
						case SourceType::CPlusPlus: {
							if (sources.find(file) == sources.end())
								sources[file] = StringList{ conf.key };
							else
								sources[file].emplace_back(conf.key);

							break;
						}
						case SourceType::WindowsResource: {
							if (resources.find(file) == resources.end())
								resources[file] = StringList{ conf.key };
							else
								resources[file].emplace_back(conf.key);

							break;
						}
						default:
							break;
					}
				}

				manifest.first = conf.state->paths.getWindowsManifestFilename(project);
				if (!manifest.first.empty())
				{
					auto file = Files::getCanonicalPath(manifest.first);
					if (Files::pathExists(file))
						manifest.first = file;

					manifest.second.emplace_back(conf.key);
				}

				icon.first = project.windowsApplicationIcon();
				if (!icon.first.empty())
				{
					auto file = Files::getCanonicalPath(icon.first);
					if (Files::pathExists(file))
						icon.first = file;

					icon.second.emplace_back(conf.key);
				}

				/*const auto& projectFiles = project.files();
					for (auto& file : projectFiles)
					{
						if (files.find(file) == files.end())
							files[file] = StringList{ config };
						else
							files[file].emplace_back(config);
					}*/
			}
		}
	}

	auto& filters = outFiltersFile.getRoot();

	if (!headers.empty())
	{
		outNode.addElement("ItemGroup", [&headers](XmlElement& node) {
			for (auto& it : headers)
			{
				const auto& file = it.first;

				node.addElement("ClInclude", [&file](XmlElement& node2) {
					node2.addAttribute("Include", file);
				});
			}
		});
		filters.addElement("ItemGroup", [&headers, &pchFile](XmlElement& node) {
			for (auto& it : headers)
			{
				const auto& file = it.first;

				if (String::equals(pchFile, file))
					continue;

				node.addElement("ClInclude", [&file](XmlElement& node2) {
					node2.addAttribute("Include", file);
					node2.addElementWithText("Filter", "Header Files");
				});
			}
		});
	}
	if (!pchFile.empty())
	{
		filters.addElement("ItemGroup", [&pchFile](XmlElement& node) {
			node.addElement("ClInclude", [&pchFile](XmlElement& node2) {
				node2.addAttribute("Include", pchFile);
				node2.addElementWithText("Filter", "Precompile Header Files");
			});
		});
	}
	if (!pchSource.empty())
	{
		filters.addElement("ItemGroup", [&pchSource](XmlElement& node) {
			for (auto& it : pchSource)
			{
				const auto& file = it.first;

				node.addElement("ClCompile", [&file](XmlElement& node2) {
					node2.addAttribute("Include", file);
					node2.addElementWithText("Filter", "Precompile Header Files");
				});
			}
		});
	}

	if (!pchSource.empty() || !sources.empty())
	{
		outNode.addElement("ItemGroup", [this, &tempStates, &sources, &pchSource](XmlElement& node) {
			for (auto& it : pchSource)
			{
				const auto& file = it.first;
				const auto& keys = it.second;

				node.addElement("ClCompile", [this, &file, &tempStates, &keys](XmlElement& node2) {
					node2.addAttribute("Include", file);
					node2.addElementWithText("PrecompiledHeader", "Create");
					node2.addElementWithText("ForcedIncludeFiles", std::string());
					node2.addElementWithText("ObjectFileName", "$(IntDir)");

					for (auto& state : tempStates)
					{
						auto key = getDictionaryKey(*state);
						if (!List::contains(keys, key))
						{
							auto condition = getCondition(*state);
							node2.addElement("ExcludedFromBuild", [&condition](XmlElement& node3) {
								node3.addAttribute("Condition", condition);
								node3.setText("true");
							});
						}
					}
				});
			}

			for (auto& it : sources)
			{
				const auto& file = it.first;
				const auto& keys = it.second;

				node.addElement("ClCompile", [this, &file, &tempStates, &keys](XmlElement& node2) {
					node2.addAttribute("Include", file);

					for (auto& state : tempStates)
					{
						auto key = getDictionaryKey(*state);
						if (!List::contains(keys, key))
						{
							auto condition = getCondition(*state);
							node2.addElement("ExcludedFromBuild", [&condition](XmlElement& node3) {
								node3.addAttribute("Condition", condition);
								node3.setText("true");
							});
						}
					}
				});
			}
		});
		filters.addElement("ItemGroup", [&sources](XmlElement& node) {
			for (auto& it : sources)
			{
				const auto& file = it.first;

				node.addElement("ClCompile", [&file](XmlElement& node2) {
					node2.addAttribute("Include", file);
					node2.addElementWithText("Filter", "Source Files");
				});
			}
		});
	}

	if (!resources.empty())
	{
		outNode.addElement("ItemGroup", [this, &tempStates, &resources](XmlElement& node) {
			for (auto& it : resources)
			{
				const auto& file = it.first;
				const auto& keys = it.second;

				node.addElement("ResourceCompile", [this, &file, &tempStates, &keys](XmlElement& node2) {
					node2.addAttribute("Include", file);
					node2.addElementWithText("PrecompiledHeader", "NotUsing");

					for (auto& state : tempStates)
					{
						auto key = getDictionaryKey(*state);
						if (!List::contains(keys, key))
						{
							auto condition = getCondition(*state);
							node2.addElement("ExcludedFromBuild", [&condition](XmlElement& node3) {
								node3.addAttribute("Condition", condition);
								node3.setText("true");
							});
						}
					}
				});
			}
		});
		filters.addElement("ItemGroup", [&resources](XmlElement& node) {
			for (auto& it : resources)
			{
				const auto& file = it.first;

				node.addElement("ResourceCompile", [&file](XmlElement& node2) {
					node2.addAttribute("Include", file);
					node2.addElementWithText("Filter", "Resource Files");
				});
			}
		});
	}

	if (!manifest.first.empty())
	{
		outNode.addElement("ItemGroup", [this, &manifest, &tempStates](XmlElement& node) {
			node.addElement("Manifest", [this, &manifest, &tempStates](XmlElement& node2) {
				node2.addAttribute("Include", manifest.first);

				for (auto& state : tempStates)
				{
					auto key = getDictionaryKey(*state);
					if (!List::contains(manifest.second, key))
					{
						auto condition = getCondition(*state);
						node2.addElement("ExcludedFromBuild", [&condition](XmlElement& node3) {
							node3.addAttribute("Condition", condition);
							node3.setText("true");
						});
					}
				}
			});
		});
		filters.addElement("ItemGroup", [&manifest](XmlElement& node) {
			node.addElement("Manifest", [&manifest](XmlElement& node2) {
				node2.addAttribute("Include", manifest.first);
				node2.addElementWithText("Filter", "Resource Files");
			});
		});
	}

	if (!icon.first.empty())
	{
		outNode.addElement("ItemGroup", [this, &icon, &tempStates](XmlElement& node) {
			node.addElement("Image", [this, &icon, &tempStates](XmlElement& node2) {
				node2.addAttribute("Include", icon.first);

				for (auto& state : tempStates)
				{
					auto key = getDictionaryKey(*state);
					if (!List::contains(icon.second, key))
					{
						auto condition = getCondition(*state);
						node2.addElement("ExcludedFromBuild", [&condition](XmlElement& node3) {
							node3.addAttribute("Condition", condition);
							node3.setText("true");
						});
					}
				}
			});
		});
		filters.addElement("ItemGroup", [&icon](XmlElement& node) {
			node.addElement("Image", [&icon](XmlElement& node2) {
				node2.addAttribute("Include", icon.first);
				node2.addElementWithText("Filter", "Resource Files");
			});
		});
	}
}

/*****************************************************************************/
void VSVCXProjGen::addTargetFiles(XmlElement& outNode, const std::string& inName, XmlFile& outFiltersFile) const
{
	UNUSED(inName);

	OrderedDictionary<StringList> sources;
	std::vector<BuildState*> tempStates;

	for (auto& conf : m_vsConfigs)
	{
		if (m_targetAdapters.find(conf.key) != m_targetAdapters.end())
		{
			const auto& targetAdapter = *m_targetAdapters.at(conf.key);

			tempStates.emplace_back(conf.state);

			auto targetFiles = targetAdapter.getFiles();

			for (auto& file : targetFiles)
			{
				if (sources.find(file) == sources.end())
					sources[file] = StringList{ conf.key };
				else
					sources[file].emplace_back(conf.key);
			}
		}
	}

	// auto& filters = outFiltersFile.getRoot();
	UNUSED(outFiltersFile);
	outNode.addElement("ItemGroup", [&sources](XmlElement& node) {
		for (auto& it : sources)
		{
			const auto& file = it.first;
			// const auto& configs = it.second;

			node.addElement("None", [&file](XmlElement& node2) {
				node2.addAttribute("Include", file);
			});
		}
	});
	/*filters.addElement("ItemGroup", [&sources](XmlElement& node) {
		for (auto& it : sources)
		{
			const auto& file = it.first;

			node.addElement("None", [&file](XmlElement& node2) {
				node2.addAttribute("Include", file);
				node2.addElementWithText("Filter", "Files");
			});
		}
	});*/
}

/*****************************************************************************/
void VSVCXProjGen::addAllTargetFiles(XmlElement& outNode) const
{
	StringList files;

	files.emplace_back(getResolvedInputFile());

	outNode.addElement("ItemGroup", [&files](XmlElement& node) {
		for (auto& file : files)
		{
			node.addElement("None", [&file](XmlElement& node2) {
				node2.addAttribute("Include", file);
			});
		}
	});
}

/*****************************************************************************/
void VSVCXProjGen::addProjectReferences(XmlElement& outNode, const std::string& inName) const
{
	for (auto& conf : m_vsConfigs)
	{
		StringList dependsList;
		conf.state->getTargetDependencies(dependsList, inName, false);

		if (!dependsList.empty())
		{
			outNode.addElement("ItemGroup", [this, &dependsList, &conf](XmlElement& node) {
				node.addAttribute("Condition", conf.condition);

				for (auto& tgt : dependsList)
				{
					if (m_targetGuids.find(tgt) == m_targetGuids.end())
						continue;

					node.addElement("ProjectReference", [this, &tgt](XmlElement& node2) {
						auto uuid = String::toUpperCase(m_targetGuids.at(tgt).str());
						node2.addAttribute("Include", fmt::format("{}.vcxproj", tgt));
						node2.addElementWithText("Project", fmt::format("{{{}}}", uuid));
						node2.addElementWithText("Name", tgt);
					});
				}
			});
		}
	}
}

/*****************************************************************************/
void VSVCXProjGen::addAllProjectReferences(XmlElement& outNode) const
{
	for (auto& conf : m_vsConfigs)
	{
		StringList list;
		for (auto& target : conf.state->targets)
		{
			list.emplace_back(target->name());
		}
		if (!list.empty())
		{
			outNode.addElement("ItemGroup", [this, &list, &conf](XmlElement& node) {
				node.addAttribute("Condition", conf.condition);

				for (auto& tgt : list)
				{
					if (m_targetGuids.find(tgt) == m_targetGuids.end())
						continue;

					node.addElement("ProjectReference", [this, &tgt](XmlElement& node2) {
						auto uuid = String::toUpperCase(m_targetGuids.at(tgt).str());
						node2.addAttribute("Include", fmt::format("{}.vcxproj", tgt));
						node2.addElementWithText("Project", fmt::format("{{{}}}", uuid));
						node2.addElementWithText("Name", tgt);
					});
				}
			});
		}
	}
}

/*****************************************************************************/
void VSVCXProjGen::addImportMsCppTargets(XmlElement& outNode) const
{
	outNode.addElement("Import", [](XmlElement& node) {
		node.addAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.targets");
	});
}

/*****************************************************************************/
void VSVCXProjGen::addExtensionTargets(XmlElement& outNode) const
{
	outNode.addElement("ImportGroup", [](XmlElement& node) {
		node.addAttribute("Label", "ExtensionTargets");
	});
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
// Utils
/*****************************************************************************/
const SourceTarget* VSVCXProjGen::getProjectFromStateContext(const BuildState& inState, const std::string& inName) const
{
	const SourceTarget* ret = nullptr;
	for (auto& target : inState.targets)
	{
		if (target->isSources() && String::equals(target->name(), inName))
		{
			ret = static_cast<const SourceTarget*>(target.get());
		}
	}

	return ret;
}

/*****************************************************************************/
const IBuildTarget* VSVCXProjGen::getTargetFromStateContext(const BuildState& inState, const std::string& inName) const
{
	const IBuildTarget* ret = nullptr;
	for (auto& target : inState.targets)
	{
		if (String::equals(target->name(), inName))
		{
			ret = target.get();
		}
	}

	return ret;
}

/*****************************************************************************/
std::string VSVCXProjGen::getWindowsTargetPlatformVersion() const
{
	auto ret = Environment::getString("UCRTVersion");
	if (ret.empty())
		ret = "10.0";

	return ret;
}

/*****************************************************************************/
std::string VSVCXProjGen::getVisualStudioVersion() const
{
	auto& state = *m_states.front();
	std::string ret = state.environment->detectedVersion();

	auto decimal = ret.find('.');
	if (decimal != std::string::npos)
	{
		decimal = ret.find('.', decimal + 1);
		if (decimal != std::string::npos)
			ret = ret.substr(0, decimal);
		else
			ret.clear();
	}
	else
		ret.clear();

	return ret;
}

/*****************************************************************************/
std::string VSVCXProjGen::getCondition(const BuildState& inState) const
{
	const auto& config = inState.configuration.name();
	auto arch = Arch::toVSArch2(inState.info.targetArchitecture());
	return fmt::format("'$(Configuration)|$(Platform)'=='{}|{}'", config, arch);
}

/*****************************************************************************/
std::string VSVCXProjGen::getDictionaryKey(const BuildState& inState) const
{
	const auto& config = inState.configuration.name();
	const auto& arch = inState.info.targetArchitectureString();
	return fmt::format("{}|{}", config, arch);
}

/*****************************************************************************/
std::string VSVCXProjGen::getResolvedInputFile() const
{
	auto& firstState = *m_states.front();
	auto inputFile = Files::getCanonicalPath(firstState.inputs.inputFile());
	if (!Files::pathExists(inputFile))
		inputFile = firstState.inputs.inputFile();

	return inputFile;
}
}
