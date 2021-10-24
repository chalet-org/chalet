/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainIntelLLVM.hpp"

#include "Compile/CompilerConfig.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileToolchainIntelLLVM::CompileToolchainIntelLLVM(const BuildState& inState, const SourceTarget& inProject, const CompilerConfig& inConfig) :
	CompileToolchainLLVM(inState, inProject, inConfig)
{
}

/*****************************************************************************/
ToolchainType CompileToolchainIntelLLVM::type() const noexcept
{
	return ToolchainType::IntelLLVM;
}

/*****************************************************************************/
bool CompileToolchainIntelLLVM::initialize()
{
	if (!CompileToolchainLLVM::initialize())
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
StringList CompileToolchainIntelLLVM::getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency)
{
	if (m_state.toolchain.isCompilerWindowsResourceLLVMRC())
		return CompileToolchainLLVM::getRcCompileCommand(inputFile, outputFile, generateDependency, dependency);

	StringList ret;
	if (m_state.toolchain.compilerWindowsResource().empty())
		return ret;

	addExectuable(ret, m_state.toolchain.compilerWindowsResource());
	ret.emplace_back("/nologo");

	{
		// addDefines MSVC
		const std::string prefix = "/d";
		for (auto& define : m_project.defines())
		{
			ret.emplace_back(prefix + define);
		}
	}
	{
		// addIncludes MSVC
		const std::string option{ "/I" };

		for (const auto& dir : m_project.includeDirs())
		{
			std::string outDir = dir;
			if (String::endsWith('/', outDir))
				outDir.pop_back();

			ret.emplace_back(getPathCommand(option, outDir));
		}

		for (const auto& dir : m_project.locations())
		{
			std::string outDir = dir;
			if (String::endsWith('/', outDir))
				outDir.pop_back();

			ret.emplace_back(getPathCommand(option, outDir));
		}

		if (m_project.usesPch())
		{
			auto outDir = String::getPathFolder(m_project.pch());
			List::addIfDoesNotExist(ret, getPathCommand(option, outDir));
		}
	}

	ret.emplace_back(getPathCommand("/Fo", outputFile));

	ret.push_back(inputFile);

	return ret;
}

/*****************************************************************************/
void CompileToolchainIntelLLVM::addThreadModelLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

}
