/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/BuildEnvironmentVisualStudio.hpp"

#include "BuildEnvironment/Script/VisualStudioEnvironmentScript.hpp"
#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Platform/Arch.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
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

	auto versionCache = getCachePath("vsversion.txt");
	m_config->setVersion(inVersion, m_state.inputs.visualStudioVersion(), versionCache);

	m_config->setEnvVarsFileBefore(getCachePath("original.env"));
	m_config->setEnvVarsFileAfter(getCachePath("all.env"));
	m_config->setEnvVarsFileDelta(getVarsPath(m_config->detectedVersion()));

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

	m_state.cache.file().addExtraHash(String::getPathFilename(versionCache));
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
bool BuildEnvironmentVisualStudio::getCompilerVersionAndDescription(CompilerInfo& outInfo) const
{
	Timer timer;

	auto& sourceCache = m_state.cache.file().sources();
	std::string cachedVersion;
	if (sourceCache.versionRequriesUpdate(outInfo.path, cachedVersion))
	{
		// Microsoft (R) C/C++ Optimizing Compiler Version 19.28.29914 for x64
		std::string rawOutput = Process::runOutput(getVersionCommand(outInfo.path));

		auto splitOutput = String::split(rawOutput, '\n');
		if (splitOutput.size() >= 2)
		{
			auto start = splitOutput[1].find("Version");
			auto end = splitOutput[1].find(" for ");
			if (start != std::string::npos && end != std::string::npos)
			{
				start += 8;
				auto version = splitOutput[1].substr(start, end - start); // cl.exe version
				version = version.substr(0, version.find_first_not_of("0123456789."));
				cachedVersion = std::move(version);

				// const auto arch = splitOutput[1].substr(end + 5);
			}
		}
	}

	if (!cachedVersion.empty())
	{
		outInfo.version = std::move(cachedVersion);

		sourceCache.addVersion(outInfo.path, outInfo.version);

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
	ret.push_back({ "/bin/hostx64/x64", "/lib/x64", "/include" });
	ret.push_back({ "/bin/hostx64/x86", "/lib/x86", "/include" });
	ret.push_back({ "/bin/hostx64/arm64", "/lib/arm64", "/include" });
	ret.push_back({ "/bin/hostx64/arm", "/lib/arm", "/include" });

	ret.push_back({ "/bin/hostx86/x86", "/lib/x86", "/include" });
	ret.push_back({ "/bin/hostx86/x64", "/lib/x64", "/include" });
	ret.push_back({ "/bin/hostx86/arm64", "/lib/arm64", "/include" });
	ret.push_back({ "/bin/hostx86/arm", "/lib/arm", "/include" });

	if (String::equals("arm64", m_state.inputs.hostArchitecture()))
	{
		// Note: these are untested
		//   https://devblogs.microsoft.com/visualstudio/arm64-visual-studio
		ret.push_back({ "/bin/hostarm64/arm64", "/lib/arm64", "/include" });
		ret.push_back({ "/bin/hostarm64/x64", "/lib/x64", "/include" });
		ret.push_back({ "/bin/hostarm64/x86", "/lib/x86", "/include" });
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

}
