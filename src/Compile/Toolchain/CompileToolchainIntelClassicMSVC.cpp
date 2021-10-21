/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainIntelClassicMSVC.hpp"

#include "Compile/CompilerConfig.hpp"

#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileToolchainIntelClassicMSVC::CompileToolchainIntelClassicMSVC(const BuildState& inState, const SourceTarget& inProject, const CompilerConfig& inConfig) :
	CompileToolchainVisualStudio(inState, inProject, inConfig)
{
}

/*****************************************************************************/
ToolchainType CompileToolchainIntelClassicMSVC::type() const noexcept
{
	return ToolchainType::IntelClassic;
}

/*****************************************************************************/
bool CompileToolchainIntelClassicMSVC::initialize()
{
	if (!CompileToolchainVisualStudio::initialize())
		return false;

	if (m_project.usesPch())
	{
		const auto& objDir = m_state.paths.objDir();
		const auto& pch = m_project.pch();
		m_pchSource = fmt::format("{}/{}.cpp", objDir, pch);

		if (!Commands::pathExists(m_pchSource))
		{
			auto pchMinusLocation = String::getPathFilename(pch);
			if (!Commands::createFileWithContents(m_pchSource, fmt::format("#include \"{}\"", pchMinusLocation)))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
StringList CompileToolchainIntelClassicMSVC::getWarningExclusions() const
{
	return {};
}

/*****************************************************************************/
void CompileToolchainIntelClassicMSVC::addIncludes(StringList& outArgList) const
{
	outArgList.push_back("/X");

	CompileToolchainVisualStudio::addIncludes(outArgList);
}

/*****************************************************************************/
void CompileToolchainIntelClassicMSVC::addDiagnosticsOption(StringList& outArgList) const
{
	// CompileToolchainVisualStudio::addDiagnosticsOption(outArgList);
	UNUSED(outArgList);
}

}
