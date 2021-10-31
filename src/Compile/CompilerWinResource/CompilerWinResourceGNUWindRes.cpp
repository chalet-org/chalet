/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerWinResource/CompilerWinResourceGNUWindRes.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerWinResourceGNUWindRes::CompilerWinResourceGNUWindRes(const BuildState& inState, const SourceTarget& inProject) :
	ICompilerWinResource(inState, inProject)
{
}

/*****************************************************************************/
StringList CompilerWinResourceGNUWindRes::getCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency)
{
	StringList ret;

	if (m_state.toolchain.compilerWindowsResource().empty())
		return ret;

	ret.emplace_back(getQuotedExecutablePath(m_state.toolchain.compilerWindowsResource()));

	ret.emplace_back("-J");
	ret.emplace_back("rc");

	ret.emplace_back("-O");
	ret.emplace_back("coff");

	if (generateDependency)
	{
		// Note: The dependency generation args have to be passed into the preprocessor
		//   The underlying preprocessor command is "gcc -E -xc-header -DRC_INVOKED"
		//   This runs in C mode, so we don't want any c++ flags passed in
		//   See: https://sourceware.org/binutils/docs/binutils/windres.html

		ret.emplace_back("--preprocessor-arg=-MT");
		ret.emplace_back(fmt::format("--preprocessor-arg={}", outputFile));
		ret.emplace_back("--preprocessor-arg=-MMD");
		ret.emplace_back("--preprocessor-arg=-MP");
		ret.emplace_back("--preprocessor-arg=-MF");
		ret.emplace_back(fmt::format("--preprocessor-arg={}", dependency));
	}

	addDefines(ret);
	addIncludes(ret);

	ret.emplace_back("-i");
	ret.push_back(inputFile);

	ret.emplace_back("-o");
	ret.push_back(outputFile);

	return ret;
}

/*****************************************************************************/
void CompilerWinResourceGNUWindRes::addIncludes(StringList& outArgList) const
{
	const std::string prefix{ "-I" };
	for (const auto& dir : m_project.includeDirs())
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();

		outArgList.emplace_back(getPathCommand(prefix, outDir));
	}
	for (const auto& dir : m_project.locations())
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();

		outArgList.emplace_back(getPathCommand(prefix, outDir));
	}

	if (m_project.usesPch())
	{
		auto outDir = String::getPathFolder(m_project.pch());
		List::addIfDoesNotExist(outArgList, getPathCommand(prefix, outDir));
	}

#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
	// must be last
	std::string localInclude{ "/usr/local/include/" };
	if (Commands::pathExists(localInclude))
		List::addIfDoesNotExist(outArgList, getPathCommand(prefix, localInclude));
#endif
}

/*****************************************************************************/
void CompilerWinResourceGNUWindRes::addDefines(StringList& outArgList) const
{
	const std::string prefix{ "-D" };
	for (auto& define : m_project.defines())
	{
		outArgList.emplace_back(prefix + define);
	}
}

}
