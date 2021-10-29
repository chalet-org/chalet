/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IARCHIVER_HPP
#define CHALET_IARCHIVER_HPP

#include "Compile/IToolchainExecutableBase.hpp"

namespace chalet
{
struct IArchiver : public IToolchainExecutableBase
{
	explicit IArchiver(const BuildState& inState, const SourceTarget& inProject);

	[[nodiscard]] static Unique<IArchiver> make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject);

	virtual StringList getCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) const = 0;

protected:
	virtual void addSourceObjects(StringList& outArgList, const StringList& sourceObjs) const final;
};
}

#endif // CHALET_IARCHIVER_HPP
