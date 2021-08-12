/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_STRATEGY_NATIVE_HPP
#define CHALET_COMPILE_STRATEGY_NATIVE_HPP

#include "Compile/Strategy/ICompileStrategy.hpp"
#include "Compile/Toolchain/ICompileToolchain.hpp"

#include "Compile/CommandPool.hpp"
#include "State/BuildState.hpp"
#include "State/Target/ProjectTarget.hpp"

namespace chalet
{
class CompileStrategyNative final : public ICompileStrategy
{
	struct CmdTemp
	{
		StringList command;
		std::string sourceFile;
	};

public:
	explicit CompileStrategyNative(BuildState& inState);

	virtual bool initialize(const StringList& inFileExtensions) final;
	virtual bool addProject(const ProjectTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain) final;

	virtual bool saveBuildFile() const final;
	virtual bool doPostBuild() const final;
	virtual bool buildProject(const ProjectTarget& inProject) const final;

private:
	CommandPool::Cmd getPchCommand(const std::string& pchTarget);
	CommandPool::CmdList getCompileCommands(const SourceFileGroupList& inGroups);
	CommandPool::Cmd getLinkCommand(const std::string& inTarget, const StringList& inObjects);

	CmdTemp getPchCompile(const std::string& source, const std::string& target) const;
	CmdTemp getCxxCompile(const std::string& source, const std::string& target, CxxSpecialization specialization) const;
	CmdTemp getRcCompile(const std::string& source, const std::string& target) const;

	CommandPool m_commandPool;

	StringList m_fileCache;

	const ProjectTarget* m_project = nullptr;
	ICompileToolchain* m_toolchain = nullptr;

	std::unordered_map<std::string, std::unique_ptr<CommandPool::Target>> m_targets;

	bool m_generateDependencies = false;
	bool m_pchChanged = false;
	bool m_sourcesChanged = false;
	bool m_initialized = false;
};
}

#endif // CHALET_COMPILE_STRATEGY_NATIVE_HPP
