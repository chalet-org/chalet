/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/BuildEnvironmentVisualStudio.hpp"

#include "BuildEnvironment/Script/VisualStudioEnvironmentScript.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Platform/Arch.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/Hash.hpp"
#include "Utility/Path.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
BuildEnvironmentVisualStudio::BuildEnvironmentVisualStudio(const ToolchainType inType, BuildState& inState) :
	IBuildEnvironment(inType, inState)
{
	m_isWindowsTarget = true;
}

/*****************************************************************************/
BuildEnvironmentVisualStudio::~BuildEnvironmentVisualStudio() = default;

/*****************************************************************************/
bool BuildEnvironmentVisualStudio::supportsCppModules() const
{
	auto& inputFile = m_state.inputs.inputFile();
	auto& compiler = m_state.toolchain.compilerCpp();
	u32 versionMajorMinor = compiler.versionMajorMinor;
	if (versionMajorMinor < 1928)
	{
		Diagnostic::error("{}: C++ modules are only supported with MSVC versions >= 19.28 (found {})", inputFile, compiler.version);
		return false;
	}

	return true;
}

/*****************************************************************************/
std::string BuildEnvironmentVisualStudio::getArchiveExtension() const
{
	return ".lib";
}

/*****************************************************************************/
std::string BuildEnvironmentVisualStudio::getPrecompiledHeaderExtension() const
{
	return ".pch";
}

/*****************************************************************************/
std::string BuildEnvironmentVisualStudio::getCompilerAliasForVisualStudio() const
{
	return "msvc";
}

/*****************************************************************************/
bool BuildEnvironmentVisualStudio::validateArchitectureFromInput()
{
	std::string host;
	std::string target;

	m_config = std::make_unique<VisualStudioEnvironmentScript>();
	if (!m_config->validateArchitectureFromInput(m_state, host, target))
		return false;

	// TODO: universal windows platform - uwp-windows-msvc

	m_state.info.setHostArchitecture(host);
	m_state.info.setTargetArchitecture(fmt::format("{}-pc-windows-msvc", Arch::toGnuArch(target)));

	return true;
}

/*****************************************************************************/
bool BuildEnvironmentVisualStudio::createFromVersion(const std::string& inVersion)
{
	if (!VisualStudioEnvironmentScript::visualStudioExists())
		return true;

	Timer timer;

	auto& toolchainName = m_state.inputs.toolchainPreferenceName();
	getDataWithCache(m_detectedVersion, "vsversion", toolchainName, [this]() {
		return m_config->getVisualStudioVersion(m_state.inputs.visualStudioVersion());
	});
	m_config->setVersion(m_detectedVersion, inVersion, m_state.inputs.visualStudioVersion());

	m_config->setEnvVarsFileBefore(getCachePath("original.env"));
	m_config->setEnvVarsFileAfter(getCachePath("all.env"));
	m_config->setEnvVarsFileDelta(getVarsPath(m_config->getEnvVarsHashKey()));

	if (m_config->envVarsFileDeltaExists())
		Diagnostic::infoEllipsis("Reading Microsoft{} Visual C/C++ Environment Cache", Unicode::registered());
	else
		Diagnostic::infoEllipsis("Creating Microsoft{} Visual C/C++ Environment Cache", Unicode::registered());

	if (!m_config->makeEnvironment(m_state))
		return false;

	m_detectedVersion = m_config->detectedVersion();

	m_config->readEnvironmentVariablesFromDeltaFile();

	// if (m_config->isPreset())
	// {
	// 	m_state.inputs.setToolchainPreferenceName(makeToolchainName(m_config->architecture()));
	// }

	m_state.cache.file().addExtraHash(String::getPathFilename(m_config->envVarsFileDelta()));

	m_config.reset(); // No longer needed

	Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
std::string BuildEnvironmentVisualStudio::makeToolchainName(const std::string& inArch) const
{
	std::string ret;
	if (!m_detectedVersion.empty())
	{
		auto versionSplit = String::split(m_detectedVersion, '.');
		if (versionSplit.size() >= 1)
		{
			chalet_assert(!inArch.empty(), "vcVarsAll arch was not set");

			ret = fmt::format("{}{}{}", inArch, m_state.info.targetArchitectureTripleSuffix(), versionSplit.front());
		}
	}
	return ret;
}

/*****************************************************************************/
StringList BuildEnvironmentVisualStudio::getVersionCommand(const std::string& inExecutable) const
{
	return { inExecutable };
}

/*****************************************************************************/
std::string BuildEnvironmentVisualStudio::getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const
{
	UNUSED(inPath);
	const auto& vsVersion = m_detectedVersion.empty() ? m_state.toolchain.version() : m_detectedVersion;
	return fmt::format("Microsoft{} Visual C/C++ version {} (VS {})", Unicode::registered(), inVersion, vsVersion);
}

/*****************************************************************************/
bool BuildEnvironmentVisualStudio::getCompilerVersionAndDescription(CompilerInfo& outInfo)
{
	Timer timer;

	std::string cachedVersion;
	getDataWithCache(cachedVersion, "version", outInfo.path, [this, &outInfo]() {
		// Microsoft (R) C/C++ Optimizing Compiler Version 19.28.29914 for x64
		auto rawOutput = Process::runOutput(getVersionCommand(outInfo.path));

		auto splitOutput = String::split(rawOutput, '\n');
		if (splitOutput.size() < 2)
			return std::string();

		// We loop through the words since this string could be a different locale
		//
		auto splitVersionLine = String::split(splitOutput[1], ' ');
		for (auto& line : splitVersionLine)
		{
			if (line.find_first_not_of("0123456789.") != std::string::npos)
				continue;

			return line;
		}

		return std::string();
	});

	if (!cachedVersion.empty())
	{
		outInfo.version = std::move(cachedVersion);
		outInfo.description = getFullCxxCompilerString(outInfo.path, outInfo.version);
		return true;
	}
	else
	{
		outInfo.description = "Unrecognized";
		return false;
	}
}

/*****************************************************************************/
std::vector<CompilerPathStructure> BuildEnvironmentVisualStudio::getValidCompilerPaths() const
{
	std::vector<CompilerPathStructure> ret;

#if defined(CHALET_WIN32)
	auto hostArch = m_state.info.hostArchitecture();
	auto arch = m_state.info.targetArchitecture();

	if (hostArch == Arch::Cpu::ARM64)
	{
		// Note: these are untested
		//   https://devblogs.microsoft.com/visualstudio/arm64-visual-studio

		if (arch == Arch::Cpu::ARM64)
			ret.push_back({ "/bin/hostarm64/arm64", "/lib/arm64", "/include" });
		else if (arch == Arch::Cpu::X64)
			ret.push_back({ "/bin/hostarm64/x64", "/lib/x64", "/include" });
		else if (arch == Arch::Cpu::X86)
			ret.push_back({ "/bin/hostarm64/x86", "/lib/x86", "/include" });
	}

	if (arch == Arch::Cpu::X64)
	{
		ret.push_back({ "/bin/hostx64/x64", "/lib/x64", "/include" });
		ret.push_back({ "/bin/hostx86/x64", "/lib/x64", "/include" });
	}
	else if (arch == Arch::Cpu::X86)
	{
		ret.push_back({ "/bin/hostx64/x86", "/lib/x86", "/include" });
		ret.push_back({ "/bin/hostx86/x86", "/lib/x86", "/include" });
	}
	else if (arch == Arch::Cpu::ARM64)
	{
		ret.push_back({ "/bin/hostx64/arm64", "/lib/arm64", "/include" });
		ret.push_back({ "/bin/hostx86/arm64", "/lib/arm64", "/include" });
	}
	else if (arch == Arch::Cpu::ARM)
	{
		ret.push_back({ "/bin/hostx64/arm", "/lib/arm", "/include" });
		ret.push_back({ "/bin/hostx86/arm", "/lib/arm", "/include" });
	}

	// ret.push_back({"/bin/hostx64/x64", "/lib/64", "/include"});
#endif

	return ret;
}

/*****************************************************************************/
bool BuildEnvironmentVisualStudio::verifyToolchain()
{
	return true;
}

/*****************************************************************************/
bool BuildEnvironmentVisualStudio::supportsFlagFile()
{
	return false;
}

/*****************************************************************************/
bool BuildEnvironmentVisualStudio::compilerVersionIsToolchainVersion() const
{
	return false;
}

/*****************************************************************************/
std::string BuildEnvironmentVisualStudio::getObjectFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.obj", m_state.paths.objDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
std::string BuildEnvironmentVisualStudio::getAssemblyFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.obj.asm", m_state.paths.asmDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
std::string BuildEnvironmentVisualStudio::getPrecompiledHeaderSourceFile(const SourceTarget& inProject) const
{
	auto pchName = String::getPathBaseName(inProject.precompiledHeader());
	return fmt::format("{}/{}.cxx", m_state.paths.intermediateDir(inProject), pchName);
}

/*****************************************************************************/
std::string BuildEnvironmentVisualStudio::getDependencyFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.d", m_state.paths.depDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
std::string BuildEnvironmentVisualStudio::getModuleDirectivesDependencyFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.module.json", m_state.paths.depDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
std::string BuildEnvironmentVisualStudio::getModuleBinaryInterfaceFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.ifc", m_state.paths.objDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
std::string BuildEnvironmentVisualStudio::getModuleBinaryInterfaceDependencyFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.ifc.d.json", m_state.paths.depDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
StringList BuildEnvironmentVisualStudio::getSystemIncludeDirectories(const std::string& inExecutable)
{
	UNUSED(inExecutable);

	StringList ret;

	auto toolsDirectory = Environment::getString("VCToolsInstallDir");
	if (!toolsDirectory.empty())
	{
		Path::toUnix(toolsDirectory);
		ret.emplace_back(toolsDirectory);
	}

	return ret;
}

}
