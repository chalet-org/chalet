/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyMSBuild.hpp"

#include "Export/IProjectExporter.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
/*****************************************************************************/
CompileStrategyMSBuild::CompileStrategyMSBuild(BuildState& inState) :
	ICompileStrategy(StrategyType::MSBuild, inState)
{
}

/*****************************************************************************/
bool CompileStrategyMSBuild::initialize()
{
	m_initialized = true;
	return true;
}

/*****************************************************************************/
bool CompileStrategyMSBuild::addProject(const SourceTarget& inProject)
{
	UNUSED(inProject);
	return true;
}

/*****************************************************************************/
bool CompileStrategyMSBuild::doFullBuild()
{
	LOG("Building via MSBuild...");
	return true;
}

/*****************************************************************************/
bool CompileStrategyMSBuild::buildProject(const SourceTarget& inProject)
{
	UNUSED(inProject);
	return true;
}

}
