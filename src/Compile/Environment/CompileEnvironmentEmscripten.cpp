/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentEmscripten.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Compile/Environment/EmscriptenEnvironmentScript.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironmentEmscripten::CompileEnvironmentEmscripten(const ToolchainType inType, BuildState& inState) :
	CompileEnvironmentLLVM(inType, inState)
{
}

/*****************************************************************************/
CompileEnvironmentEmscripten::~CompileEnvironmentEmscripten() = default;

/*****************************************************************************/
StringList CompileEnvironmentEmscripten::getVersionCommand(const std::string& inExecutable) const
{
	if (String::endsWith("emcc.py", inExecutable))
	{
		return StringList{ m_commandInvoker, inExecutable, "--version" };
	}
	else
	{
		return CompileEnvironmentLLVM::getVersionCommand(inExecutable);
	}
}

/*****************************************************************************/
std::string CompileEnvironmentEmscripten::getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const
{
	UNUSED(inPath);
	return fmt::format("Emscripten version {} (Based on LLVM Clang {})", m_emccVersion, inVersion);
}

/*****************************************************************************/
bool CompileEnvironmentEmscripten::createFromVersion(const std::string& inVersion)
{
	UNUSED(inVersion);

	Timer timer;

	m_config = std::make_unique<EmscriptenEnvironmentScript>();

	m_config->setEnvVarsFileBefore(m_state.cache.getHashPath(fmt::format("{}_original.env", this->identifier()), CacheType::Local));
	m_config->setEnvVarsFileAfter(m_state.cache.getHashPath(fmt::format("{}_all.env", this->identifier()), CacheType::Local));
	m_config->setEnvVarsFileDelta(getVarsPath("0"));

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
bool CompileEnvironmentEmscripten::validateArchitectureFromInput()
{
	return true;
}

/*****************************************************************************/
bool CompileEnvironmentEmscripten::readArchitectureTripleFromCompiler()
{
	const auto& compiler = m_state.toolchain.compilerCxxAny().path;

	if (compiler.empty())
		return false;

	auto& sourceCache = m_state.cache.file().sources();
	std::string cachedArch;
	if (sourceCache.archRequriesUpdate(compiler, cachedArch))
	{
		auto& targetArch = m_state.inputs.targetArchitecture();
		if (targetArch.empty())
		{
			cachedArch = Commands::subprocessOutput({ compiler, "-dumpmachine" });
			if (!String::equals("wasm32", cachedArch))
				return false;

			cachedArch = getToolchainTriple(cachedArch);
		}
		else
		{
			if (!String::equals("wasm32", targetArch))
				return false;

			cachedArch = getToolchainTriple(targetArch);
		}
	}

	if (cachedArch.empty())
		return false;

	m_state.info.setTargetArchitecture(cachedArch);
	sourceCache.addArch(compiler, cachedArch);

	return true;
}

/*****************************************************************************/
bool CompileEnvironmentEmscripten::getCompilerVersionAndDescription(CompilerInfo& outInfo) const
{
	CompilerInfo info = outInfo;
	bool result = CompileEnvironmentLLVM::getCompilerVersionAndDescription(info);
	if (!result)
		return false;

	outInfo.version = info.version;

	auto& sourceCache = m_state.cache.file().sources();
	std::string cachedVersion;
	if (sourceCache.versionRequriesUpdate(m_emcc, cachedVersion))
	{
		{
			//
			auto userPath = Environment::getUserDirectory();
			auto configFile = fmt::format("{}/.emscripten", userPath);
			// auto configFile = fmt::format("{}/.emscripten", m_emsdkUpstream);

			// if (!Commands::pathExists(configFile))
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

				if (!Commands::createFileWithContents(configFile, configContents))
					return false;
			}
		}

		// Expects:
		// emcc (Emscripten gcc/clang-like replacement + linker emulating GNU ld) 3.1.47 (431685f05c67f0424c11473cc16798b9587bb536)

		auto rawOutput = Commands::subprocessOutput(getVersionCommand(m_emcc));

		StringList splitOutput;
		splitOutput = String::split(rawOutput, '\n');

		if (splitOutput.size() >= 2)
		{
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

			cachedVersion = std::move(version);
		}
	}

	if (!cachedVersion.empty())
	{
		outInfo.path = m_emcc;
		m_emccVersion = std::move(cachedVersion);

		sourceCache.addVersion(m_emcc, m_emccVersion);

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
std::vector<CompilerPathStructure> CompileEnvironmentEmscripten::getValidCompilerPaths() const
{
	std::vector<CompilerPathStructure> ret = CompileEnvironmentGNU::getValidCompilerPaths();

	auto arch = m_state.info.targetArchitecture();

	if (arch == Arch::Cpu::WASM32)
	{
		ret.push_back({ "/bin", "/lib", "/emscripten/cache/sysroot/include" });
	}

	return ret;
}

/*****************************************************************************/
ToolchainType CompileEnvironmentEmscripten::getToolchainTypeFromMacros(const std::string& inMacros) const
{
	auto llvmType = CompileEnvironmentLLVM::getToolchainTypeFromMacros(inMacros);
	if (llvmType != ToolchainType::LLVM)
		return llvmType;

	return ToolchainType::Emscripten;
}

/*****************************************************************************/
std::string CompileEnvironmentEmscripten::getToolchainTriple(const std::string& inArch) const
{
	return fmt::format("{}-unknown-emscripten", inArch);
}

/*****************************************************************************/
std::string CompileEnvironmentEmscripten::getAssemblyFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.o.wat", m_state.paths.asmDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

}