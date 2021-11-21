/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMAND_POOL_HPP
#define CHALET_COMMAND_POOL_HPP

#include "Terminal/Color.hpp"
#include "Utility/ThreadPool.hpp"

namespace chalet
{
struct CommandPool
{
	struct Cmd
	{
		std::string output;
		StringList command;
		std::string label;
#if defined(CHALET_WIN32)
		std::string outputReplace;
#endif
	};
	using CmdList = std::vector<Cmd>;

	struct Target
	{
		CmdList pre;
		Cmd post;
		CmdList list;
	};

	struct Settings
	{
		Color color = Color::Red;
		bool quiet = false;
		bool showCommands = false;
		bool msvcCommand = false;
		bool renameAfterCommand = false;
	};

	explicit CommandPool(const std::size_t inThreads);

	bool run(const Target& inTarget, const Settings& inSettings);

private:
	std::string getPrintedText(std::string inText, uint inTotal);
	bool onError();
	void cleanup();

	ThreadPool m_threadPool;

	std::string m_reset;
	std::string m_exceptionThrown;
	bool m_quiet = false;
};
}

#endif // CHALET_COMMAND_POOL_HPP
