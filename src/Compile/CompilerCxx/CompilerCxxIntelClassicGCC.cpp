/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/CompilerCxxIntelClassicGCC.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerCxxIntelClassicGCC::CompilerCxxIntelClassicGCC(const BuildState& inState, const SourceTarget& inProject) :
	CompilerCxxGCC(inState, inProject)
{
}

/*****************************************************************************/
bool CompilerCxxIntelClassicGCC::initialize()
{
	if (!CompilerCxxGCC::initialize())
		return false;

	const auto& cxxExt = m_state.paths.cxxExtension();
	if (cxxExt.empty())
		return false;

	if (m_project.usesPrecompiledHeader())
	{
		const auto& objDir = m_state.paths.objDir();
		const auto& pch = m_project.precompiledHeader();
		m_pchSource = fmt::format("{}/{}.{}", objDir, pch, cxxExt);

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
StringList CompilerCxxIntelClassicGCC::getPrecompiledHeaderCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const std::string& arch)
{
	StringList ret;
	if (m_project.usesPrecompiledHeader())
	{
		ret = CompilerCxxGCC::getPrecompiledHeaderCommand(inputFile, outputFile, generateDependency, dependency, arch);
		ret.pop_back();
		ret.push_back(m_pchSource);
	}

	return ret;
}

/*****************************************************************************/
StringList CompilerCxxIntelClassicGCC::getWarningExclusions() const
{
	return {
		"pedantic",
		"cast-align",
		"double-promotion",
		"redundant-decls",
		"noexcept",
		"old-style-cast",
		"strict-null-sentinel",
		"invalid-pch"
	};
}

/*****************************************************************************/
void CompilerCxxIntelClassicGCC::addCharsets(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompilerCxxIntelClassicGCC::addFastMathOption(StringList& outArgList) const
{
	if (m_project.fastMath())
	{
		List::addIfDoesNotExist(outArgList, "-fp-model=fast");
	}
}

/*****************************************************************************/
void CompilerCxxIntelClassicGCC::addLinkTimeOptimizations(StringList& outArgList) const
{
	UNUSED(outArgList);
}

}
