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
CompileStrategyNative::CompileStrategyNative(BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain) :
	m_state(inState),
	m_project(inProject),
	m_toolchain(inToolchain),
	m_threadPool(m_state.environment.maxJobs())
{
	// m_generateDependencies = !Environment::isContinuousIntegrationServer();
	m_generateDependencies = true;
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
	::signal(SIGINT, signalHandler);
	::signal(SIGTERM, signalHandler);
	::signal(SIGABRT, signalHandler);

	s_shutdownHandler = [this]() {
		this->m_threadPool.stop();
		this->m_canceled = true;
	};

	bool cleanOutput = m_state.environment.cleanOutput();

	s_compileIndex = 1;
	uint totalCompiles = static_cast<uint>(m_compiles.size());

	if (m_project.usesPch())
	{
		totalCompiles++;

		if (!printCommand(m_pch.output, m_pch.command, Color::Blue, " ", cleanOutput, totalCompiles))
			return false;

		if (!executeCommand(m_pch.command, m_pch.renameFrom, m_pch.renameTo, m_generateDependencies))
			return false;
	}

	bool buildFailed = false;
	std::vector<std::future<bool>> threadResults;
	for (auto& it : m_compiles)
	{
		threadResults.emplace_back(m_threadPool.enqueue(printCommand, it.output, it.command, Color::Blue, " ", cleanOutput, totalCompiles));
		threadResults.emplace_back(m_threadPool.enqueue(executeCommand, it.command, it.renameFrom, it.renameTo, m_generateDependencies));
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

	if (!printCommand(m_linker.output, m_linker.command, Color::Blue, Unicode::rightwardsTripleArrow(), cleanOutput))
		return false;

	if (!executeCommand(m_linker.command, m_linker.renameFrom, m_linker.renameTo, m_generateDependencies))
		return false;

	if (m_project.dumpAssembly())
	{
		s_compileIndex = 1;
		totalCompiles = static_cast<uint>(m_assemblies.size());

		threadResults.clear();
		for (auto& it : m_assemblies)
		{
			threadResults.emplace_back(m_threadPool.enqueue(printCommand, it.output, it.command, Color::Magenta, " ", cleanOutput, totalCompiles));
			threadResults.emplace_back(m_threadPool.enqueue(executeCommand, it.command, it.renameFrom, it.renameTo, m_generateDependencies));
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
void CompileStrategyNative::getCompileCommands(const StringList& inObjects)
{
	const auto& compilerConfig = m_state.compilers.getConfig(m_project.language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(m_project, compilerConfig.isClang());
	if (m_project.usesPch())
	{
		std::string source = m_project.pch();

		auto tmp = getPchCompile(source, pchTarget);
		m_pch.output = std::move(source);
		m_pch.command = std::move(tmp.command);
		m_pch.renameFrom = std::move(tmp.renameFrom);
		m_pch.renameTo = std::move(tmp.renameTo);
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

		CxxSpecialization specialization = CxxSpecialization::Cpp;
		if (String::endsWith(".m", source) || String::endsWith(".M", source))
			specialization = CxxSpecialization::ObjectiveC;
		else if (String::endsWith(".mm", source))
			specialization = CxxSpecialization::ObjectiveCpp;

		if (String::endsWith(".rc", source))
		{
#if defined(CHALET_WIN32)
			auto tmp = getRcCompile(source, target);
			Command out;
			out.output = std::move(source);
			out.command = std::move(tmp.command);
			out.renameFrom = std::move(tmp.renameFrom);
			out.renameTo = std::move(tmp.renameTo);
			m_compiles.push_back(std::move(out));
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
			m_compiles.push_back(std::move(out));
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
		m_assemblies.push_back({ asmFile, String::split(command), std::string(), std::string() });
	}
}

/*****************************************************************************/
void CompileStrategyNative::getLinkCommand(const StringList& inObjects)
{
	const auto target = m_state.paths.getTargetFilename(m_project);
	const auto targetBasename = m_state.paths.getTargetBasename(m_project);

	m_linker.command = m_toolchain->getLinkerTargetCommand(target, inObjects, targetBasename);
	m_linker.output = fmt::format("Linking {}", target);
}

/*****************************************************************************/
CompileStrategyNative::CommandTemp CompileStrategyNative::getPchCompile(const std::string& source, const std::string& target) const
{
	CommandTemp ret;

	if (m_project.usesPch())
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
	CommandTemp ret;

	const auto& depDir = m_state.paths.depDir();

	ret.renameFrom = fmt::format("{depDir}/{source}.Td", FMT_ARG(depDir), FMT_ARG(source));
	ret.renameTo = fmt::format("{depDir}/{source}.d", FMT_ARG(depDir), FMT_ARG(source));

	// TODO: Split between C, C++ Objective-C, Objective-C++
	ret.command = m_toolchain->getCxxCompileCommand(source, target, m_generateDependencies, ret.renameFrom, specialization);

	return ret;
}

/*****************************************************************************/
CompileStrategyNative::CommandTemp CompileStrategyNative::getRcCompile(const std::string& source, const std::string& target) const
{
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
std::string CompileStrategyNative::getAsmGenerate(const std::string& object, const std::string& target) const
{
	std::string ret;

	ret = m_state.tools.getAsmGenerateCommand(object, target);

	return ret;
}

}
