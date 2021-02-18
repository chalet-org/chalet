/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyNative.hpp"

#include "Compile/Strategy/NinjaGenerator.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Color.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"

// TODO: Finish. It's a bit crap

namespace chalet
{
/*****************************************************************************/
namespace
{
std::mutex s_mutex;
std::function<void()> s_shutdownHandler;

/*****************************************************************************/
bool printCommand(std::string output, std::string command, Color inColor, std::string symbol, bool cleanOutput, uint index = 0, uint total = 0)
{
	s_mutex.lock();
	if (total > 0)
	{
		auto indexStr = std::to_string(index);
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
	s_mutex.unlock();

	return true;
}

/*****************************************************************************/
bool executeCommand(std::string command)
{
	StringList commands = String::split(command, " && ");
	for (auto& cmd : commands)
	{
		const auto& cmdSplit = String::split(cmd, " ");
		if (cmdSplit.size() == 0)
			continue;

		if (String::equals(cmdSplit.front(), "rename"))
		{
			if (cmdSplit.size() != 3)
				continue;

			s_mutex.lock();
			const bool result = Commands::rename(cmdSplit[1], cmdSplit[2]);
			s_mutex.unlock();
			if (!result)
				return false;
		}
		else
		{
			if (!Commands::subprocess(cmdSplit))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
void signalHandler(int inSignal)
{
	UNUSED(inSignal);
	s_shutdownHandler();
}
}

/*****************************************************************************/
CompileStrategyNative::CompileStrategyNative(BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain) :
	m_state(inState),
	m_project(inProject),
	m_toolchain(inToolchain),
	m_threadPool(m_state.environment.maxJobs())
{
}

/*****************************************************************************/
StrategyType CompileStrategyNative::type() const
{
	return StrategyType::Native;
}

/*****************************************************************************/
bool CompileStrategyNative::createCache(const SourceOutputs& inOutputs)
{
	getCompileCommands(inOutputs.objectList);
	getAsmCommands(inOutputs.assemblyList);
	getLinkCommand(inOutputs.objectList);

	return true;
}

/*****************************************************************************/
bool CompileStrategyNative::initialize()
{
	return true;
}

/*****************************************************************************/
bool CompileStrategyNative::run()
{
	std::signal(SIGINT, signalHandler);
	std::signal(SIGTERM, signalHandler);
	std::signal(SIGABRT, signalHandler);

	s_shutdownHandler = [this]() {
		this->m_threadPool.stop();
		this->m_canceled = true;
	};

	bool cleanOutput = m_state.environment.cleanOutput();

	uint index = 1;
	uint totalCompiles = static_cast<uint>(m_compiles.size());

	bool result = true;
	if (m_project.usesPch())
	{
		auto shitList = String::split(m_pch.command, " ");
		UNUSED(shitList);

		totalCompiles++;

		result |= printCommand(m_pch.output, m_pch.command, Color::Blue, " ", cleanOutput, index, totalCompiles);
		result |= executeCommand(m_pch.command);
		index++;
	}

	if (result)
	{
		std::vector<std::future<bool>> threadResults;
		for (auto& it : m_compiles)
		{
			threadResults.emplace_back(m_threadPool.enqueue(printCommand, it.output, it.command, Color::Blue, " ", cleanOutput, index, totalCompiles));
			threadResults.emplace_back(m_threadPool.enqueue(executeCommand, it.command));
			index++;
		}

		for (auto& tr : threadResults)
		{
			try
			{
				result |= tr.get();
			}
			catch (std::future_error&)
			{
				result = false;
				break;
			}
		}

		if (result)
		{
			Output::lineBreak();

			result |= printCommand(m_linker.output, m_linker.command, Color::Blue, u8"\xE2\x87\x9B", cleanOutput);
			result |= executeCommand(m_linker.command);
		}
	}

	if (result && m_project.dumpAssembly())
	{
		index = 1;
		totalCompiles = static_cast<uint>(m_assemblies.size());

		std::vector<std::future<bool>> threadResults;
		for (auto& it : m_assemblies)
		{
			threadResults.emplace_back(m_threadPool.enqueue(printCommand, it.output, it.command, Color::Magenta, " ", cleanOutput, index, totalCompiles));
			threadResults.emplace_back(m_threadPool.enqueue(executeCommand, it.command));
			index++;
		}

		for (auto& tr : threadResults)
		{
			try
			{
				result |= tr.get();
			}
			catch (std::future_error&)
			{
				result = false;
				break;
			}
		}
	}

	Output::lineBreak();

	const bool error = !result || m_canceled;
	return !error;
}

/*****************************************************************************/
void CompileStrategyNative::getCompileCommands(const StringList& inObjects)
{
	const auto& compilerConfig = m_state.compilers.getConfig(m_project.language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(m_project, compilerConfig.isClang());
	if (m_project.usesPch())
	{
		std::string source = m_project.pch();
		std::string command = getPchCompile(source, pchTarget);
		m_pch = { std::move(source), std::move(command) };
	}

	const auto& objDir = fmt::format("{}/", m_state.paths.objDir());

	for (auto& target : inObjects)
	{
		if (target.empty())
			continue;

		std::string source = target;
		String::replaceAll(source, objDir, "");

		if (String::endsWith(".o", source))
			source = source.substr(0, source.size() - 2);
		else if (String::endsWith(".res", source))
			source = source.substr(0, source.size() - 4);

		if (String::endsWith(".rc", source))
		{
#if defined(CHALET_WIN32)
			auto command = getRcCompile(source, target);
			m_compiles.push_back({ std::move(source), std::move(command) });
#else
			continue;
#endif
		}
		else
		{
			auto command = getCppCompile(source, target);
			m_compiles.push_back({ std::move(source), std::move(command) });
		}
	}
}

/*****************************************************************************/
void CompileStrategyNative::getAsmCommands(const StringList& inAssemblies)
{
	if (!m_project.dumpAssembly())
		return;

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

		std::string command = getAsmGenerate(object, asmFile);
		m_assemblies.push_back({ asmFile, std::move(command) });
	}
}

/*****************************************************************************/
void CompileStrategyNative::getLinkCommand(const StringList& inObjects)
{
	auto objects = String::join(inObjects);

	const auto target = m_state.paths.getTargetFilename(m_project);
	const auto targetBasename = m_state.paths.getTargetBasename(m_project);
	std::string command = m_toolchain->getLinkerTargetCommand(target, objects, targetBasename);
	while (command.back() == ' ')
		command.pop_back();

	std::string output = fmt::format("Linking {}", target);
	m_linker = { std::move(output), std::move(command) };
}

/*****************************************************************************/
std::string CompileStrategyNative::getPchCompile(const std::string& source, const std::string& target) const
{
	std::string ret;

	if (m_project.usesPch())
	{
		const auto& depDir = m_state.paths.depDir();

		const auto dependency = fmt::format("{depDir}/{source}.d", FMT_ARG(depDir), FMT_ARG(source));
		const auto tempDependency = fmt::format("{depDir}/{source}.Td", FMT_ARG(depDir), FMT_ARG(source));

		const auto compile = m_toolchain->getPchCompileCommand(source, target, tempDependency);
		const auto moveCommand = getRenameCommand(tempDependency, dependency);

		ret = fmt::format("{compile} && {moveCommand}",
			FMT_ARG(compile),
			FMT_ARG(moveCommand));
	}

	return ret;
}

/*****************************************************************************/
std::string CompileStrategyNative::getCppCompile(const std::string& source, const std::string& target) const
{
	std::string ret;

	const auto& depDir = m_state.paths.depDir();

	const auto dependency = fmt::format("{depDir}/{source}.d", FMT_ARG(depDir), FMT_ARG(source));
	const auto tempDependency = fmt::format("{depDir}/{source}.Td", FMT_ARG(depDir), FMT_ARG(source));

	const auto compile = m_toolchain->getCppCompileCommand(source, target, tempDependency);
	const auto moveCommand = getRenameCommand(tempDependency, dependency);

	ret = fmt::format("{compile} && {moveCommand}",
		FMT_ARG(compile),
		FMT_ARG(moveCommand));

	return ret;
}

/*****************************************************************************/
std::string CompileStrategyNative::getRcCompile(const std::string& source, const std::string& target) const
{
	std::string ret;

#if defined(CHALET_WIN32)
	const auto& depDir = m_state.paths.depDir();

	const auto dependency = fmt::format("{depDir}/{source}.d", FMT_ARG(depDir), FMT_ARG(source));
	const auto tempDependency = fmt::format("{depDir}/{source}.Td", FMT_ARG(depDir), FMT_ARG(source));

	const auto compile = m_toolchain->getRcCompileCommand(source, target, tempDependency);
	const auto moveCommand = getRenameCommand(tempDependency, dependency);

	ret = fmt::format("{compile} && {moveCommand}",
		FMT_ARG(compile),
		FMT_ARG(moveCommand));
#else
	UNUSED(source, target);
#endif

	return ret;
}

/*****************************************************************************/
std::string CompileStrategyNative::getAsmGenerate(const std::string& object, const std::string& target) const
{
	std::string ret;

	ret = m_toolchain->getAsmGenerateCommand(object, target);

	return ret;
}

/*****************************************************************************/
std::string CompileStrategyNative::getRenameCommand(const std::string& inInput, const std::string& inOutput) const
{
	return fmt::format("rename {} {}", inInput, inOutput);
}
}
