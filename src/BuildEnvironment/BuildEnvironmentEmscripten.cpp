/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/BuildEnvironmentEmscripten.hpp"

#include "BuildEnvironment/Script/EmscriptenEnvironmentScript.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
BuildEnvironmentEmscripten::BuildEnvironmentEmscripten(const ToolchainType inType, BuildState& inState) :
	BuildEnvironmentLLVM(inType, inState)
{
}

/*****************************************************************************/
BuildEnvironmentEmscripten::~BuildEnvironmentEmscripten() = default;

/*****************************************************************************/
std::string BuildEnvironmentEmscripten::getExecutableExtension() const
{
	return ".html";
}

/*****************************************************************************/
std::string BuildEnvironmentEmscripten::getSharedLibraryExtension() const
{
	return ".wasm";
}

/*****************************************************************************/
std::string BuildEnvironmentEmscripten::getArchiveExtension() const
{
	return BuildEnvironmentGNU::getArchiveExtension();
}

/*****************************************************************************/
std::string BuildEnvironmentEmscripten::getSystemIncludeOutputFromClang(const std::string& inExecutable, const std::string& inTempFile, const std::string& inSystemDirsFile)
{
	UNUSED(inExecutable, inSystemDirsFile);

	chalet_assert(!m_clangPath.empty(), "");
	if (m_clangPath.empty())
		return std::string();

	return Process::runOutput({ m_clangPath, "-E", "-x", "c++", "-v", inTempFile });
}

/*****************************************************************************/
StringList BuildEnvironmentEmscripten::getVersionCommand(const std::string& inExecutable) const
{
	if (String::endsWith("emcc.py", inExecutable))
	{
		return StringList{ m_commandInvoker, inExecutable, "--version" };
	}
	else
	{
		return BuildEnvironmentLLVM::getVersionCommand(inExecutable);
	}
}

/*****************************************************************************/
std::string BuildEnvironmentEmscripten::getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const
{
	UNUSED(inPath);
	return fmt::format("Emscripten version {} (Based on LLVM Clang {})", m_emccVersion, inVersion);
}

/*****************************************************************************/
bool BuildEnvironmentEmscripten::createFromVersion(const std::string& inVersion)
{
	UNUSED(inVersion);

	Timer timer;

	m_config = std::make_unique<EmscriptenEnvironmentScript>();

	m_config->setEnvVarsFileBefore(getCachePath("original.env"));
	m_config->setEnvVarsFileAfter(getCachePath("all.env"));
	m_config->setEnvVarsFileDelta(getVarsPath());

	if (m_config->envVarsFileDeltaExists())
		Diagnostic::infoEllipsis("Reading Emscripten C/C++ Environment Cache");
	else
		Diagnostic::infoEllipsis("Creating Emscripten C/C++ Environment Cache");

	if (!m_config->makeEnvironment(m_state))
		return false;

	m_config->readEnvironmentVariablesFromDeltaFile();

	m_state.cache.file().addExtraHash(String::getPathFilename(m_config->envVarsFileDelta()));

	m_config.reset(); // No longer needed

	m_emsdkRoot = Environment::getString("EMSDK");
	m_commandInvoker = Environment::getString("EMSDK_PYTHON");
	m_emsdkUpstream = Environment::getString("EMSDK_UPSTREAM_EMSCRIPTEN");
	m_emcc = fmt::format("{}/emcc.py", m_emsdkUpstream);

	Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
bool BuildEnvironmentEmscripten::validateArchitectureFromInput()
{
	return true;
}

/*****************************************************************************/
bool BuildEnvironmentEmscripten::readArchitectureTripleFromCompiler()
{
	const auto& compiler = m_state.toolchain.compilerCxxAny().path;

	if (compiler.empty())
		return false;

	std::string cachedArch;
	getDataWithCache(cachedArch, "arch", compiler, [this, &compiler]() {
		auto& targetArch = m_state.inputs.targetArchitecture();
		std::string outArch;
		if (targetArch.empty())
		{
			outArch = Process::runOutput({ compiler, "-dumpmachine" });
			if (!String::equals("wasm32", outArch))
				return std::string();

			outArch = getToolchainTriple(outArch);
		}
		else
		{
			if (!String::equals("wasm32", targetArch))
				return std::string();

			outArch = getToolchainTriple(targetArch);
		}
		return outArch;
	});

	if (cachedArch.empty())
		return false;

	m_state.info.setTargetArchitecture(cachedArch);

	return true;
}

/*****************************************************************************/
bool BuildEnvironmentEmscripten::getCompilerVersionAndDescription(CompilerInfo& outInfo)
{
	CompilerInfo info = outInfo;
	bool result = BuildEnvironmentLLVM::getCompilerVersionAndDescription(info);
	if (!result)
		return false;

	m_clangPath = outInfo.path;
	outInfo.version = info.version;

	std::string cachedVersion;
	getDataWithCache(cachedVersion, "version", outInfo.path, [this]() {
		{
			auto userPath = Environment::getUserDirectory();
			auto configFile = fmt::format("{}/.emscripten", userPath);
			// auto configFile = fmt::format("{}/.emscripten", m_emsdkUpstream);

			// if (!Files::pathExists(configFile))
			{
				auto upstreamBin = Environment::getString("EMSDK_UPSTREAM_BIN");
				auto upstream = String::getPathFolder(upstreamBin);
				auto nodePath = Environment::getString("EMSDK_NODE");
				auto pythonPath = Environment::getString("EMSDK_PYTHON");
				auto javaPath = Environment::getString("EMSDK_JAVA");

				std::string configContents;
				configContents += fmt::format("NODE_JS = '{}'\n", nodePath);
				configContents += fmt::format("PYTHON = '{}'\n", pythonPath);
				configContents += fmt::format("JAVA = '{}'\n", javaPath);
				configContents += fmt::format("LLVM_ROOT = '{}'\n", upstreamBin);
				configContents += fmt::format("BINARYEN_ROOT = '{}'\n", upstream);
				configContents += fmt::format("EMSCRIPTEN_ROOT = '{}'\n", m_emsdkUpstream);
				configContents += "COMPILER_ENGINE = NODE_JS\n";
				configContents += "JS_ENGINES = [NODE_JS]";

				if (!Files::createFileWithContents(configFile, configContents))
					return std::string();
			}
		}

		// Expects:
		// emcc (Emscripten gcc/clang-like replacement + linker emulating GNU ld) 3.1.47 (431685f05c67f0424c11473cc16798b9587bb536)

		auto rawOutput = Process::runOutput(getVersionCommand(m_emcc));

		StringList splitOutput;
		splitOutput = String::split(rawOutput, '\n');

		if (splitOutput.size() < 2)
			return std::string();

		std::string version;
		for (auto& line : splitOutput)
		{
			auto start = line.find(") ");
			if (start != std::string::npos)
			{
				version = line.substr(start + 2);
				version = version.substr(0, version.find_first_not_of("0123456789."));
				if (!version.empty())
					break;
			}
		}

		return version;
	});

	if (!cachedVersion.empty())
	{
		m_emccVersion = std::move(cachedVersion);
		outInfo.path = m_emcc;
		outInfo.description = getFullCxxCompilerString(m_emcc, outInfo.version);
		return true;
	}
	else
	{
		outInfo.description = "Unrecognized";
		return false;
	}
}

/*****************************************************************************/
std::vector<CompilerPathStructure> BuildEnvironmentEmscripten::getValidCompilerPaths() const
{
	std::vector<CompilerPathStructure> ret = BuildEnvironmentGNU::getValidCompilerPaths();

	auto arch = m_state.info.targetArchitecture();

	if (arch == Arch::Cpu::WASM32)
	{
		ret.push_back({ "/bin", "/lib", "/emscripten/cache/sysroot/include" });
	}

	return ret;
}

/*****************************************************************************/
ToolchainType BuildEnvironmentEmscripten::getToolchainTypeFromMacros(const std::string& inMacros) const
{
	auto llvmType = BuildEnvironmentLLVM::getToolchainTypeFromMacros(inMacros);
	if (llvmType != ToolchainType::LLVM)
		return llvmType;

	return ToolchainType::Emscripten;
}

/*****************************************************************************/
std::string BuildEnvironmentEmscripten::getToolchainTriple(const std::string& inArch) const
{
	return fmt::format("{}-unknown-emscripten", inArch);
}

/*****************************************************************************/
std::string BuildEnvironmentEmscripten::getAssemblyFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.o.wat", m_state.paths.asmDir(), m_state.paths.getNormalizedOutputPath(inSource));
}
}