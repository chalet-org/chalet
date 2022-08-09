/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
 */

#include "Export/VisualStudio/VSVCXProjGen.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Export/VisualStudio/ProjectAdapterVCXProj.hpp"
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

	if (!saveFiltersFile(inState, inProject, fmt::format("{}.filters", projectFile)))
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
		const auto project = getProjectFromStateContext(*state, inProject.name());
		const auto& config = state->configuration;

		ProjectAdapterVCXProj vcxprojAdapter(*state, *project);

		xmlRoot.addElement("PropertyGroup", [&config, &vcxprojAdapter](XmlElement& node) {
			node.addAttribute("Condition", fmt::format("'$(Configuration)|$(Platform)'=='{}|x64'", config.name()));
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
			// EnableUnitySupport - true/false // Unity build
			// CLRSupport - NetCore // ..others
			// UseOfMfc - Dynamic

			// C/C++ Settings
			node.addElementWithTextIfNotEmpty("EnableASAN", vcxprojAdapter.getEnableAddressSanitizer());
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
		const auto project = getProjectFromStateContext(*state, inProject.name());
		const auto& config = state->configuration;

		ProjectAdapterVCXProj vcxprojAdapter(*state, *project);

		xmlRoot.addElement("PropertyGroup", [&vcxprojAdapter, &config](XmlElement& node) {
			node.addAttribute("Condition", fmt::format("'$(Configuration)|$(Platform)'=='{}|x64'", config.name()));

			if (config.debugSymbols() && !config.enableSanitizers() && !config.enableProfiling())
			{
				node.addElementWithText("LinkIncremental", vcxprojAdapter.getBoolean(true));
			}

			// Advanced Tab
			// CopyLocalDeploymentContent - true/false
			// CopyLocalProjectReference - true/false
			// CopyLocalDebugSymbols - true/false
			// CopyCppRuntimeToOutputDir - true/false
			// EnableManagedIncrementalBuild - true/false
			// ManagedAssembly - true/false
		});
	}

	for (auto& state : m_states)
	{
		const auto project = getProjectFromStateContext(*state, inProject.name());
		const auto& config = state->configuration;

		ProjectAdapterVCXProj vcxprojAdapter(*state, *project);

		xmlRoot.addElement("ItemDefinitionGroup", [&config, &vcxprojAdapter](XmlElement& node) {
			node.addAttribute("Condition", fmt::format("'$(Configuration)|$(Platform)'=='{}|x64'", config.name()));
			node.addElement("ClCompile", [&vcxprojAdapter](XmlElement& node2) {
				node2.addElementWithTextIfNotEmpty("WarningLevel", vcxprojAdapter.getWarningLevel());
				node2.addElementWithTextIfNotEmpty("FunctionLevelLinking", vcxprojAdapter.getFunctionLevelLinking());
				node2.addElementWithTextIfNotEmpty("IntrinsicFunctions", vcxprojAdapter.getIntrinsicFunctions());

				node2.addElementWithTextIfNotEmpty("SDLCheck", vcxprojAdapter.getSDLCheck());
				node2.addElementWithText("PreprocessorDefinitions", fmt::format("{}%(PreprocessorDefinitions)", vcxprojAdapter.getPreprocessorDefinitions()));

				node2.addElementWithTextIfNotEmpty("ConformanceMode", vcxprojAdapter.getConformanceMode());
				node2.addElementWithTextIfNotEmpty("LanguageStandard", vcxprojAdapter.getLanguageStandardCpp());
				node2.addElementWithTextIfNotEmpty("LanguageStandard_C", vcxprojAdapter.getLanguageStandardCpp());

				// C/C++ Settings
				node2.addElementWithTextIfNotEmpty("TreatWarningsAsError", vcxprojAdapter.getTreatWarningsAsError());
				node2.addElementWithTextIfNotEmpty("DiagnosticsFormat", vcxprojAdapter.getDiagnosticsFormat());
				node2.addElementWithTextIfNotEmpty("DebugInformationFormat", vcxprojAdapter.getDebugInformationFormat());
				node2.addElementWithTextIfNotEmpty("SupportJustMyCode", vcxprojAdapter.getSupportJustMyCode());
				// MultiProcessorCompilation - true/false - /MP (Not used currentlY)
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

				node2.addElementWithText("AdditionalOptions", fmt::format("{}%(AdditionalOptions)", vcxprojAdapter.getAdditionalOptions()));
			});

			node.addElement("Link", [&config, &vcxprojAdapter](XmlElement& node2) {
				auto trueStr = vcxprojAdapter.getBoolean(true);
				node2.addElementWithTextIfNotEmpty("SubSystem", vcxprojAdapter.getSubSystem());
				if (!config.debugSymbols())
				{
					node2.addElementWithText("EnableCOMDATFolding", trueStr);
					node2.addElementWithText("OptimizeReferences", trueStr);
				}
				node2.addElementWithText("GenerateDebugInformation", trueStr);
			});
		});
	}

	auto headerFiles = inProject.getHeaderFiles();
	const auto& pch = inProject.precompiledHeader();
	if (!pch.empty())
		headerFiles.push_back(pch);

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
	/*xmlRoot.addElement("ItemGroup", [](XmlElement& node) {
		node.addElement("ResourceCompile", [](XmlElement& node2) {
			node2.addAttribute("Include", file);
		});
	});*/
	// Ico files
	/*xmlRoot.addElement("ItemGroup", [](XmlElement& node) {
		node.addElement("Image", [](XmlElement& node2) {
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
bool VSVCXProjGen::saveFiltersFile(const BuildState& inState, const SourceTarget& inProject, const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	ProjectAdapterVCXProj vcxprojAdapter(inState, inProject);

	auto& xml = xmlFile.xml;
	auto& xmlRoot = xml.root();
	xmlRoot.setName("Project");
	xmlRoot.addAttribute("ToolsVersion", "4.0");
	xmlRoot.addAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
	xmlRoot.addElement("ItemGroup", [this, &vcxprojAdapter](XmlElement& node) {
		node.addElement("Filter", [this, &vcxprojAdapter](XmlElement& node2) {
			auto extensions = String::join(vcxprojAdapter.getSourceExtensions(), ';');
			auto guid = Uuid::v5(extensions, m_projectTypeGuid).toUpperCase();
			node2.addAttribute("Include", "Source Files");
			node2.addElementWithText("UniqueIdentifier", fmt::format("{{{}}}", guid));
			node2.addElementWithText("Extensions", extensions);
		});
		node.addElement("Filter", [this, &vcxprojAdapter](XmlElement& node2) {
			auto extensions = String::join(vcxprojAdapter.getHeaderExtensions(), ';');
			auto guid = Uuid::v5(extensions, m_projectTypeGuid).toUpperCase();
			node2.addAttribute("Include", "Header Files");
			node2.addElementWithText("UniqueIdentifier", fmt::format("{{{}}}", guid));
			node2.addElementWithText("Extensions", extensions);
		});
		node.addElement("Filter", [this, &vcxprojAdapter](XmlElement& node2) {
			auto extensions = String::join(vcxprojAdapter.getResourceExtensions(), ';');
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

	chalet_assert(ret != nullptr, "project name not found");
	return ret;
}

}
