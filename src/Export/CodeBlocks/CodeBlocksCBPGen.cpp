/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CodeBlocks/CodeBlocksCBPGen.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Compile/CompileToolchainController.hpp"
#include "Compile/Linker/ILinker.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/TargetExportAdapter.hpp"
#include "State/BuildConfiguration.hpp"
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
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CodeBlocksCBPGen::CodeBlocksCBPGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inAllBuildName) :
	m_states(inStates),
	m_allBuildName(inAllBuildName),
	m_headerExtensions({ "h", "hpp", "hh", "hxx", "H", "inl", "ii", "ixx", "h++", "ipp", "txx", "tpp", "tpl" })
{
}

/*****************************************************************************/
bool CodeBlocksCBPGen::saveProjectFiles(const std::string& inDirectory)
{
	m_exportPath = String::getPathFolder(inDirectory);

	if (!initialize())
		return false;

	for (auto& [name, group] : m_groups)
	{
		auto projectFile = fmt::format("{}/{}.cbp", inDirectory, name);
		if (!saveSourceTargetProjectFiles(projectFile, name, group))
		{
			Diagnostic::error("There was a problem creating the CodeBlocks project file for the target: {}", name);
			return false;
		}
	}

	UNUSED(inDirectory);
	return true;
}

/*****************************************************************************/
bool CodeBlocksCBPGen::initialize()
{
	if (m_states.empty())
		return false;

	const auto& firstState = *m_states.front();
	auto& groups = m_groups;

	const auto& inputFile = firstState.inputs.inputFile();

	m_resourceExtensions = firstState.paths.windowsResourceExtensions();
	m_resourceExtensions.emplace_back("manifest");
	m_cwd = Files::getCanonicalPath(firstState.inputs.workingDirectory());
	m_defaultInputFile = firstState.inputs.defaultInputFile();
	m_yamlInputFile = firstState.inputs.yamlInputFile();

	for (auto& state : m_states)
	{
		const auto& configName = state->configuration.name();
		if (m_configToTargets.find(configName) == m_configToTargets.end())
			m_configToTargets.emplace(configName, std::vector<const IBuildTarget*>{});

		for (auto& target : state->targets)
		{
			m_configToTargets[configName].push_back(target.get());

			if (target->isSources())
			{
				auto& sourceTarget = static_cast<const SourceTarget&>(*target);
				state->paths.setBuildDirectoriesBasedOnProjectKind(sourceTarget);

				const auto& name = sourceTarget.name();
				if (groups.find(name) == groups.end())
				{
					groups.emplace(name, TargetGroup{});
					groups[name].pch = sourceTarget.precompiledHeader();
					// groups[name].path = workingDirectory;
					groups[name].kind = TargetGroupKind::Source;
				}

				state->getTargetDependencies(m_groups[name].dependencies, name, false);

				auto& sharedLinks = sourceTarget.projectSharedLinks();
				for (auto& link : sharedLinks)
					List::addIfDoesNotExist(groups[name].dependencies, link);

				auto& staticLinks = sourceTarget.projectStaticLinks();
				for (auto& link : staticLinks)
					List::addIfDoesNotExist(groups[name].dependencies, link);

				auto& sources = groups[name].sources;
				const auto& files = sourceTarget.files();
				for (auto& file : files)
				{
					sources[file].push_back(configName);
				}

				auto tmpHeaders = sourceTarget.getHeaderFiles();
				for (auto& file : tmpHeaders)
				{
					sources[file].push_back(configName);
				}

				auto windowsManifest = state->paths.getWindowsManifestFilename(sourceTarget);
				if (!windowsManifest.empty())
					sources[windowsManifest].push_back(configName);

				if (state->environment->isWindowsTarget())
				{
					auto windowsIconResource = state->paths.getWindowsIconResourceFilename(sourceTarget);
					if (!windowsIconResource.empty())
						sources[windowsIconResource].push_back(configName);

					auto windowsManifestResource = state->paths.getWindowsManifestResourceFilename(sourceTarget);
					if (!windowsManifestResource.empty())
						sources[windowsManifestResource].push_back(configName);
				}
			}
			else
			{
				TargetExportAdapter adapter(*state, *target);
				auto command = adapter.getCommand();
				if (!command.empty())
				{
					auto& name = target->name();
					if (groups.find(name) == groups.end())
					{
						groups.emplace(name, TargetGroup{});
						groups[name].kind = TargetGroupKind::Script;
					}

#if defined(CHALET_WIN32)
					String::replaceAll(command, "\r\n", "\n");
#endif
					String::replaceAll(command, state->toolchain.compilerC().path, "$(TARGET_CC)");
					String::replaceAll(command, state->toolchain.compilerCpp().path, "$(TARGET_CPP)");
					String::replaceAll(command, state->toolchain.linker(), "$(TARGET_LD)");
					String::replaceAll(command, state->toolchain.archiver(), "$(TARGET_LIB)");
					String::replaceAll(command, state->toolchain.compilerWindowsResource(), "$(WINDRES)");

					groups[name].scripts.push_back(std::move(command));

					auto& sources = groups[name].sources;
					auto files = adapter.getFiles();
					for (auto& file : files)
					{
						sources[file].push_back(configName);
					}

					state->getTargetDependencies(m_groups[name].dependencies, name, false);
				}
			}

			if (m_compiler.empty())
			{
				if (state->environment->isClang())
					m_compiler = "clang";
				else
					m_compiler = "gcc";
			}
		}
	}

	{
		auto rootBuildFile = getResolvedPath(inputFile);
		TargetGroup buildAllGroup;
		buildAllGroup.kind = TargetGroupKind::BuildAll;
		auto& sources = buildAllGroup.sources[rootBuildFile];
		for (auto& [config, _] : m_configToTargets)
			sources.push_back(config);

		for (auto& [target, _] : groups)
		{
			buildAllGroup.dependencies.push_back(target);
		}
		groups.emplace(m_allBuildName, std::move(buildAllGroup));
	}

	for (auto& [name, group] : groups)
	{
		bool isScript = group.kind == TargetGroupKind::Script;
		bool isBuildAll = group.kind == TargetGroupKind::BuildAll;
		if (isScript || isBuildAll)
		{
			std::string makefileContents;
			size_t index = 0;
			if (isBuildAll)
			{
				makefileContents += R"shell(build:
	@echo Building

clean:
	@echo Nothing to clean

)shell";
			}
			else
			{
				for (auto& [config, _] : m_configToTargets)
				{
					std::string dependency;
					for (auto& state : m_states)
					{
						if (String::equals(config, state->configuration.name()))
						{
							dependency = fmt::format("{}/cache", getResolvedPath(state->paths.buildOutputDir()));
							break;
						}
					}
					if (!Files::pathExists(dependency))
						Files::makeDirectory(dependency);

					dependency += fmt::format("/{}", Hash::string(fmt::format("{}_{}", name, config)));

					auto& script = group.scripts.at(index);
					auto split = String::split(script, '\n');
					split.emplace_back(fmt::format("echo Generated > {}", dependency));
#if defined(CHALET_WIN32)
					std::string removeFile{ "del" };
					Path::toWindows(dependency);
#else
					std::string removeFile{ "rm -f" };
#endif
					makefileContents += fmt::format(R"shell({dependency}:
	@{cmd}

{config}: {dependency}
.PHONY: {config}

clean{config}:
	-{removeFile} "{dependency}"
.PHONY: clean{config}

)shell",
						FMT_ARG(dependency),
						FMT_ARG(config),
						FMT_ARG(removeFile),
						fmt::arg("cmd", String::join(split, "\n\t@")));
					index++;
				}
			}

			auto outPath = fmt::format("{}/scripts/{}.mk", m_exportPath, Hash::uint64(name));
			Files::createFileWithContents(outPath, makefileContents);
		}
	}

	return true;
}

/*****************************************************************************/
bool CodeBlocksCBPGen::saveSourceTargetProjectFiles(const std::string& inFilename, const std::string& inName, const TargetGroup& group)
{
	XmlFile xmlFile(inFilename);

	auto& xml = xmlFile.xml;
	auto& xmlRoot = xml.root();

	xmlRoot.setName("CodeBlocks_project_file");
	xmlRoot.addElement("FileVersion", [](XmlElement& node) {
		node.addAttribute("major", "1");
		node.addAttribute("minor", "6");
	});
	xmlRoot.addElement("Project", [this, &inName, &group](XmlElement& node) {
		node.addElement("Option", [&inName](XmlElement& node2) {
			node2.addAttribute("title", inName);
		});

		if (group.kind == TargetGroupKind::Script || group.kind == TargetGroupKind::BuildAll)
		{
			node.addElement("Option", [](XmlElement& node2) {
				node2.addAttribute("makefile_is_custom", "1");
			});
		}

		node.addElement("Option", [this](XmlElement& node2) {
			node2.addAttribute("compiler", m_compiler);
		});
		node.addElement("Option", [](XmlElement& node2) {
			node2.addAttribute("extended_obj_names", "1");
		});
		node.addElement("Build", [this, &inName](XmlElement& node2) {
			// Call function that makes each target
			StringList configs;
			for (auto& [config, _] : m_configToTargets)
				configs.emplace_back(config);

			for (auto& config : configs)
			{
				addBuildConfigurationForTarget(node2, inName, config);
			}
		});
		for (auto& it : group.sources)
		{
			auto& file = it.first;
			auto& configs = it.second;
			node.addElement("Unit", [this, &file, &configs, &group](XmlElement& node2) {
				auto resolved = getResolvedPath(file);
				node2.addAttribute("filename", resolved);

				auto virtualFolder = getVirtualFolder(file, group.pch);
				node2.addElement("Option", [&virtualFolder](XmlElement& node3) {
					node3.addAttribute("virtualFolder", virtualFolder);
				});

				for (auto& config : configs)
				{
					node2.addElement("Option", [&config](XmlElement& node3) {
						node3.addAttribute("target", config);
					});
				}
			});
		}
		node.addElement("Extensions", [](XmlElement& node2) {
			node2.addElement("Extensions", [](XmlElement& node3) {
				node3.addElement("code_completion");
				node3.addElement("envvars");
				node3.addElement("debugger");
				node3.addElement("lib_finder", [](XmlElement& node4) {
					node4.addAttribute("disable_auto", "1");
				});
			});
		});
	});

	xml.setStandalone(true);
	if (!xmlFile.save(1))
	{
		Diagnostic::error("There was a problem saving: {}", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
void CodeBlocksCBPGen::addBuildConfigurationForTarget(XmlElement& outNode, const std::string& inName, const std::string& inConfigName) const
{
	StringList configs;
	for (auto& [config, targets] : m_configToTargets)
	{
		for (auto& target : targets)
		{
			if (String::equals(inName, target->name()))
				List::addIfDoesNotExist(configs, config);
		}
	}

	outNode.addElement("Target", [this, &configs, &inName, &inConfigName](XmlElement& node) {
		node.addAttribute("title", inConfigName);

		bool isAllTarget = String::equals(m_allBuildName, inName);
		if (!isAllTarget && !List::contains(configs, inConfigName))
		{
			node.addElement("Option", [](XmlElement& node2) {
				node2.addAttribute("platforms", "");
			});
		}

		for (auto& state : m_states)
		{
			if (!String::equals(inConfigName, state->configuration.name()))
				continue;

			if (isAllTarget)
			{
				addAllBuildTarget(node, *state);
			}
			else
			{
				bool found = false;
				for (auto& target : state->targets)
				{
					if (!String::equals(inName, target->name()))
						continue;

					if (target->isSources())
					{
						auto& sourceTarget = static_cast<const SourceTarget&>(*target);

						auto toolchain = std::make_unique<CompileToolchainController>(sourceTarget);
						if (!toolchain->initialize(*state))
						{
							Diagnostic::error("Error preparing the toolchain for project: {}", sourceTarget.name());
							continue;
						}

						state->paths.setBuildDirectoriesBasedOnProjectKind(sourceTarget);
						addSourceTarget(node, *state, sourceTarget, *toolchain);
					}
					else
					{
						addScriptTarget(node, *state, inName);
					}
					found = true;
					break;
				}
				if (found)
					break;
			}
		}
	});
	// <Option platforms="" />
}

/*****************************************************************************/
void CodeBlocksCBPGen::addSourceTarget(XmlElement& outNode, const BuildState& inState, const SourceTarget& inTarget, const CompileToolchainController& inToolchain) const
{
	outNode.addElement("Option", [&inState, &inTarget](XmlElement& node2) {
		auto outputFile = Files::getCanonicalPath(inState.paths.getTargetFilename(inTarget));
		node2.addAttribute("output", outputFile);
		node2.addAttribute("prefix_auto", "0");
		node2.addAttribute("extension_auto", "0");
	});
	outNode.addElement("Option", [this](XmlElement& node2) {
		node2.addAttribute("working_dir", m_cwd);
	});

	outNode.addElement("Option", [this, &inState](XmlElement& node2) {
		node2.addAttribute("object_output", getResolvedObjDir(inState));
	});
	// outNode.addElement("Option", [](XmlElement& node2) {
	// 	node2.addAttribute("use_console_runner", "1");
	// });

	// projectStaticLinks - resolved ?
	// <Option external_deps="" />
	// <Option additional_output="" />

	outNode.addElement("Option", [this, &inTarget](XmlElement& node) {
		node.addAttribute("type", getOutputType(inTarget));
	});

	const auto& runArgumentMap = inState.getCentralState().runArgumentMap();
	if (runArgumentMap.find(inTarget.name()) != runArgumentMap.end())
	{
		auto& args = runArgumentMap.at(inTarget.name());
		outNode.addElement("Option", [&args](XmlElement& node) {
			node.addAttribute("parameters", args);
		});
	}

	outNode.addElement("Option", [](XmlElement& node) {
		node.addAttribute("pch_mode", "1");
	});

	if (inTarget.isStaticLibrary())
	{
		outNode.addElement("Option", [](XmlElement& node) {
			node.addAttribute("createDefFile", "1");
		});
		outNode.addElement("Option", [](XmlElement& node) {
			node.addAttribute("createStaticLib", "1");
		});
	}

	outNode.addElement("Compiler", [this, &inTarget, &inToolchain, &inState](XmlElement& node) {
		addSourceCompilerOptions(node, inState, inTarget, inToolchain);
	});
	outNode.addElement("Linker", [this, &inTarget, &inToolchain, &inState](XmlElement& node) {
		addSourceLinkerOptions(node, inState, inTarget, inToolchain);
	});

	/*
		<ExtraCommands>
			<Add before=""/>
			<Add after=""/>
			<Mode after="always" />
		</ExtraCommands>
	*/
}

/*****************************************************************************/
void CodeBlocksCBPGen::addSourceCompilerOptions(XmlElement& outNode, const BuildState& inState, const SourceTarget& inTarget, const CompileToolchainController& inToolchain) const
{
	UNUSED(outNode, inTarget, inToolchain, inState);

	// Compiler Options
	{
		StringList argList;
		inToolchain.compilerCxx->getCommandOptions(argList, inTarget.getDefaultSourceType());
		if (inTarget.usesPrecompiledHeader())
		{
			auto pch = getResolvedPath(inTarget.precompiledHeader());
			argList.emplace_back(fmt::format("-include \"{}\"", pch));
		}

		for (auto& arg : argList)
		{
			outNode.addElement("Add", [&arg](XmlElement& node) {
				node.addAttribute("option", arg);
			});
		}
	}

	// Compiler Includes
	{
		const auto& includeDirs = inTarget.includeDirs();
		for (auto& dir : includeDirs)
		{
			outNode.addElement("Add", [this, &dir](XmlElement& node) {
				node.addAttribute("directory", getResolvedPath(dir));
			});
		}
	}
}

/*****************************************************************************/
void CodeBlocksCBPGen::addSourceLinkerOptions(XmlElement& outNode, const BuildState& inState, const SourceTarget& inTarget, const CompileToolchainController& inToolchain) const
{
	// Linker Options
	{
		StringList argList;
		inToolchain.linker->getCommandOptions(argList);

		for (auto& arg : argList)
		{
			outNode.addElement("Add", [&arg](XmlElement& node) {
				node.addAttribute("option", arg);
			});
		}
	}

	// Lib Dirs
	{
		const auto& libDirs = inTarget.libDirs();
		for (auto& dir : libDirs)
		{
			outNode.addElement("Add", [this, &dir](XmlElement& node) {
				node.addAttribute("directory", getResolvedLibraryPath(dir));
			});
		}

		const auto& buildDir = inState.paths.buildOutputDir();
		outNode.addElement("Add", [this, &buildDir](XmlElement& node) {
			node.addAttribute("directory", getResolvedLibraryPath(buildDir));
		});
	}

	// Links
	{
		auto links = List::combineRemoveDuplicates(inTarget.links(), inTarget.staticLinks());
		if (inState.environment->isMingw())
		{
			auto win32Links = ILinker::getWin32CoreLibraryLinks(inState, inTarget);
			for (auto& link : win32Links)
				List::addIfDoesNotExist(links, std::move(link));
		}
		for (auto& link : links)
		{
			outNode.addElement("Add", [&link](XmlElement& node) {
				node.addAttribute("library", link);
			});
		}
	}
}

/*****************************************************************************/
void CodeBlocksCBPGen::addScriptTarget(XmlElement& outNode, const BuildState& inState, const std::string& inName) const
{
	UNUSED(inState);

	outNode.addElement("Option", [this](XmlElement& node2) {
		node2.addAttribute("working_dir", m_cwd);
	});
	outNode.addElement("Option", [](XmlElement& node) {
		node.addAttribute("type", "4");
	});
	outNode.addElement("MakeCommands", [this, &inName](XmlElement& node) {
		auto makefilePath = fmt::format("{}/scripts/{}.mk", m_exportPath, Hash::uint64(inName));

		auto command = fmt::format("$make -f {} --no-builtin-rules --no-builtin-variables --no-print-directory $target TARGET_CC=$(TARGET_CC) TARGET_CPP=$(TARGET_CPP) TARGET_LD=$(TARGET_LD) TARGET_LIB=$(TARGET_LIB) WINDRES=$(WINDRES)", makefilePath);

		node.addElement("Build", [&command](XmlElement& node2) {
			node2.addAttribute("command", command);
		});
		node.addElement("CompileFile", [&command](XmlElement& node2) {
			node2.addAttribute("command", command);
		});
		node.addElement("Clean", [&makefilePath](XmlElement& node2) {
			node2.addAttribute("command", fmt::format("$make -f {} --no-builtin-rules --no-builtin-variables --no-print-directory clean$target", makefilePath));
		});
		node.addElement("DistClean", [&makefilePath](XmlElement& node2) {
			node2.addAttribute("command", fmt::format("$make -f {} --no-builtin-rules --no-builtin-variables --no-print-directory clean$target", makefilePath));
		});
		// node.addElement("AskRebuildNeeded", [&command](XmlElement& node2) {
		// 	node2.addAttribute("command", "");
		// });
		// node.addElement("SilentBuild", [&command](XmlElement& node2) {
		// 	node2.addAttribute("command", "");
		// });
	});
}

/*****************************************************************************/
void CodeBlocksCBPGen::addAllBuildTarget(XmlElement& outNode, const BuildState& inState) const
{
	UNUSED(inState);

	// outNode.addElement("Option", [this, &inState](XmlElement& node2) {
	// 	auto outputFile = fmt::format("{}/{}", m_cwd, inState.paths.objDir());
	// 	node2.addAttribute("output", outputFile);
	// 	node2.addAttribute("prefix_auto", "0");
	// 	node2.addAttribute("extension_auto", "0");
	// });
	outNode.addElement("Option", [this](XmlElement& node2) {
		node2.addAttribute("working_dir", m_cwd);
	});

	// outNode.addElement("Option", [this, &inState](XmlElement& node2) {
	// 	node2.addAttribute("object_output", getResolvedObjDir(inState));
	// });
	// outNode.addElement("Option", [](XmlElement& node2) {
	// 	node2.addAttribute("use_console_runner", "1");
	// });

	outNode.addElement("Option", [](XmlElement& node) {
		node.addAttribute("type", "4");
	});

	outNode.addElement("MakeCommands", [this](XmlElement& node) {
		auto makefilePath = fmt::format("{}/scripts/{}.mk", m_exportPath, Hash::uint64(m_allBuildName));

		auto command = fmt::format("$make -f {} --no-builtin-rules --no-builtin-variables --no-print-directory build TARGET_CC=$(TARGET_CC) TARGET_CPP=$(TARGET_CPP) TARGET_LD=$(TARGET_LD) TARGET_LIB=$(TARGET_LIB) WINDRES=$(WINDRES)", makefilePath);

		node.addElement("Build", [&command](XmlElement& node2) {
			node2.addAttribute("command", command);
		});
		node.addElement("CompileFile", [&command](XmlElement& node2) {
			node2.addAttribute("command", command);
		});
		node.addElement("Clean", [&makefilePath](XmlElement& node2) {
			node2.addAttribute("command", fmt::format("$make -f {} --no-builtin-rules --no-builtin-variables --no-print-directory clean", makefilePath));
		});
		node.addElement("DistClean", [&makefilePath](XmlElement& node2) {
			node2.addAttribute("command", fmt::format("$make -f {} --no-builtin-rules --no-builtin-variables --no-print-directory clean", makefilePath));
		});
		// node.addElement("AskRebuildNeeded", [&command](XmlElement& node2) {
		// 	node2.addAttribute("command", "");
		// });
		// node.addElement("SilentBuild", [&command](XmlElement& node2) {
		// 	node2.addAttribute("command", "");
		// });
	});
}

/*****************************************************************************/
std::string CodeBlocksCBPGen::getOutputType(const SourceTarget& inTarget) const
{
	char outputType = '1';
	if (inTarget.isSharedLibrary())
		outputType = '3';
	else if (inTarget.isStaticLibrary())
		outputType = '2';
#if defined(CHALET_WIN32)
	else if (inTarget.windowsSubSystem() == WindowsSubSystem::Windows)
		outputType = '0';
#endif

	std::string ret;
	ret += outputType;
	return ret;
}

/*****************************************************************************/
std::string CodeBlocksCBPGen::getResolvedPath(const std::string& inFile) const
{
	return Files::getCanonicalPath(inFile);
}

/*****************************************************************************/
std::string CodeBlocksCBPGen::getResolvedLibraryPath(const std::string& inFile) const
{
	return Files::getCanonicalPath(inFile);
}

/*****************************************************************************/
std::string CodeBlocksCBPGen::getResolvedObjDir(const BuildState& inState) const
{
	return Files::getCanonicalPath(inState.paths.objDir());
}

/*****************************************************************************/
std::string CodeBlocksCBPGen::getVirtualFolder(const std::string& inFile, const std::string& inPch) const
{
	if (!inPch.empty() && String::equals(inPch, inFile))
		return "Precompile Header Files";

	chalet_assert(!m_defaultInputFile.empty(), "m_defaultInputFile not set");
	if (String::endsWith(m_defaultInputFile, inFile) || String::endsWith(m_yamlInputFile, inFile))
		return "Chalet";

	if (String::endsWith("CMakeLists.txt", inFile))
		return "CMake";

	auto ext = String::getPathSuffix(inFile);
	if (!ext.empty())
	{
		if (List::contains(m_headerExtensions, ext))
			return "Header Files";
		else if (List::contains(m_resourceExtensions, ext))
			return "Resource Files";
	}

	return "Source Files";
}
}
