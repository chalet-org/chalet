/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Generator/NativeGenerator.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/SourceOutputs.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Shell.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

// TODO: Finish.

namespace chalet
{
/*****************************************************************************/
NativeGenerator::NativeGenerator(BuildState& inState) :
	m_state(inState),
	m_sourceCache(m_state.cache.file().sources())
{
}

/*****************************************************************************/
bool NativeGenerator::addProject(const SourceTarget& inProject, const Unique<SourceOutputs>& inOutputs, CompileToolchain& inToolchain)
{
	const auto& name = inProject.name();

	m_project = &inProject;
	m_toolchain = inToolchain.get();

	m_pchChanged = false;
	m_sourcesChanged = false;

	chalet_assert(m_project != nullptr, "");

	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project);
	const auto& outputs = inOutputs;

	bool targetExists = Files::pathExists(outputs->target);

	bool dependentchanged = targetExists && checkDependentTargets(inProject);

	{
		CommandPool::JobList jobs;

		{
			auto target = std::make_unique<CommandPool::Job>();
			target->list = getPchCommands(pchTarget);
			if (!target->list.empty() || !targetExists)
			{
				jobs.emplace_back(std::move(target));
			}
		}

		bool compileTarget = m_sourcesChanged || m_pchChanged || dependentchanged || !targetExists;
		{
			auto target = std::make_unique<CommandPool::Job>();
			target->list = getCompileCommands(outputs->groups);
			if (!target->list.empty() || !targetExists)
			{
				jobs.emplace_back(std::move(target));
				compileTarget = true;
			}
		}

		if (compileTarget && !List::contains(m_fileCache, outputs->target))
		{
			m_fileCache.push_back(outputs->target);

			auto target = std::make_unique<CommandPool::Job>();
			target->list = getLinkCommand(outputs->target, outputs->objectListLinker);
			if (!target->list.empty())
			{
				jobs.emplace_back(std::move(target));
			}
		}

		if (!jobs.empty())
		{
			if (m_targets.find(name) == m_targets.end())
			{
				m_targets.emplace(name, std::move(jobs));
			}
		}
	}

	m_toolchain = nullptr;
	m_project = nullptr;

	return true;
}

/*****************************************************************************/
bool NativeGenerator::buildProject(const SourceTarget& inProject)
{
	m_targetsChanged.clear();
	m_fileCache.clear();

	if (m_targets.find(inProject.name()) == m_targets.end())
		return true;

	auto& buildJobs = m_targets.at(inProject.name());

	if (!buildJobs.empty())
	{
		CommandPool::Settings settings;
		settings.color = Output::theme().build;
		settings.msvcCommand = m_state.environment->isMsvc();
		settings.keepGoing = m_state.info.keepGoing();
		settings.showCommands = Output::showCommands();
		settings.quiet = Output::quietNonBuild();

		if (!m_commandPool->runAll(buildJobs, settings))
		{
			m_state.cache.file().setDisallowSave(true);

			Output::lineBreak();
			return false;
		}

		Output::lineBreak();
	}

	return true;
}

/*****************************************************************************/
void NativeGenerator::initialize()
{
	m_commandPool = std::make_unique<CommandPool>(m_state.info.maxJobs());
}

/*****************************************************************************/
void NativeGenerator::dispose() const
{
	m_commandPool.reset();
}

/*****************************************************************************/
CommandPool::CmdList NativeGenerator::getPchCommands(const std::string& pchTarget)
{
	chalet_assert(m_project != nullptr, "");

	CommandPool::CmdList ret;
	if (m_project->usesPrecompiledHeader())
	{
		const auto& source = m_project->precompiledHeader();
		const auto& objDir = m_state.paths.objDir();

#if defined(CHALET_MACOS)
		if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
		{
			auto baseFolder = String::getPathFolder(pchTarget);
			auto filename = String::getPathFilename(pchTarget);

			for (auto& arch : m_state.inputs.universalArches())
			{
				auto outObject = fmt::format("{}_{}/{}", baseFolder, arch, filename);
				auto intermediateSource = String::getPathFolderBaseName(outObject);

				bool pchChanged = fileChangedOrDependentChanged(source, outObject);
				m_pchChanged |= pchChanged;
				if (pchChanged)
				{
					auto pchCache = fmt::format("{}/{}", objDir, intermediateSource);
					if (!List::contains(m_fileCache, intermediateSource))
					{
						m_fileCache.emplace_back(std::move(pchCache));

						auto dependency = m_state.environment->getDependencyFile(source);

						CommandPool::Cmd out;
						out.output = fmt::format("{} ({})", source, arch);
						out.command = m_toolchain->compilerCxx->getPrecompiledHeaderCommand(source, outObject, m_generateDependencies, dependency, arch);

						ret.emplace_back(std::move(out));
					}
				}
			}
		}
		else
#endif
		{
			bool pchChanged = fileChangedOrDependentChanged(source, pchTarget);
			m_pchChanged |= pchChanged;
			if (pchChanged)
			{
				auto pchCache = fmt::format("{}/{}", objDir, source);
				if (!List::contains(m_fileCache, pchCache))
				{
					m_fileCache.emplace_back(std::move(pchCache));

					auto dependency = m_state.environment->getDependencyFile(source);

					CommandPool::Cmd out;
					out.output = source;
					out.command = m_toolchain->compilerCxx->getPrecompiledHeaderCommand(source, pchTarget, m_generateDependencies, dependency, std::string());

					const auto& cxxExt = m_state.paths.cxxExtension();
					if (!cxxExt.empty())
						out.reference = fmt::format("{}.{}", out.output, cxxExt);

#if defined(CHALET_WIN32)
					if (m_state.environment->isMsvc())
						out.dependency = std::move(dependency);
#endif

					ret.emplace_back(std::move(out));
				}
			}
		}

		m_sourcesChanged |= m_pchChanged;
	}

	return ret;
}

/*****************************************************************************/
CommandPool::CmdList NativeGenerator::getCompileCommands(const SourceFileGroupList& inGroups)
{
	chalet_assert(m_project != nullptr, "");

	CommandPool::CmdList ret;

	const auto& objDir = m_state.paths.objDir();

	const bool objectiveCxx = m_project->objectiveCxx();

	for (auto& group : inGroups)
	{
		const auto& source = group->sourceFile;
		const auto& target = group->objectFile;

		if (source.empty())
			continue;

		if (!objectiveCxx && (group->type == SourceType::ObjectiveC || group->type == SourceType::ObjectiveCPlusPlus))
			continue;

		switch (group->type)
		{
			case SourceType::WindowsResource: {
				bool sourceChanged = fileChangedOrDependentChanged(source, target);
				m_sourcesChanged |= sourceChanged;
				if (sourceChanged)
				{
					auto sourceFile = fmt::format("{}/{}", objDir, source);
					if (!List::contains(m_fileCache, sourceFile))
					{
						m_fileCache.emplace_back(std::move(sourceFile));

						CommandPool::Cmd out;
						out.output = std::move(source);
						out.command = getRcCompile(source, target);
						out.reference = out.output;

						ret.emplace_back(std::move(out));
					}
				}
				break;
			}
			case SourceType::C:
			case SourceType::CPlusPlus:
			case SourceType::ObjectiveC:
			case SourceType::ObjectiveCPlusPlus: {
				bool sourceChanged = fileChangedOrDependentChanged(source, target);
				m_sourcesChanged |= sourceChanged;
				if (sourceChanged || m_pchChanged)
				{
					auto sourceFile = fmt::format("{}/{}", objDir, source);
					if (!List::contains(m_fileCache, sourceFile))
					{
						m_fileCache.emplace_back(std::move(sourceFile));

						CommandPool::Cmd out;
						out.output = std::move(source);
						out.command = getCxxCompile(source, target, group->type);
						out.reference = out.output;

#if defined(CHALET_WIN32)
						if (m_state.environment->isMsvc())
							out.dependency = m_state.environment->getDependencyFile(source);
#endif
						ret.emplace_back(std::move(out));
					}
				}
				break;
			}

			case SourceType::CxxPrecompiledHeader:
			case SourceType::Unknown:
			default:
				break;
		}
	}

	if (m_sourcesChanged)
	{
		m_targetsChanged.emplace_back(m_project->name());
	}

	return ret;
}

/*****************************************************************************/
CommandPool::CmdList NativeGenerator::getLinkCommand(const std::string& inTarget, const StringList& inObjects)
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	const auto targetBasename = m_state.paths.getTargetBasename(*m_project);

	CommandPool::CmdList ret;

	{
		CommandPool::Cmd out;
		out.command = m_toolchain->getOutputTargetCommand(inTarget, inObjects, targetBasename);

		auto label = m_project->isStaticLibrary() ? "Archiving" : "Linking";
		out.output = fmt::format("{} {}", label, inTarget);

		ret.emplace_back(std::move(out));
	}

	return ret;
}

/*****************************************************************************/
StringList NativeGenerator::getCxxCompile(const std::string& source, const std::string& target, const SourceType derivative) const
{
	chalet_assert(m_toolchain != nullptr, "");

	StringList ret;

	auto dependency = m_state.environment->getDependencyFile(source);
	ret = m_toolchain->compilerCxx->getCommand(source, target, m_generateDependencies, dependency, derivative);

	return ret;
}

/*****************************************************************************/
StringList NativeGenerator::getRcCompile(const std::string& source, const std::string& target) const
{
	chalet_assert(m_toolchain != nullptr, "");

	StringList ret;

	auto dependency = m_state.environment->getDependencyFile(source);
	ret = m_toolchain->compilerWindowsResource->getCommand(source, target, m_generateDependencies, dependency);

	return ret;
}

/*****************************************************************************/
bool NativeGenerator::fileChangedOrDependentChanged(const std::string& source, const std::string& target)
{
	bool result = m_sourceCache.fileChangedOrDoesNotExist(source, target);
	if (!result)
	{
		if (m_cwd.empty())
		{
			m_cwd = m_state.inputs.workingDirectory() + '/';
		}

		auto dependency = m_state.environment->getDependencyFile(source);
		if (Files::pathExists(dependency))
		{
			std::ifstream input(dependency);
			for (std::string line; std::getline(input, line);)
			{
				if (line.empty())
					continue;

				if (!String::endsWith(':', line))
					continue;

				String::replaceAll(line, m_cwd, "");

				line.pop_back();

				if (m_sourceCache.fileChangedOrDoesNotExist(line))
				{
					result = true;
					break;
				}

				// LOG(source, line);
			}
		}
	}
	return result;
}

/*****************************************************************************/
bool NativeGenerator::checkDependentTargets(const SourceTarget& inProject) const
{
	bool result = false;

	auto links = List::combineRemoveDuplicates(inProject.projectSharedLinks(), inProject.projectStaticLinks());
	for (auto& link : links)
	{
		if (List::contains(m_targetsChanged, link))
		{
			result = true;
			break;
		}
	}

	return result;
}
}