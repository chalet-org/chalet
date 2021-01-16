/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_STRATEGY_NATIVE_HPP
#define CHALET_COMPILE_STRATEGY_NATIVE_HPP

#include <csignal>

#include "Compile/Strategy/ICompileStrategy.hpp"
#include "Compile/Toolchain/ICompileToolchain.hpp"

#include "BuildJson/ProjectConfiguration.hpp"
#include "State/BuildState.hpp"
#include "Utility/ThreadPool.hpp"

namespace chalet
{
class CompileStrategyNative final : public ICompileStrategy
{
	struct Command
	{
		std::string output;
		std::string command;
	};

	using CommandList = std::vector<Command>;

public:
	explicit CompileStrategyNative(BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain);

	virtual StrategyType type() const final;

	virtual bool createCache(const SourceOutputs& inOutputs) final;
	virtual bool initialize() final;
	virtual bool run() final;

private:
	void getCompileCommands(const StringList& inObjects);
	void getAsmCommands(const StringList& inAssemblies);
	void getLinkCommand(const StringList& inObjects);

	std::string getPchCompile(const std::string& source, const std::string& target) const;
	std::string getCppCompile(const std::string& source, const std::string& target) const;
	std::string getRcCompile(const std::string& source, const std::string& target) const;
	std::string getAsmGenerate(const std::string& object, const std::string& target) const;

	std::string getMoveCommand() const;

	BuildState& m_state;
	const ProjectConfiguration& m_project;
	CompileToolchain& m_toolchain;

	Command m_pch;
	Command m_linker;
	CommandList m_compiles;
	CommandList m_assemblies;

	ThreadPool m_threadPool;

	bool m_canceled = false;
};
}

#endif // CHALET_COMPILE_STRATEGY_NATIVE_HPP
