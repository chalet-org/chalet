/*
	 Distributed under the OSI-approved BSD 3-Clause License.
	 See accompanying file LICENSE.txt for details.
 */

#include "Compile/Linker/LinkerEmscripten.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerEmscripten::LinkerEmscripten(const BuildState& inState, const SourceTarget& inProject) :
	LinkerLLVMClang(inState, inProject)
{
}

/*****************************************************************************/
bool LinkerEmscripten::addExecutable(StringList& outArgList) const
{
	auto& executable = m_state.toolchain.compilerCxx(m_project.language()).path;
	if (executable.empty())
		return false;

	auto& python = m_state.environment->commandInvoker();

	outArgList.emplace_back(getQuotedPath(python));
	outArgList.emplace_back(getQuotedPath(executable));

	return true;
}

/*****************************************************************************/
void LinkerEmscripten::addLinks(StringList& outArgList) const
{
	LinkerLLVMClang::addLinks(outArgList);

	const auto& sharedLinks = m_project.projectSharedLinks();
	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			const auto& project = static_cast<const SourceTarget&>(*target);
			if (project.isSharedLibrary())
			{
				if (List::contains(sharedLinks, project.name()))
				{
					outArgList.emplace_back(m_state.paths.getTargetFilename(project));
				}
			}
		}
	}
}

/*****************************************************************************/
void LinkerEmscripten::addRunPath(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerEmscripten::addLinkerOptions(StringList& outArgList) const
{
	LinkerLLVMClang::addLinkerOptions(outArgList);

	if (m_state.configuration.debugSymbols())
	{
		List::addIfDoesNotExist(outArgList, "-gsource-map");
		// List::addIfDoesNotExist(outArgList, "-gseparate-dwarf");
		// List::addIfDoesNotExist(outArgList, "-gsplit-dwarf");

		// std::string sourceMapBase("--source-map-base");
		// if (!List::contains(outArgList, sourceMapBase))
		// {
		// 	outArgList.emplace_back(sourceMapBase);
		// 	outArgList.emplace_back("http://127.0.0.1:6931/");
		// }
	}

	List::addIfDoesNotExist(outArgList, "--emit-symbol-map");
	// List::addIfDoesNotExist(outArgList, "--emrun");
}

/*****************************************************************************/
void LinkerEmscripten::addThreadModelLinks(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerEmscripten::addSharedOption(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, "-sSIDE_MODULE");
}

/*****************************************************************************/
void LinkerEmscripten::addExecutableOption(StringList& outArgList) const
{
	if (!m_project.projectSharedLinks().empty())
		List::addIfDoesNotExist(outArgList, "-sMAIN_MODULE");
	// UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerEmscripten::addPositionIndependentCodeOption(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, "-fPIC");
}
}
