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
		std::string symbol = " ";
		Color color = Color::Red;
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
		bool quiet = false;
		bool showCommands = false;
		bool msvcCommand = false;
		bool renameAfterCommand = false;
	};

	explicit CommandPool(const std::size_t inThreads);

	bool run(const Target& inTarget, const Settings& inSettings) const;

private:
	mutable ThreadPool m_threadPool;
};
}

#endif // CHALET_COMMAND_POOL_HPP
