/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/BuildEnvironmentVisualStudioLLVM.hpp"

#include "BuildEnvironment/Script/VisualStudioEnvironmentScript.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
BuildEnvironmentVisualStudioLLVM::BuildEnvironmentVisualStudioLLVM(const ToolchainType inType, BuildState& inState) :
	BuildEnvironmentLLVM(inType, inState)
{
}

/*****************************************************************************/
BuildEnvironmentVisualStudioLLVM::~BuildEnvironmentVisualStudioLLVM() = default;

/*****************************************************************************/
bool BuildEnvironmentVisualStudioLLVM::validateArchitectureFromInput()
{
	std::string host;
	std::string target;

	m_config = std::make_unique<VisualStudioEnvironmentScript>();
	if (!m_config->validateArchitectureFromInput(m_state, host, target))
		return false;

	m_isWindowsTarget = true;

	// TODO: universal windows platform - uwp-windows-msvc

	return BuildEnvironmentLLVM::validateArchitectureFromInput();
}

/*****************************************************************************/
bool BuildEnvironmentVisualStudioLLVM::createFromVersion(const std::string& inVersion)
{
	if (!VisualStudioEnvironmentScript::visualStudioExists())
		return true;

	Timer timer;

	m_config->setVersion(inVersion, m_state.inputs.visualStudioVersion());

	m_config->setEnvVarsFileBefore(m_state.cache.getHashPath(fmt::format("{}_original.env", this->identifier()), CacheType::Local));
	m_config->setEnvVarsFileAfter(m_state.cache.getHashPath(fmt::format("{}_all.env", this->identifier()), CacheType::Local));
	m_config->setEnvVarsFileDelta(getVarsPath(m_config->detectedVersion()));

	if (m_config->envVarsFileDeltaExists())
		Diagnostic::infoEllipsis("Reading Microsoft{} Visual C/C++ Environment Cache", Unicode::registered());
	else
		Diagnostic::infoEllipsis("Creating Microsoft{} Visual C/C++ Environment Cache", Unicode::registered());

	if (!m_config->makeEnvironment(m_state))
		return false;

	m_detectedVersion = m_config->detectedVersion();
	m_config->readEnvironmentVariablesFromDeltaFile();

	// This is a hack to enforce the VS LLVM path in PATH to avoid clashing with other LLVMs that might be in path
	{
		auto path = Environment::getPath();
		std::string vsLLVM;

		auto cl = Commands::which("cl");
		if (!cl.empty())
		{
			auto find = cl.find("/VC/Tools");
			if (find != std::string::npos)
			{
				vsLLVM = cl.substr(0, find + 9);
				vsLLVM += "/Llvm";
			}
		}

		if (!vsLLVM.empty())
		{
			auto pathA = fmt::format("{}/x64/bin;", vsLLVM);
			auto pathB = fmt::format("{}/bin;", vsLLVM);
			String::replaceAll(path, pathA, "");
			String::replaceAll(path, pathB, "");

			path = fmt::format("{}{}{}", pathA, pathB, path);
			Environment::setPath(path);
		}
	}
	m_state.cache.file().addExtraHash(String::getPathFilename(m_config->envVarsFileDelta()));

	m_config.reset(); // No longer needed

	Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
std::vector<CompilerPathStructure> BuildEnvironmentVisualStudioLLVM::getValidCompilerPaths() const
{
	std::vector<CompilerPathStructure> ret;

	std::string include;
	bool found = false;

	auto cl = Commands::which("cl");
	if (!cl.empty())
	{
		cl = String::toLowerCase(cl);
		auto find = cl.find("/vc/tools");
		if (find != std::string::npos)
		{
			include = cl.substr(find + 9);
			find = include.find("/bin");
			if (find != std::string::npos)
			{
				include = include.substr(0, find);
				include += "/include";
				found = true;
			}
		}
	}

	if (!found)
		include = "/llvm/include";

#if defined(CHALET_WIN32)
	ret.push_back({ "/llvm/x64/bin", "/llvm/x64/lib", include });
	ret.push_back({ "/llvm/bin", "/llvm/lib", include });
#endif

	return ret;
}

/*****************************************************************************/
std::string BuildEnvironmentVisualStudioLLVM::getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const
{
	UNUSED(inPath);
	const auto& vsVersion = m_detectedVersion.empty() ? m_state.toolchain.version() : m_detectedVersion;
	return fmt::format("LLVM Clang version {} (VS {})", inVersion, vsVersion);
}

/*****************************************************************************/
ToolchainType BuildEnvironmentVisualStudioLLVM::getToolchainTypeFromMacros(const std::string& inMacros) const
{
	auto llvmType = BuildEnvironmentLLVM::getToolchainTypeFromMacros(inMacros);
	if (llvmType != ToolchainType::LLVM)
		return llvmType;

	return ToolchainType::VisualStudioLLVM;
}

/*****************************************************************************/
std::string BuildEnvironmentVisualStudioLLVM::getObjectFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.obj", m_state.paths.objDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

}
