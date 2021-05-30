/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyNative.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Color.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/String.hpp"
#include "Utility/Subprocess.hpp"

// TODO: Finish. It's a bit crap

namespace chalet
{
/*****************************************************************************/
namespace
{
std::mutex s_mutex;
std::atomic<uint> s_compileIndex = 0;
std::function<void()> s_shutdownHandler;

/*****************************************************************************/
bool printCommand(std::string output, StringList command, Color inColor, std::string symbol, bool cleanOutput, uint total = 0)
{
	std::unique_lock<std::mutex> lock(s_mutex);
	if (total > 0)
	{
		auto indexStr = std::to_string(s_compileIndex);
		const auto totalStr = std::to_string(total);
		while (indexStr.size() < totalStr.size())
		{
			indexStr = " " + indexStr;
		}

		std::cout << Output::getAnsiStyle(inColor) << fmt::format("{}  ", symbol) << Output::getAnsiReset() << fmt::format("[{}/{}] ", indexStr, total);
	}
	else
	{
		std::cout << Output::getAnsiStyle(inColor) << fmt::format("{}  ", symbol);
	}

	if (cleanOutput)
		Output::print(inColor, output);
	else
		Output::print(inColor, command);

	s_compileIndex++;

	return true;
}

/*****************************************************************************/
bool executeCommandMsvc(StringList command, std::string renameFrom, std::string renameTo, bool generateDependencies)
{
	std::string srcFile;
	{
		auto start = renameFrom.find_last_of('/') + 1;
		auto end = renameFrom.find_last_of('.');
		srcFile = renameFrom.substr(start, end - start);
	}

	SubprocessOptions options;
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::StdErr;
	options.onStdOut = [&srcFile](std::string inData) {
		if (String::startsWith(srcFile, inData))
			return;

		std::cout << inData << std::flush;
	};

	if (Subprocess::run(command, std::move(options)) != EXIT_SUCCESS)
		return false;

	if (!generateDependencies)
		return true;

	if (!renameFrom.empty() && !renameTo.empty())
	{
		std::unique_lock<std::mutex> lock(s_mutex);
		return Commands::rename(renameFrom, renameTo);
	}

	return true;
}

/*****************************************************************************/
bool executeCommand(StringList command, std::string renameFrom, std::string renameTo, bool generateDependencies)
{
	if (!Commands::subprocess(command))
		return false;

	if (!generateDependencies)
		return true;

	if (!renameFrom.empty() && !renameTo.empty())
	{
		std::unique_lock<std::mutex> lock(s_mutex);
		return Commands::rename(renameFrom, renameTo);
	}

	return true;
}

/*****************************************************************************/
void signalHandler(int inSignal)
{
	Subprocess::haltAllProcesses(inSignal);
	s_shutdownHandler();
}
}

/*****************************************************************************/
CompileStrategyNative::CompileStrategyNative(BuildState& inState) :
	ICompileStrategy(StrategyType::Native, inState),
	m_threadPool(m_state.environment.maxJobs())
{
}

/*****************************************************************************/
bool CompileStrategyNative::initialize()
{
	return true;
}

/*****************************************************************************/
bool CompileStrategyNative::addProject(const ProjectTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain)
{
	m_project = &inProject;
	m_toolchain = inToolchain.get();

	chalet_assert(m_project != nullptr, "");

	const auto& compilerConfig = m_state.compilerTools.getConfig(m_project->language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project, compilerConfig.isClangOrMsvc());

	m_generateDependencies = !Environment::isContinuousIntegrationServer() && !compilerConfig.isMsvc();

	auto target = std::make_unique<NativeTarget>();
	target->pch = getPchCommand(pchTarget);
	target->compiles = getCompileCommands(inOutputs.objectList);
	target->assemblies = getAsmCommands(inOutputs.assemblyList);
	target->linkTarget = getLinkCommand(inOutputs.target, inOutputs.objectListLinker);

	auto& name = inProject.name();

	if (m_targets.find(name) == m_targets.end())
	{
		m_targets.emplace(name, std::move(target));
	}

	m_outputs[name] = std::move(inOutputs);

	m_toolchain = nullptr;
	m_project = nullptr;

	return true;
}

/*****************************************************************************/
bool CompileStrategyNative::saveBuildFile() const
{
	return true;
}

/*****************************************************************************/
bool CompileStrategyNative::buildProject(const ProjectTarget& inProject) const
{
	if (m_targets.find(inProject.name()) == m_targets.end())
		return false;

	auto& target = *m_targets.at(inProject.name());
	auto& compiles = target.compiles;
	auto& pch = target.pch;
	auto& linkTarget = target.linkTarget;
	auto& assemblies = target.assemblies;

	::signal(SIGINT, signalHandler);
	::signal(SIGTERM, signalHandler);
	::signal(SIGABRT, signalHandler);

	s_shutdownHandler = [this]() {
		this->m_threadPool.stop();
		this->m_canceled = true;
	};

	const auto& config = m_state.compilerTools.getConfig(inProject.language());
	auto executeCommandFunc = config.isMsvc() ? executeCommandMsvc : executeCommand;

	bool cleanOutput = m_state.environment.cleanOutput();

	s_compileIndex = 1;
	uint totalCompiles = static_cast<uint>(compiles.size());

	if (!pch.command.empty())
	{
		totalCompiles++;

		if (!printCommand(pch.output, pch.command, Color::Blue, " ", cleanOutput, totalCompiles))
			return false;

		if (!executeCommandFunc(pch.command, pch.renameFrom, pch.renameTo, m_generateDependencies))
			return false;
	}

	bool buildFailed = false;
	std::vector<std::future<bool>> threadResults;
	for (auto& it : compiles)
	{
		threadResults.emplace_back(m_threadPool.enqueue(printCommand, it.output, it.command, Color::Blue, " ", cleanOutput, totalCompiles));
		threadResults.emplace_back(m_threadPool.enqueue(executeCommandFunc, it.command, it.renameFrom, it.renameTo, m_generateDependencies));
	}

	for (auto& tr : threadResults)
	{
		try
		{
			if (!tr.get())
			{
				signalHandler(SIGTERM);
				buildFailed = true;
				break;
			}
		}
		catch (std::future_error& err)
		{
			std::cerr << err.what() << std::endl;
			return false;
		}
	}

	if (buildFailed)
	{
		threadResults.clear();
		return false;
	}

	Output::lineBreak();

	if (!printCommand(linkTarget.output, linkTarget.command, Color::Blue, Unicode::rightwardsTripleArrow(), cleanOutput))
		return false;

	if (!executeCommandFunc(linkTarget.command, linkTarget.renameFrom, linkTarget.renameTo, m_generateDependencies))
		return false;

	if (!assemblies.empty())
	{
		Output::lineBreak();

		s_compileIndex = 1;
		totalCompiles = static_cast<uint>(assemblies.size());

		threadResults.clear();
		for (auto& it : assemblies)
		{
			threadResults.emplace_back(m_threadPool.enqueue(printCommand, it.output, it.command, Color::Magenta, " ", cleanOutput, totalCompiles));
			threadResults.emplace_back(m_threadPool.enqueue(executeCommandFunc, it.command, it.renameFrom, it.renameTo, m_generateDependencies));
		}

		for (auto& tr : threadResults)
		{
			try
			{
				if (!tr.get())
				{
					signalHandler(SIGTERM);
					buildFailed = true;
					break;
				}
			}
			catch (std::future_error& err)
			{
				std::cerr << err.what() << std::endl;
				return false;
			}
		}
	}

	if (buildFailed)
	{
		threadResults.clear();
		return false;
	}

	Output::lineBreak();

	const bool completed = !m_canceled;
	return completed;
}

/*****************************************************************************/
CompileStrategyNative::Command CompileStrategyNative::getPchCommand(const std::string& pchTarget)
{
	chalet_assert(m_project != nullptr, "");

	Command ret;
	if (m_project->usesPch())
	{
		std::string source = m_project->pch();

		auto tmp = getPchCompile(source, pchTarget);
		ret.output = std::move(source);
		ret.command = std::move(tmp.command);
		ret.renameFrom = std::move(tmp.renameFrom);
		ret.renameTo = std::move(tmp.renameTo);
	}

	return ret;
}

/*****************************************************************************/
CompileStrategyNative::CommandList CompileStrategyNative::getCompileCommands(const StringList& inObjects)
{
	const auto& objDir = fmt::format("{}/", m_state.paths.objDir());

	CommandList ret;

	for (auto& target : inObjects)
	{
		if (target.empty())
			continue;

		std::string source = target;
		String::replaceAll(source, objDir, "");

		if (String::endsWith(".o", source))
			source = source.substr(0, source.size() - 2);
		else if (String::endsWith({ ".res", ".obj" }, source))
			source = source.substr(0, source.size() - 4);

		CxxSpecialization specialization = CxxSpecialization::Cpp;
		if (String::endsWith({ ".m", ".M" }, source))
			specialization = CxxSpecialization::ObjectiveC;
		else if (String::endsWith(".mm", source))
			specialization = CxxSpecialization::ObjectiveCpp;

		if (String::endsWith({ ".rc", ".RC" }, source))
		{
#if defined(CHALET_WIN32)
			auto tmp = getRcCompile(source, target);
			Command out;
			out.output = std::move(source);
			out.command = std::move(tmp.command);
			out.renameFrom = std::move(tmp.renameFrom);
			out.renameTo = std::move(tmp.renameTo);
			ret.push_back(std::move(out));
#else
			continue;
#endif
		}
		else
		{
			auto tmp = getCxxCompile(source, target, specialization);
			Command out;
			out.output = std::move(source);
			out.command = std::move(tmp.command);
			out.renameFrom = std::move(tmp.renameFrom);
			out.renameTo = std::move(tmp.renameTo);
			ret.push_back(std::move(out));
		}
	}

	return ret;
}

/*****************************************************************************/
CompileStrategyNative::CommandList CompileStrategyNative::getAsmCommands(const StringList& inAssemblies)
{
	chalet_assert(m_project != nullptr, "");

	CommandList ret;
	if (!m_project->dumpAssembly())
		return ret;

	const auto& objDir = m_state.paths.objDir();
	const auto& asmDir = m_state.paths.asmDir();

	for (auto& asmFile : inAssemblies)
	{
		if (asmFile.empty())
			continue;

		std::string object = asmFile;
		String::replaceAll(object, asmDir, objDir);

		if (String::endsWith(".asm", object))
			object = object.substr(0, object.size() - 4);

		auto command = getAsmGenerate(object, asmFile);
		ret.push_back({ asmFile, command, std::string(), std::string() });
	}

	return ret;
}

/*****************************************************************************/
CompileStrategyNative::Command CompileStrategyNative::getLinkCommand(const std::string& inTarget, const StringList& inObjects)
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	const auto targetBasename = m_state.paths.getTargetBasename(*m_project);

	const std::string description = m_project->isStaticLibrary() ? "Archiving" : "Linking";

	Command ret;
	ret.command = m_toolchain->getLinkerTargetCommand(inTarget, inObjects, targetBasename);
	ret.output = fmt::format("{} {}", description, inTarget);

	return ret;
}

/*****************************************************************************/
CompileStrategyNative::CommandTemp CompileStrategyNative::getPchCompile(const std::string& source, const std::string& target) const
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	CommandTemp ret;

	if (m_project->usesPch())
	{
		const auto& depDir = m_state.paths.depDir();

		ret.renameFrom = fmt::format("{depDir}/{source}.Td", FMT_ARG(depDir), FMT_ARG(source));
		ret.renameTo = fmt::format("{depDir}/{source}.d", FMT_ARG(depDir), FMT_ARG(source));

		ret.command = m_toolchain->getPchCompileCommand(source, target, m_generateDependencies, ret.renameFrom);
	}

	return ret;
}

/*****************************************************************************/
CompileStrategyNative::CommandTemp CompileStrategyNative::getCxxCompile(const std::string& source, const std::string& target, CxxSpecialization specialization) const
{
	chalet_assert(m_toolchain != nullptr, "");

	CommandTemp ret;

	const auto& depDir = m_state.paths.depDir();

	ret.renameFrom = fmt::format("{depDir}/{source}.Td", FMT_ARG(depDir), FMT_ARG(source));
	ret.renameTo = fmt::format("{depDir}/{source}.d", FMT_ARG(depDir), FMT_ARG(source));

	ret.command = m_toolchain->getCxxCompileCommand(source, target, m_generateDependencies, ret.renameFrom, specialization);

	return ret;
}

/*****************************************************************************/
CompileStrategyNative::CommandTemp CompileStrategyNative::getRcCompile(const std::string& source, const std::string& target) const
{
	chalet_assert(m_toolchain != nullptr, "");

	CommandTemp ret;

#if defined(CHALET_WIN32)
	const auto& depDir = m_state.paths.depDir();

	ret.renameFrom = fmt::format("{depDir}/{source}.Td", FMT_ARG(depDir), FMT_ARG(source));
	ret.renameTo = fmt::format("{depDir}/{source}.d", FMT_ARG(depDir), FMT_ARG(source));

	ret.command = m_toolchain->getRcCompileCommand(source, target, m_generateDependencies, ret.renameFrom);
#else
	UNUSED(source, target);
#endif

	return ret;
}

/*****************************************************************************/
StringList CompileStrategyNative::getAsmGenerate(const std::string& object, const std::string& target) const
{
	StringList ret;

	if (!m_state.tools.bash().empty() && m_state.tools.bashAvailable())
	{
		auto asmCommand = m_state.tools.getAsmGenerateCommand(object, target);

		ret.push_back(m_state.tools.bash());
		ret.push_back("-c");
		ret.push_back(std::move(asmCommand));
	}

	return ret;
}

}
