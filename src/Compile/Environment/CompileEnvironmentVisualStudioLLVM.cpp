/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentVisualStudioLLVM.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Compile/Environment/VisualStudioEnvironmentScript.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironmentVisualStudioLLVM::CompileEnvironmentVisualStudioLLVM(const ToolchainType inType, BuildState& inState) :
	CompileEnvironmentLLVM(inType, inState)
{
}

/*****************************************************************************/
CompileEnvironmentVisualStudioLLVM::~CompileEnvironmentVisualStudioLLVM() = default;

/*****************************************************************************/
bool CompileEnvironmentVisualStudioLLVM::validateArchitectureFromInput()
{
	auto gnuArchToMsvcArch = [](const std::string& inArch) -> std::string {
		if (String::equals("x86_64", inArch))
			return "x64";
		else if (String::equals("i686", inArch))
			return "x86";
		else if (String::equals("aarch64", inArch))
			return "arm64";

		return inArch;
	};

	auto splitHostTarget = [](std::string& outHost, std::string& outTarget) -> void {
		if (!String::contains('_', outTarget))
			return;

		auto split = String::split(outTarget, '_');

		if (outHost.empty())
			outHost = split.front();

		outTarget = split.back();
	};

	std::string host;
	std::string target = gnuArchToMsvcArch(m_state.inputs.targetArchitecture());

	const auto& compiler = m_state.toolchain.compilerCxxAny().path;
	if (!compiler.empty())
	{
		std::string lower = String::toLowerCase(compiler);
		auto search = lower.find("/bin/host");
		if (search == std::string::npos)
		{
			Diagnostic::error("MSVC Host architecture was not detected in compiler path: {}", compiler);
			return false;
		}

		auto nextPath = lower.find('/', search + 5);
		if (search == std::string::npos)
		{
			Diagnostic::error("MSVC Host architecture was not detected in compiler path: {}", compiler);
			return false;
		}

		search += 9;
		std::string hostFromCompilerPath = lower.substr(search, nextPath - search);
		search = nextPath + 1;
		nextPath = lower.find('/', search);
		if (search == std::string::npos)
		{
			Diagnostic::error("MSVC Target architecture was not detected in compiler path: {}", compiler);
			return false;
		}

		splitHostTarget(host, target);
		if (host.empty())
			host = hostFromCompilerPath;

		std::string targetFromCompilerPath = lower.substr(search, nextPath - search);
		if (target.empty() || (target == targetFromCompilerPath && host == hostFromCompilerPath))
		{
			target = lower.substr(search, nextPath - search);
		}
		else
		{
			const auto& preferenceName = m_state.inputs.toolchainPreferenceName();
			Diagnostic::error("Expected host '{}' and target '{}'. Please use a different toolchain or create a new one for this architecture.", hostFromCompilerPath, targetFromCompilerPath);
			Diagnostic::error("Architecture '{}' is not supported by the '{}' toolchain.", m_state.inputs.targetArchitecture(), preferenceName);
			return false;
		}
	}
	else
	{
		if (target.empty())
			target = gnuArchToMsvcArch(m_state.inputs.hostArchitecture());

		splitHostTarget(host, target);

		if (host.empty())
			host = gnuArchToMsvcArch(m_state.inputs.hostArchitecture());
	}

	m_config = std::make_unique<VisualStudioEnvironmentScript>();
	m_config->setArchitecture(host, target, m_state.inputs.archOptions());

	m_isWindowsTarget = true;

	// TODO: universal windows platform - uwp-windows-msvc

	return CompileEnvironmentLLVM::validateArchitectureFromInput();
}
/*****************************************************************************/
bool CompileEnvironmentVisualStudioLLVM::createFromVersion(const std::string& inVersion)
{
	if (!VisualStudioEnvironmentScript::visualStudioExists())
		return true;

	Timer timer;

	m_config->setVersion(inVersion, m_state.inputs.visualStudioVersion());

	m_config->setEnvVarsFileBefore(m_state.cache.getHashPath(fmt::format("{}_original.env", this->identifier()), CacheType::Local));
	m_config->setEnvVarsFileAfter(m_state.cache.getHashPath(fmt::format("{}_all.env", this->identifier()), CacheType::Local));
	m_config->setEnvVarsFileDelta(getVarsPath(m_config->detectedVersion()));

	m_ouptuttedDescription = true;

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
std::vector<CompilerPathStructure> CompileEnvironmentVisualStudioLLVM::getValidCompilerPaths() const
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
std::string CompileEnvironmentVisualStudioLLVM::getFullCxxCompilerString(const std::string& inVersion) const
{
	const auto& vsVersion = m_detectedVersion.empty() ? m_state.toolchain.version() : m_detectedVersion;
	return fmt::format("LLVM Clang version {} (VS {})", inVersion, vsVersion);
}

/*****************************************************************************/
ToolchainType CompileEnvironmentVisualStudioLLVM::getToolchainTypeFromMacros(const std::string& inMacros) const
{
	auto llvmType = CompileEnvironmentLLVM::getToolchainTypeFromMacros(inMacros);
	if (llvmType != ToolchainType::LLVM)
		return llvmType;

	return ToolchainType::VisualStudioLLVM;
}

/*****************************************************************************/
std::string CompileEnvironmentVisualStudioLLVM::getObjectFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.obj", m_state.paths.objDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

}
