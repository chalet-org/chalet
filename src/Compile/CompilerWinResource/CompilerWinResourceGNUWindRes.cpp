/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerWinResource/CompilerWinResourceGNUWindRes.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
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
StringList CompilerWinResourceGNUWindRes::getCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency)
{
	StringList ret;

	if (!m_state.toolchain.canCompileWindowsResources())
		return ret;

	ret.emplace_back(getQuotedPath(m_state.toolchain.compilerWindowsResource()));

	ret.emplace_back("-J");
	ret.emplace_back("rc");

	ret.emplace_back("-O");
	ret.emplace_back("coff");

	if (generateDependencies() && !dependency.empty())
	{
		// Note: The dependency generation args have to be passed into the preprocessor
		//   The underlying preprocessor command is "gcc -E -xc-header -DRC_INVOKED"
		//   This runs in C mode, so we don't want any c++ flags passed in
		//   See: https://sourceware.org/binutils/docs/binutils/windres.html

		ret.emplace_back("--preprocessor-arg=-MT");
		ret.emplace_back(fmt::format("--preprocessor-arg={}", getQuotedPath(outputFile)));
		ret.emplace_back("--preprocessor-arg=-MMD");
		ret.emplace_back("--preprocessor-arg=-MP");
		ret.emplace_back("--preprocessor-arg=-MF");
		ret.emplace_back(fmt::format("--preprocessor-arg={}", getQuotedPath(dependency)));
	}

	addDefines(ret);
	addIncludes(ret);

	ret.emplace_back("-i");
	ret.emplace_back(getQuotedPath(inputFile));

	ret.emplace_back("-o");
	ret.emplace_back(getQuotedPath(outputFile));

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

	if (m_project.usesPrecompiledHeader())
	{
		auto outDir = String::getPathFolder(m_project.precompiledHeader());
		List::addIfDoesNotExist(outArgList, getPathCommand(prefix, outDir));
	}
}

/*****************************************************************************/
void CompilerWinResourceGNUWindRes::addDefines(StringList& outArgList) const
{
	addDefinesToList(outArgList, "-D");
}

}
