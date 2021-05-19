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
		StringList command;
		std::string renameFrom;
		std::string renameTo;
	};
	struct CommandTemp
	{
		StringList command;
		std::string renameFrom;
		std::string renameTo;
	};

	using CommandList = std::vector<Command>;

	struct NativeTarget
	{
		Command pch;
		Command linkTarget;
		CommandList compiles;
		CommandList assemblies;
	};

public:
	explicit CompileStrategyNative(BuildState& inState);

	virtual bool initialize() final;
	virtual bool addProject(const ProjectConfiguration& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain) final;

	virtual bool saveBuildFile() const final;
	virtual bool buildProject(const ProjectConfiguration& inProject) const final;

private:
	Command getPchCommand(const std::string& pchTarget);
	CommandList getCompileCommands(const StringList& inObjects);
	CommandList getAsmCommands(const StringList& inAssemblies);
	Command getLinkCommand(const std::string& inTarget, const StringList& inObjects);

	CommandTemp getPchCompile(const std::string& source, const std::string& target) const;
	CommandTemp getCxxCompile(const std::string& source, const std::string& target, CxxSpecialization specialization) const;
	CommandTemp getRcCompile(const std::string& source, const std::string& target) const;
	StringList getAsmGenerate(const std::string& object, const std::string& target) const;

	const ProjectConfiguration* m_project = nullptr;
	ICompileToolchain* m_toolchain = nullptr;

	std::unordered_map<std::string, std::unique_ptr<NativeTarget>> m_targets;

	mutable ThreadPool m_threadPool;

	bool m_generateDependencies = false;
	mutable bool m_canceled = false;
};
}

#endif // CHALET_COMPILE_STRATEGY_NATIVE_HPP
