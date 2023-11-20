/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Generator/NativeGenerator.hpp"
#include "Compile/Strategy/ICompileStrategy.hpp"

#include "State/Target/SourceTarget.hpp"

namespace chalet
{
class BuildState;

class CompileStrategyNative final : public ICompileStrategy
{
public:
	explicit CompileStrategyNative(BuildState& inState);

	virtual bool initialize() final;
	virtual bool addProject(const SourceTarget& inProject) final;

	virtual bool doPreBuild() final;
	virtual bool doPostBuild() const final;
	virtual bool buildProject(const SourceTarget& inProject) final;

private:
	std::string m_cacheFile;
	std::string m_cacheFolder;

	NativeGenerator m_nativeGenerator;

	bool m_initialized = false;
	bool m_cacheNeedsUpdate = false;
};
}
