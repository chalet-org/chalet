/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/CompilerCxxIntelClang.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerCxxIntelClang::CompilerCxxIntelClang(const BuildState& inState, const SourceTarget& inProject) :
	CompilerCxxClang(inState, inProject)
{
}

/*****************************************************************************/
bool CompilerCxxIntelClang::initialize()
{
	if (!CompilerCxxClang::initialize())
		return false;

	// TODO: needed?
	/*const auto& cxxExt = m_state.paths.cxxExtension();
	if (m_project.usesPch())
	{
		const auto& objDir = m_state.paths.objDir();
		const auto& pch = m_project.pch();
		m_pchSource = fmt::format("{}/{}.{}", objDir, pch, cxxExt);

		if (!Commands::pathExists(m_pchSource))
		{
			auto pchMinusLocation = String::getPathFilename(pch);
			if (!Commands::createFileWithContents(m_pchSource, fmt::format("#include \"{}\"", pchMinusLocation)))
				return false;
		}
	}*/

	return true;
}

}
