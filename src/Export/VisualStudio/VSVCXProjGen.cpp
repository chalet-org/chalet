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
#include "Terminal/Environment.hpp"
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
VSVCXProjGen::~VSVCXProjGen() = default;

/*****************************************************************************/
bool VSVCXProjGen::saveProjectFiles(const BuildState& inState, const SourceTarget& inProject)
{
	const auto& name = inProject.name();
	if (m_targetGuids.find(name) == m_targetGuids.end())
		return false;

	m_currentTarget = name;
	m_currentGuid = m_targetGuids.at(name).str();

	if (m_adapters.empty())
	{
		for (auto& state : m_states)
		{
			const auto project = getProjectFromStateContext(*state, name);
			const auto& config = state->configuration.name();

			StringList fileCache;
			m_outputs.emplace(config, state->paths.getOutputs(inProject, fileCache));

			auto [it, _] = m_adapters.emplace(config, std::make_unique<ProjectAdapterVCXProj>(*state, *project, m_cwd));
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

	auto projectFile = fmt::format("{name}.vcxproj", FMT_ARG(name));

	if (!saveProjectFile(inState, name, projectFile))
		return false;

	if (!saveFiltersFile(inState, fmt::format("{}.filters", projectFile)))
		return false;

	if (!saveUserFile(fmt::format("{}.user", projectFile)))
		return false;

	return true;
}

/*****************************************************************************/
bool VSVCXProjGen::saveProjectFile(const BuildState& inState, const std::string& inName, const std::string& inFilename)
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
		node.addElementWithText("ProjectGuid", fmt::format("{{{}}}", m_currentGuid));
		node.addElementWithText("WindowsTargetPlatformVersion", getWindowsTargetPlatformVersion());
		node.addElementWithText("Keyword", "Win32Proj");
		node.addElementWithText("VCProjectVersion", visualStudioVersion);
		node.addElementWithText("RootNamespace", m_currentTarget);
		node.addElementWithText("ProjectName", m_currentTarget);
		node.addElementWithText("VCProjectUpgraderObjectName", "NoUpgrade");
	});

	xmlRoot.addElement("Import", [](XmlElement& node) {
		node.addAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props");
	});

	for (auto& state : m_states)
	{
		const auto& config = state->configuration.name();
		const auto& vcxprojAdapter = *m_adapters.at(config);
		auto condition = getCondition(config);

		xmlRoot.addElement("PropertyGroup", [&condition, &vcxprojAdapter](XmlElement& node) {
			node.addAttribute("Condition", condition);
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
		auto condition = getCondition(name);
		xmlRoot.addElement("ImportGroup", [&condition](XmlElement& node) {
			node.addAttribute("Label", "PropertySheets");
			node.addAttribute("Condition", condition);
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
		const auto& config = state->configuration.name();
		const auto& vcxprojAdapter = *m_adapters.at(config);
		auto condition = getCondition(config);

		xmlRoot.addElement("PropertyGroup", [&condition, &vcxprojAdapter](XmlElement& node) {
			node.addAttribute("Condition", condition);
			node.addElementWithTextIfNotEmpty("LinkIncremental", vcxprojAdapter.getLinkIncremental());
			node.addElementWithText("OutDir", vcxprojAdapter.getBuildDir());
			node.addElementWithText("IntDir", vcxprojAdapter.getObjectDir());
			node.addElementWithText("EmbedManifest", vcxprojAdapter.getEmbedManifest());

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
		const auto& config = state->configuration.name();
		const auto& vcxprojAdapter = *m_adapters.at(config);
		auto condition = getCondition(config);

		xmlRoot.addElement("ItemDefinitionGroup", [&condition, &vcxprojAdapter](XmlElement& node) {
			node.addAttribute("Condition", condition);
			node.addElement("ClCompile", [&vcxprojAdapter](XmlElement& node2) {
				node2.addElementWithTextIfNotEmpty("ConformanceMode", vcxprojAdapter.getConformanceMode());
				node2.addElementWithTextIfNotEmpty("LanguageStandard", vcxprojAdapter.getLanguageStandardCpp());
				node2.addElementWithTextIfNotEmpty("LanguageStandard_C", vcxprojAdapter.getLanguageStandardC());

				// C/C++ Settings
				node2.addElementWithTextIfNotEmpty("AdditionalIncludeDirectories", vcxprojAdapter.getAdditionalIncludeDirectories());

				if (vcxprojAdapter.usesPrecompiledHeader())
				{
					node2.addElementWithTextIfNotEmpty("PrecompiledHeader", "Use");
					node2.addElementWithTextIfNotEmpty("PrecompiledHeaderFile", vcxprojAdapter.getPrecompiledHeaderMinusLocation());
					node2.addElementWithTextIfNotEmpty("PrecompiledHeaderOutputFile", vcxprojAdapter.getPrecompiledHeaderOutputFile());
					node2.addElementWithTextIfNotEmpty("ForcedIncludeFiles", vcxprojAdapter.getPrecompiledHeaderMinusLocation());
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

				node2.addElementWithText("AdditionalOptions", vcxprojAdapter.getAdditionalOptions());
			});

			if (vcxprojAdapter.usesLibrarian())
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
					node2.addElementWithTextIfNotEmpty("SubSystem", vcxprojAdapter.getSubSystem());
					node2.addElementWithText("EnableCOMDATFolding", vcxprojAdapter.getEnableCOMDATFolding());
					node2.addElementWithText("OptimizeReferences", vcxprojAdapter.getOptimizeReferences());
					node2.addElementWithText("GenerateDebugInformation", vcxprojAdapter.getGenerateDebugInformation());
				});
			}

			node.addElement("ResourceCompile", [&vcxprojAdapter](XmlElement& node2) {
				node2.addElementWithText("PreprocessorDefinitions", vcxprojAdapter.getPreprocessorDefinitions());
				node2.addElementWithTextIfNotEmpty("AdditionalIncludeDirectories", vcxprojAdapter.getAdditionalIncludeDirectories(true));
			});
		});
	}

	{
		OrderedDictionary<bool> headers;
		OrderedDictionary<StringList> sources;
		OrderedDictionary<StringList> resources;
		std::pair<std::string, StringList> manifest;
		std::pair<std::string, StringList> icon;
		std::string pchSource;
		StringList allConfigs;

		for (auto& state : m_states)
		{
			const auto& project = *getProjectFromStateContext(*state, inName);
			const auto& config = state->configuration.name();
			const auto& vcxprojAdapter = *m_adapters.at(config);

			allConfigs.emplace_back(config);

			auto headerFiles = project.getHeaderFiles();
			const auto& pch = project.precompiledHeader();
			if (!pch.empty())
			{
				headerFiles.push_back(pch);
				if (pchSource.empty())
				{
					pchSource = vcxprojAdapter.getPrecompiledHeaderSourceFile();
				}
			}

			for (auto& file : headerFiles)
			{
				headers[file] = true;
			}

			SourceOutputs& outputs = *m_outputs.at(config).get();

			for (auto& group : outputs.groups)
			{
				const auto& file = group->sourceFile;
				switch (group->type)
				{
					case SourceType::C:
					case SourceType::CPlusPlus: {
						if (sources.find(file) == sources.end())
							sources[file] = StringList{ config };
						else
							sources[file].emplace_back(config);

						break;
					}
					case SourceType::WindowsResource: {
						if (resources.find(file) == resources.end())
							resources[file] = StringList{ config };
						else
							resources[file].emplace_back(config);

						break;
					}
					default:
						break;
				}
			}

			manifest.first = state->paths.getWindowsManifestFilename(project);
			if (!manifest.first.empty())
				manifest.second.emplace_back(config);

			icon.first = project.windowsApplicationIcon();
			if (!icon.first.empty())
				icon.second.emplace_back(config);

			/*const auto& projectFiles = project.files();
			for (auto& file : projectFiles)
			{
				if (files.find(file) == files.end())
					files[file] = StringList{ config };
				else
					files[file].emplace_back(config);
			}*/
		}

		UNUSED(resources);

		xmlRoot.addElement("ItemGroup", [this, &headers](XmlElement& node) {
			for (auto& it : headers)
			{
				const auto& file = it.first;

				node.addElement("ClInclude", [this, &file](XmlElement& node2) {
					node2.addAttribute("Include", fmt::format("{}/{}", m_cwd, file));
				});
			}
		});

		xmlRoot.addElement("ItemGroup", [this, &allConfigs, &sources, &pchSource](XmlElement& node) {
			if (!pchSource.empty())
			{
				node.addElement("ClCompile", [&pchSource](XmlElement& node2) {
					node2.addAttribute("Include", pchSource);
					node2.addElementWithText("PrecompiledHeader", "Create");
					node2.addElementWithText("ForcedIncludeFiles", std::string());
					node2.addElementWithText("ObjectFileName", "$(IntDir)");
				});
			}

			for (auto& it : sources)
			{
				const auto& file = it.first;
				const auto& configs = it.second;

				node.addElement("ClCompile", [this, &file, &allConfigs, &configs](XmlElement& node2) {
					node2.addAttribute("Include", fmt::format("{}/{}", m_cwd, file));

					for (auto& config : allConfigs)
					{
						if (!List::contains(configs, config))
						{
							auto condition = getCondition(config);
							node2.addElement("ExcludedFromBuild", [&condition](XmlElement& node3) {
								node3.addAttribute("Condition", condition);
								node3.setText("true");
							});
						}
					}
				});
			}
		});

		xmlRoot.addElement("ItemGroup", [this, &allConfigs, &resources](XmlElement& node) {
			for (auto& it : resources)
			{
				const auto& file = it.first;
				const auto& configs = it.second;

				node.addElement("ResourceCompile", [this, &file, &allConfigs, &configs](XmlElement& node2) {
					node2.addAttribute("Include", fmt::format("{}/{}", m_cwd, file));
					node2.addElementWithText("PrecompiledHeader", "NotUsing");

					for (auto& config : allConfigs)
					{
						if (!List::contains(configs, config))
						{
							auto condition = getCondition(config);
							node2.addElement("ExcludedFromBuild", [&condition](XmlElement& node3) {
								node3.addAttribute("Condition", condition);
								node3.setText("true");
							});
						}
					}
				});
			}
		});

		if (!manifest.first.empty())
		{
			xmlRoot.addElement("ItemGroup", [this, &manifest, &allConfigs](XmlElement& node) {
				node.addElement("Manifest", [this, &manifest, &allConfigs](XmlElement& node2) {
					node2.addAttribute("Include", fmt::format("{}/{}", m_cwd, manifest.first));

					for (auto& config : allConfigs)
					{
						if (!List::contains(manifest.second, config))
						{
							auto condition = getCondition(config);
							node2.addElement("ExcludedFromBuild", [&condition](XmlElement& node3) {
								node3.addAttribute("Condition", condition);
								node3.setText("true");
							});
						}
					}
				});
			});
		}

		if (!icon.first.empty())
		{
			xmlRoot.addElement("ItemGroup", [this, &icon, &allConfigs](XmlElement& node) {
				node.addElement("Image", [this, &icon, &allConfigs](XmlElement& node2) {
					node2.addAttribute("Include", fmt::format("{}/{}", m_cwd, icon.first));

					for (auto& config : allConfigs)
					{
						if (!List::contains(icon.second, config))
						{
							auto condition = getCondition(config);
							node2.addElement("ExcludedFromBuild", [&condition](XmlElement& node3) {
								node3.addAttribute("Condition", condition);
								node3.setText("true");
							});
						}
					}
				});
			});
		}
	}

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

	// TODO: Extensions should be cumulative across configurations
	//   This is kind of hacky otherwise
	const auto& config = inState.configuration.name();
	const auto& vcxprojAdapter = *m_adapters.at(config);

	auto& xml = xmlFile.xml;
	auto& xmlRoot = xml.root();
	xmlRoot.setName("Project");
	xmlRoot.addAttribute("ToolsVersion", "4.0");
	xmlRoot.addAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
	xmlRoot.addElement("ItemGroup", [&vcxprojAdapter](XmlElement& node) {
		node.addElement("ClInclude", [&vcxprojAdapter](XmlElement& node2) {
			node2.addAttribute("Include", vcxprojAdapter.getPrecompiledHeaderFile());
			node2.addElementWithText("Filter", "Precompile Header Files");
		});
	});
	xmlRoot.addElement("ItemGroup", [&vcxprojAdapter](XmlElement& node) {
		node.addElement("ClCompile", [&vcxprojAdapter](XmlElement& node2) {
			node2.addAttribute("Include", vcxprojAdapter.getPrecompiledHeaderSourceFile());
			node2.addElementWithText("Filter", "Precompile Header Files");
		});
	});
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
		node.addElement("Filter", [this, &vcxprojAdapter](XmlElement& node2) {
			auto extensions = String::join(vcxprojAdapter.getHeaderExtensions(), ';');
			auto guid = Uuid::v5(extensions, m_projectTypeGuid).toUpperCase();
			node2.addAttribute("Include", "Precompile Header Files");
			node2.addElementWithText("UniqueIdentifier", fmt::format("{{{}}}", guid));
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
	xmlRoot.addElement("PropertyGroup", [this](XmlElement& node) {
		node.addElementWithText("LocalDebuggerWorkingDirectory", m_cwd);
		node.addElementWithText("DebuggerFlavor", "WindowsLocalDebugger");
	});

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

/*****************************************************************************/
std::string VSVCXProjGen::getWindowsTargetPlatformVersion() const
{
	auto ret = Environment::getAsString("UCRTVersion");
	if (ret.empty())
		ret = "10.0";

	return ret;
}

/*****************************************************************************/
std::string VSVCXProjGen::getCondition(const std::string& inConfig) const
{
	return fmt::format("'$(Configuration)|$(Platform)'=='{}|x64'", inConfig);
}

}
