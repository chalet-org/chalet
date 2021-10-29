/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerWinResource/CompilerWinResourceVisualStudioRC.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerWinResourceVisualStudioRC::CompilerWinResourceVisualStudioRC(const BuildState& inState, const SourceTarget& inProject) :
	ICompilerWinResource(inState, inProject)
{
}

/*****************************************************************************/
StringList CompilerWinResourceVisualStudioRC::getCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency)
{
	UNUSED(generateDependency, dependency);

	StringList ret;

	if (m_state.toolchain.compilerWindowsResource().empty())
		return ret;

	ret.emplace_back(getQuotedExecutablePath(m_state.toolchain.compilerWindowsResource()));
	ret.emplace_back("/nologo");

	addDefines(ret);
	addIncludes(ret);

	ret.emplace_back(getPathCommand("/Fo", outputFile));

	ret.push_back(inputFile);

	return ret;
}

/*****************************************************************************/
void CompilerWinResourceVisualStudioRC::addIncludes(StringList& outArgList) const
{
	// outArgList.emplace_back("/X"); // ignore "Path"

	const std::string option{ "/I" };

	for (const auto& dir : m_project.includeDirs())
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();

		outArgList.emplace_back(getPathCommand(option, outDir));
	}

	for (const auto& dir : m_project.locations())
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();

		outArgList.emplace_back(getPathCommand(option, outDir));
	}

	if (m_project.usesPch())
	{
		auto outDir = String::getPathFolder(m_project.pch());
		List::addIfDoesNotExist(outArgList, getPathCommand(option, outDir));
	}
}

/*****************************************************************************/
void CompilerWinResourceVisualStudioRC::addDefines(StringList& outArgList) const
{
	const std::string prefix = "/D";
	for (auto& define : m_project.defines())
	{
		outArgList.emplace_back(prefix + define);
	}
}

}
