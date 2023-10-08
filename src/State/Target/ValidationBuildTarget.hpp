/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VALIDATION_BUILD_TARGET_HPP
#define CHALET_VALIDATION_BUILD_TARGET_HPP

#include "State/ScriptType.hpp"
#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
struct ValidationBuildTarget final : public IBuildTarget
{
	explicit ValidationBuildTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;
	virtual std::string getHash() const final;

	const std::string& schema() const noexcept;
	void setSchema(std::string&& inValue) noexcept;

	const StringList& files() const noexcept;
	void addFiles(StringList&& inList);
	void addFile(std::string&& inValue);

private:
	std::string m_schema;
	StringList m_files;
};
}

#endif // CHALET_VALIDATION_BUILD_TARGET_HPP
