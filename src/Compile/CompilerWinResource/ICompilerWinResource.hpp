/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ICOMPILER_WIN_RESOURCE_HPP
#define CHALET_ICOMPILER_WIN_RESOURCE_HPP

#include "Compile/IToolchainExecutableBase.hpp"

namespace chalet
{
struct ICompilerWinResource : public IToolchainExecutableBase
{
	explicit ICompilerWinResource(const BuildState& inState, const SourceTarget& inProject);

	[[nodiscard]] static Unique<ICompilerWinResource> make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() final;

	virtual StringList getCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) = 0;

protected:
	virtual void addIncludes(StringList& outArgList) const;
	virtual void addDefines(StringList& outArgList) const;
};
}

#endif // CHALET_ICOMPILER_WINDOWS_RESOURCE_HPP
