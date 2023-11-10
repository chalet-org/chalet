/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyXcodeBuild.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Core/Arch.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/IProjectExporter.hpp"
#include "Export/XcodeProjectExporter.hpp"
#include "Process/Environment.hpp"
#include "Process/ProcessController.hpp"
#include "Process/ProcessOptions.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileStrategyXcodeBuild::CompileStrategyXcodeBuild(BuildState& inState) :
	ICompileStrategy(StrategyType::XcodeBuild, inState)
{
}

/*****************************************************************************/
bool CompileStrategyXcodeBuild::initialize()
{
	if (m_initialized)
		return false;

	auto& cacheFile = m_state.cache.file();
	const auto& cachePathId = m_state.cachePathId();
	UNUSED(m_state.cache.getCachePath(cachePathId, CacheType::Local));

	const bool buildStrategyChanged = cacheFile.buildStrategyChanged();
	if (buildStrategyChanged)
	{
		Commands::removeRecursively(m_state.paths.buildOutputDir());
	}

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool CompileStrategyXcodeBuild::addProject(const SourceTarget& inProject)
{
	return ICompileStrategy::addProject(inProject);
}

/*****************************************************************************/
bool CompileStrategyXcodeBuild::doFullBuild()
{
	// auto& route = m_state.inputs.route();
	auto& cwd = m_state.inputs.workingDirectory();

	auto xcodebuild = Commands::which("xcodebuild");
	if (xcodebuild.empty())
	{
		Diagnostic::error("Xcodebuild is required, but was not found in path.");
		return false;
	}

	auto platform = getPlatformName();
	if (platform.empty())
	{
		Diagnostic::error("OS Target is not supported by xcodebuild: {}", m_state.inputs.osTargetName());
		return false;
	}

	XcodeProjectExporter exporter(m_state.inputs);

	StringList cmd{
		xcodebuild,
		// "-verbose",
		"-hideShellScriptEnvironment"
	};

	if (Output::showCommands())
		cmd.emplace_back("-verbose");

	cmd.emplace_back("-configuration");
	cmd.emplace_back(m_state.configuration.name());

	cmd.emplace_back("-destination");
	cmd.emplace_back(fmt::format("platform={},arch={}", platform, m_state.info.targetArchitectureString()));

	if (m_state.inputs.route().isBundle())
	{
		// All targets, including bundles
		cmd.emplace_back("-alltargets");
	}
	else
	{
		auto& lastTarget = m_state.inputs.lastTarget();
		if (!lastTarget.empty())
		{
			cmd.emplace_back("-scheme");
			cmd.emplace_back(lastTarget);
		}

		// cmd.emplace_back("-target");
		// cmd.emplace_back(exporter.getAllBuildTargetName());
	}

	// std::string target;
	// if (route.isClean())
	// 	target = "Clean";
	// else if (route.isRebuild())
	// 	target = "Clean,Build";
	// else
	// 	target = "Build";

	// cmd.emplace_back(fmt::format("-target:{}", target));

	cmd.emplace_back("-jobs");
	cmd.emplace_back(std::to_string(m_state.info.maxJobs()));

	cmd.emplace_back("-parallelizeTargets");

	cmd.emplace_back("-project");

	auto project = exporter.getMainProjectOutput(m_state);
	cmd.emplace_back(project);

	if (!Output::showCommands())
		cmd.emplace_back("BUILD_FROM_CHALET=1");

	const auto& signingDevelopmentTeam = m_state.tools.signingDevelopmentTeam();
	if (!signingDevelopmentTeam.empty())
		cmd.emplace_back(fmt::format("DEVELOPMENT_TEAM={}", signingDevelopmentTeam));

	auto& signingCertificate = m_state.tools.signingCertificate();
	if (!signingCertificate.empty())
		cmd.emplace_back(fmt::format("CODE_SIGN_IDENTITY={}", signingCertificate));

	bool result = false;
	if (Output::showCommands())
	{
		result = Commands::subprocess(cmd);
	}
	else
	{
		result = subprocessXcodeBuild(cmd, cwd);
		if (result)
		{
			String::replaceAll(project, fmt::format("{}/", cwd), "");
			Output::msgAction("Succeeded", project);
		}
		else
		{
			Output::lineBreak();
		}
	}

	return result;
}

/*****************************************************************************/
bool CompileStrategyXcodeBuild::buildProject(const SourceTarget& inProject)
{
	UNUSED(inProject);
	return true;
}

/*****************************************************************************/
bool CompileStrategyXcodeBuild::subprocessXcodeBuild(const StringList& inCmd, std::string inCwd)
{
	if (Output::showCommands())
		Output::printCommand(inCmd);

	static auto getTargetName = [](const std::string& inLine) {
		auto lastParen = inLine.find_last_of('(');
		if (lastParen != std::string::npos)
		{
			auto substring = inLine.substr(lastParen);
			auto apost = substring.find('\'');
			if (apost != std::string::npos)
			{
				substring = substring.substr(apost + 1);
				apost = substring.find('\'');
				if (apost != std::string::npos)
				{
					return substring.substr(0, apost);
				}
			}
		}

		return std::string();
	};

	static auto getFilePath = [](const std::string& inLine) {
		auto firstSpace = inLine.find(' ');
		if (firstSpace != std::string::npos)
		{
			auto substring = inLine.substr(firstSpace + 1);
			auto nextSpace = substring.find(' ');
			if (nextSpace != std::string::npos)
			{
				substring = substring.substr(0, nextSpace);
				if (!String::startsWith('/', substring))
					substring.clear();
			}
			return substring;
		}

		return std::string();
	};

	static auto getTargetPath = [](const std::string& inLine) {
		auto firstSpace = inLine.find(' ');
		if (firstSpace != std::string::npos)
		{
			auto substring = inLine.substr(firstSpace + 1);
			auto lastParen = substring.find_last_of('(');
			if (lastParen != std::string::npos)
			{
				size_t lastSpace = std::string::npos;
				substring = substring.substr(0, lastParen - 1);
				while (!substring.empty())
				{
					lastSpace = substring.find_last_of(' ', lastSpace);
					if (lastSpace == std::string::npos)
						break;

					if (substring[lastSpace - 1] == '\\')
					{
						lastSpace--;
						continue;
					}

					auto next = substring.substr(lastSpace + 1);
					if (String::startsWith('/', next))
					{
						substring = std::move(next);
						break;
					}
					else
					{
						substring = substring.substr(0, lastSpace);
						lastSpace = std::string::npos;
					}
				}

				return substring;
			}
		}

		return std::string();
	};

	const auto color = Output::getAnsiStyle(Output::theme().build);
	const auto reset = Output::getAnsiStyle(Output::theme().reset);

	const auto cwd = fmt::format("{}/", inCwd);

	bool printed = false;
	bool errored = false;
	bool allowOutput = false;
	auto processLine = [&cwd, &printed, &allowOutput, &errored, &color, &reset](std::string& inLine) {
		UNUSED(inLine);
		UNUSED(getTargetName);

		if (String::startsWith("Compile", inLine))
		{
			auto path = getTargetPath(inLine);
			if (!path.empty())
			{
				// path = String::getPathFilename(path);
				String::replaceAll(path, cwd, "");
				if (!path.empty())
				{
					auto output = fmt::format("   {}{}{}\n", color, path, reset);
					std::cout.write(output.data(), output.size());
					std::cout.flush();
					printed = true;
				}
			}
		}
		else if (String::startsWith("Generate", inLine))
		{
			auto path = getFilePath(inLine);
			if (!path.empty())
			{
				// path = String::getPathFilename(path);
				String::replaceAll(path, cwd, "");
				if (!path.empty())
				{
					auto output = fmt::format("   {}Generating {}{}\n", color, path, reset);
					std::cout.write(output.data(), output.size());
					std::cout.flush();
					printed = true;
				}
			}
		}
		else if (String::startsWith("CodeSign", inLine))
		{
			auto path = getTargetPath(inLine);
			if (!path.empty())
			{
				// path = String::getPathFilename(path);
				String::replaceAll(path, cwd, "");
				if (!path.empty())
				{
					auto output = fmt::format("   {}Signing {}{}\n", color, path, reset);
					std::cout.write(output.data(), output.size());
					std::cout.flush();
					printed = true;
				}
			}
		}
		// Ld
		else if (String::startsWith("Ld ", inLine))
		{
			auto path = getTargetPath(inLine);
			if (!path.empty())
			{
				// path = String::getPathFilename(path);
				String::replaceAll(path, cwd, "");
				if (!path.empty())
				{
					auto output = fmt::format("   {}Linking {}{}\n", color, path, reset);
					std::cout.write(output.data(), output.size());
					std::cout.flush();
					printed = true;
				}
			}
		}
		// Libtool
		else if (String::startsWith("Libtool ", inLine))
		{
			auto path = getTargetPath(inLine);
			if (!path.empty())
			{
				// path = String::getPathFilename(path);
				String::replaceAll(path, cwd, "");
				if (!path.empty())
				{
					auto output = fmt::format("   {}Archiving {}{}\n", color, path, reset);
					std::cout.write(output.data(), output.size());
					std::cout.flush();
					printed = true;
				}
			}
		}
		// ProcessPCH
		else if (String::startsWith("ProcessPCH", inLine))
		{
			auto path = getTargetPath(inLine);
			if (!path.empty())
			{
				// path = String::getPathFilename(path);
				String::replaceAll(path, cwd, "");
				if (!path.empty())
				{
					auto output = fmt::format("   {}{}{}\n", color, path, reset);
					std::cout.write(output.data(), output.size());
					std::cout.flush();
					printed = true;
				}
			}
		}
		// PhaseScriptExecution
		else if (String::startsWith("*== script start ==*", inLine))
		{
			inLine.clear();
			allowOutput = true;
		}
		// WriteAuxiliaryFile
		else if (String::startsWith("*== script end ==*", inLine))
		{
			allowOutput = false;
		}
		else if (errored || String::contains("error:", inLine) || String::startsWith("ld:", inLine))
		{
			std::cout.write(inLine.data(), inLine.size());
			std::cout.flush();
			printed = true;
			errored = true;
		}
		// else
		// {
		// 	std::cout.write(inLine.data(), inLine.size());
		// 	std::cout.flush();
		// 	printed = true;
		// }
		if (allowOutput && !inLine.empty())
		{
			std::cout.write(inLine.data(), inLine.size());
			std::cout.flush();
		}
	};

	std::string data;
	std::string errors;

	ProcessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::Pipe;
	options.onStdErr = [&errors](std::string inData) -> void {
		errors += std::move(inData);
	};
	options.onStdOut = [&processLine, &data](std::string inData) -> void {
		auto lineBreak = inData.find('\n');
		if (lineBreak == std::string::npos)
		{
			data += std::move(inData);
		}
		else
		{
			while (lineBreak != std::string::npos)
			{
				data += inData.substr(0, lineBreak + 1);
				processLine(data);
				data.clear();

				inData = inData.substr(lineBreak + 1);
				lineBreak = inData.find('\n');
				if (lineBreak == std::string::npos)
				{
					data += std::move(inData);
				}
			}
		}
	};

	i32 result = ProcessController::run(inCmd, options);

	if (!errors.empty())
	{
		bool printed2 = false;
		auto list = String::split(errors, '\n');
		for (auto& line : list)
		{
			if (line.empty())
				continue;

			// TODO: figure out what this is - seems like a false positive
			if (String::contains("DVTCoreDeviceEnabledState:", line))
				continue;

			bool last = &line == &list.back();
			if (!last)
				line += '\n';

			if (!printed2 && printed)
				Output::lineBreak(true);

			std::cout.write(line.data(), line.size());
			printed2 = true;
		}

		if (printed2)
			Output::lineBreak(true);
	}

	return result == EXIT_SUCCESS;
}

/*****************************************************************************/
std::string CompileStrategyXcodeBuild::getPlatformName() const
{
	auto& osTargetName = m_state.inputs.osTargetName();
	if (String::equals("macosx", osTargetName))
		return "OS X";
	else if (String::equals("iphoneos", osTargetName))
		return "iOS";
	else if (String::equals("iphonesimulator", osTargetName))
		return "iOS Simulator";
	else if (String::equals("watchos", osTargetName))
		return "watchOS";
	else if (String::equals("watchsimulator", osTargetName))
		return "watchOS Simulator";
	else if (String::equals("appletvos", osTargetName))
		return "tvOS";
	else if (String::equals("appletvsimulator", osTargetName))
		return "tvOS Simulator";
	else if (String::equals("xros", osTargetName))
		return "visionOS";
	else if (String::equals("xrsimulator", osTargetName))
		return "visionOS Simulator";

	return std::string();
}
}
