/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/NativeCompileAdapter.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/SourceOutputs.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
NativeCompileAdapter::NativeCompileAdapter(const BuildState& inState) :
	m_state(inState),
	m_sourceCache(inState.cache.file().sources())
{
}

/*****************************************************************************/
void NativeCompileAdapter::addChangedTarget(const SourceTarget& inProject)
{
	List::addIfDoesNotExist(m_targetsChanged, inProject.name());
}

/*****************************************************************************/
bool NativeCompileAdapter::checkDependentTargets(const SourceTarget& inProject) const
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

/*****************************************************************************/
bool NativeCompileAdapter::anyCmakeOrSubChaletTargetsChanged() const
{
	// Note: At the moment, this forces any sources targets to re-link if the below returns true
	//  In the future, it would be better to figure out which libraries are where
	//
	for (auto& target : m_state.targets)
	{
		if (target->isCMake())
		{
			auto& project = static_cast<const CMakeTarget&>(*target);
			if (project.hashChanged())
				return true;
		}
		else if (target->isSubChalet())
		{
			auto& project = static_cast<const SubChaletTarget&>(*target);
			if (project.hashChanged())
				return true;
		}
	}

	return false;
}

/*****************************************************************************/
void NativeCompileAdapter::setDependencyCacheSize(const size_t inSize)
{
	m_dependencyCache.reserve(inSize);
}

/*****************************************************************************/
void NativeCompileAdapter::clearDependencyCache()
{
	m_dependencyCache.clear();
}

/*****************************************************************************/
bool NativeCompileAdapter::fileChangedOrDependentChanged(const std::string& source, const std::string& target, const std::string& dependency)
{
	// Check the source file and target (object) if they were changed
	bool result = m_sourceCache.fileChangedOrDoesNotExist(source, target);
	if (result)
		return true;

	// Read through all the dependencies
	if (!dependency.empty() && Files::pathExists(dependency))
	{
		std::ifstream input(dependency);
		for (std::string line; std::getline(input, line);)
		{
			if (line.empty())
				continue;

			if (line.back() != ':')
				continue;

			line.pop_back();

			// The file didn't change if it's cached, so skip it
			if (m_dependencyCache.find(line) != m_dependencyCache.end())
				continue;

			if (m_sourceCache.fileChangedOrDoesNotExist(line))
				return true;

			// Cache the filename if it didn't change
			m_dependencyCache.emplace(line, true);
		}
	}

	return false;
}

/*****************************************************************************/
CommandPool::Settings NativeCompileAdapter::getCommandPoolSettings() const
{
	CommandPool::Settings settings;
	settings.color = Output::theme().build;
	settings.msvcCommand = m_state.environment->isMsvc();
	settings.keepGoing = m_state.info.keepGoing();
	settings.showCommands = Output::showCommands();
	settings.quiet = Output::quietNonBuild();

	return settings;
}

/*****************************************************************************/
CommandPool::CmdList NativeCompileAdapter::getLinkCommand(const SourceTarget& inProject, CompileToolchainController& inToolchain, const SourceOutputs& inOutputs) const
{
	CommandPool::CmdList ret;

	{
		CommandPool::Cmd out;
		out.command = inToolchain.getOutputTargetCommand(inOutputs.target, inOutputs.objectListLinker);

		auto label = inProject.isStaticLibrary() ? "Archiving" : "Linking";
		out.output = fmt::format("{} {}", label, inOutputs.target);

		ret.emplace_back(std::move(out));
	}

	return ret;
}

}
