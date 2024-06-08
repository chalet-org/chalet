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
#include "State/CompilerTools.hpp"
#include "State/SourceOutputs.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Shell.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
NativeGenerator::NativeGenerator(BuildState& inState) :
	m_state(inState),
	m_compileAdapter(inState)
{
}

/*****************************************************************************/
bool NativeGenerator::addProject(const SourceTarget& inProject, const Unique<SourceOutputs>& inOutputs, CompileToolchain& inToolchain)
{
	const auto& name = inProject.name();

	m_project = &inProject;
	m_toolchain = inToolchain.get();

	chalet_assert(m_project != nullptr, "");

	checkCommandsForChanges();

	bool otherTargetsChanged = m_compileAdapter.anyCmakeOrSubChaletTargetsChanged();

	m_sourcesChanged = m_pchChanged = false;

	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project);
	const auto& outputs = inOutputs;

	bool targetExists = Files::pathExists(outputs->target);
	bool dependentChanged = targetExists && m_compileAdapter.checkDependentTargets(inProject);

	m_fileCache.reserve(m_fileCache.size() + outputs->groups.size() + 3);
	m_compileAdapter.setDependencyCacheSize(m_fileCache.size() * 2);

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

		m_linkTarget = m_targetCommandChanged || m_sourcesChanged || m_pchChanged || dependentChanged || otherTargetsChanged || !targetExists;
		{
			auto target = std::make_unique<CommandPool::Job>();
			target->list = getCompileCommands(outputs->groups);
			if (!target->list.empty() || !targetExists)
			{
				jobs.emplace_back(std::move(target));
				m_linkTarget = true;
			}
		}

		const auto& toCache = outputs->target;
		if (m_linkTarget && m_fileCache.find(toCache) == m_fileCache.end())
		{
			m_fileCache.insert(toCache);

			Files::removeIfExists(outputs->target);

			auto target = std::make_unique<CommandPool::Job>();
			target->list = m_compileAdapter.getLinkCommand(*m_project, *m_toolchain, *outputs);
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
	m_fileCache.clear();
	m_compileAdapter.clearDependencyCache();

	if (m_targets.find(inProject.name()) == m_targets.end())
		return true;

	auto& buildJobs = m_targets.at(inProject.name());
	if (!buildJobs.empty())
	{
		auto settings = m_compileAdapter.getCommandPoolSettings();
		if (!m_commandPool->runAll(buildJobs, settings))
		{
			for (auto& failure : m_commandPool->failures())
			{
				auto objectFile = m_state.environment->getObjectFile(failure);

				Files::removeIfExists(objectFile);
			}

			Output::lineBreak();
			return false;
		}

		Output::lineBreak(m_state.isSubChaletTarget());
	}

	return true;
}

/*****************************************************************************/
bool NativeGenerator::targetCompiled() const noexcept
{
	return m_linkTarget;
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
		auto dependency = m_state.environment->getDependencyFile(source);
		const auto& objDir = m_state.paths.objDir();

		bool pchCommandChanged = m_commandsChanged[SourceType::CxxPrecompiledHeader];

#if defined(CHALET_MACOS)
		if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
		{
			auto baseFolder = String::getPathFolder(pchTarget);
			auto filename = String::getPathFilename(pchTarget);

			for (auto& arch : m_state.inputs.universalArches())
			{
				auto outObject = fmt::format("{}_{}/{}", baseFolder, arch, filename);
				auto intermediateSource = String::getPathFolderBaseName(outObject);

				bool pchChanged = pchCommandChanged || m_compileAdapter.fileChangedOrDependentChanged(source, outObject, dependency);
				m_pchChanged |= pchChanged;
				if (pchChanged)
				{
					auto toCache = fmt::format("{}/{}", objDir, intermediateSource);
					if (m_fileCache.find(toCache) == m_fileCache.end())
					{
						m_fileCache.insert(std::move(toCache));

						Files::removeIfExists(outObject);

						CommandPool::Cmd out;
						out.output = fmt::format("{} ({})", m_state.paths.getBuildOutputPath(source), arch);
						out.command = m_toolchain->compilerCxx->getPrecompiledHeaderCommand(source, outObject, dependency, arch);

						ret.emplace_back(std::move(out));
					}
				}
			}
		}
		else
#endif
		{
			bool pchChanged = pchCommandChanged || m_compileAdapter.fileChangedOrDependentChanged(source, pchTarget, dependency);
			m_pchChanged |= pchChanged;
			if (pchChanged)
			{
				auto toCache = fmt::format("{}/{}", objDir, source);
				if (m_fileCache.find(toCache) == m_fileCache.end())
				{
					m_fileCache.insert(std::move(toCache));

					Files::removeIfExists(pchTarget);

					CommandPool::Cmd out;
					out.output = m_state.paths.getBuildOutputPath(source);
					out.command = m_toolchain->compilerCxx->getPrecompiledHeaderCommand(source, pchTarget, dependency, std::string());

					auto pchSource = m_state.environment->getPrecompiledHeaderSourceFile(*m_project);

					out.reference = String::getPathFilename(pchSource);

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
		const auto& dependency = group->dependencyFile;
		const auto& target = group->objectFile;

		if (source.empty())
			continue;

		if (!objectiveCxx && (group->type == SourceType::ObjectiveC || group->type == SourceType::ObjectiveCPlusPlus))
			continue;

		switch (group->type)
		{
			case SourceType::WindowsResource: {
				bool sourceCmdChanged = m_commandsChanged[group->type];
				bool sourceChanged = sourceCmdChanged || m_compileAdapter.fileChangedOrDependentChanged(source, target, dependency);
				m_sourcesChanged |= sourceChanged;
				if (sourceChanged)
				{
					auto toCache = fmt::format("{}/{}", objDir, source);
					if (m_fileCache.find(toCache) == m_fileCache.end())
					{
						m_fileCache.insert(std::move(toCache));

						Files::removeIfExists(target);

						CommandPool::Cmd out;
						out.output = m_state.paths.getBuildOutputPath(source);
						out.command = getRcCompile(source, target);
						out.reference = source;

						ret.emplace_back(std::move(out));
					}
				}
				break;
			}
			case SourceType::C:
			case SourceType::CPlusPlus:
			case SourceType::ObjectiveC:
			case SourceType::ObjectiveCPlusPlus: {
				bool sourceCmdChanged = m_commandsChanged[group->type];
				bool sourceChanged = sourceCmdChanged || m_compileAdapter.fileChangedOrDependentChanged(source, target, dependency);
				m_sourcesChanged |= sourceChanged;
				if (sourceChanged || m_pchChanged)
				{
					auto toCache = fmt::format("{}/{}", objDir, source);
					if (m_fileCache.find(toCache) == m_fileCache.end())
					{
						m_fileCache.insert(std::move(toCache));

						Files::removeIfExists(target);

						CommandPool::Cmd out;
						out.output = m_state.paths.getBuildOutputPath(source);
						out.command = getCxxCompile(source, target, group->type);
						out.reference = source;

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
		m_compileAdapter.addChangedTarget(*m_project);
	}

	return ret;
}

/*****************************************************************************/
StringList NativeGenerator::getCxxCompile(const std::string& source, const std::string& target, const SourceType derivative) const
{
	chalet_assert(m_toolchain != nullptr, "");

	StringList ret;

	auto dependency = m_state.environment->getDependencyFile(source);
	ret = m_toolchain->compilerCxx->getCommand(source, target, dependency, derivative);

	return ret;
}

/*****************************************************************************/
StringList NativeGenerator::getRcCompile(const std::string& source, const std::string& target) const
{
	chalet_assert(m_toolchain != nullptr, "");

	StringList ret;

	auto dependency = m_state.environment->getDependencyFile(source);
	ret = m_toolchain->compilerWindowsResource->getCommand(source, target, dependency);

	return ret;
}

/*****************************************************************************/
void NativeGenerator::checkCommandsForChanges()
{
	m_commandsChanged.clear();
	m_targetCommandChanged = false;

	auto& name = m_project->name();
	auto& sourceCache = m_state.cache.file().sources();

	{
		auto derivative = m_project->getDefaultSourceType();
		SourceTypeList types{
			derivative
		};
		if (m_project->usesPrecompiledHeader())
		{
			types.emplace_back(SourceType::CxxPrecompiledHeader);
		}
		if (m_project->objectiveCxx())
		{
			if (derivative == SourceType::ObjectiveC)
				types.emplace_back(SourceType::C);
			else if (derivative == SourceType::ObjectiveCPlusPlus)
				types.emplace_back(SourceType::CPlusPlus);
		}
		if (m_state.toolchain.canCompileWindowsResources())
		{
			types.emplace_back(SourceType::WindowsResource);
		}

		for (auto& type : types)
		{
			auto cxxHashKey = Hash::string(fmt::format("{}_source_{}", name, static_cast<int>(type)));
			StringList options;
			if (type == SourceType::WindowsResource)
				options = m_toolchain->compilerWindowsResource->getCommand("cmd.rc", "cmd.res", "cmd.rc.d");
			else if (type == SourceType::CxxPrecompiledHeader)
				options = m_toolchain->compilerCxx->getPrecompiledHeaderCommand("cmd.h", "cmd.h.pch", "cmd.h.d", std::string());
			else
				options = m_toolchain->compilerCxx->getCommand("cmd.cxx", "cmd.cxx.o", "cmd.cxx.d", derivative);

			auto hash = Hash::string(String::join(options));
			m_commandsChanged[type] = sourceCache.dataCacheValueChanged(cxxHashKey, hash);
		}

		auto targetHashKey = Hash::string(fmt::format("{}_target", name));
		auto targetOptions = m_toolchain->getOutputTargetCommand(m_project->outputFile(), m_project->files());
		auto targetHash = Hash::string(String::join(targetOptions));
		m_targetCommandChanged = sourceCache.dataCacheValueChanged(targetHashKey, targetHash);

		if (m_targetCommandChanged)
		{
			m_compileAdapter.addChangedTarget(*m_project);
		}
	}
}
}
