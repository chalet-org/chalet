/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
struct MesonTarget final : public IBuildTarget
{
	explicit MesonTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;
	virtual const std::string& getHash() const final;

	bool hashChanged() const noexcept;

	const StringList& defines() const noexcept;
	void addDefines(StringList&& inList);
	void addDefine(std::string&& inValue);

	const StringList& targets() const noexcept;
	void addTargets(StringList&& inList);
	void addTarget(std::string&& inValue);

	const std::string& location() const noexcept;
	const std::string& targetFolder() const noexcept;
	void setLocation(std::string&& inValue) noexcept;

	const std::string& buildFile() const noexcept;
	void setBuildFile(std::string&& inValue) noexcept;

	const std::string& runExecutable() const noexcept;
	void setRunExecutable(std::string&& inValue) noexcept;

	bool recheck() const noexcept;
	void setRecheck(const bool inValue) noexcept;

	bool rebuild() const noexcept;
	void setRebuild(const bool inValue) noexcept;

	bool clean() const noexcept;
	void setClean(const bool inValue) noexcept;

	bool install() const noexcept;
	void setInstall(const bool inValue) noexcept;

private:
	StringList m_defines;
	StringList m_targets;
	std::string m_location;
	std::string m_targetFolder;
	std::string m_buildFile;
	std::string m_toolset;
	std::string m_runExecutable;

	mutable i32 m_hashChanged = -1;

	bool m_recheck = true;
	bool m_rebuild = true;
	bool m_clean = true;
	bool m_install = false;
};
}
