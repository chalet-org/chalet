/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VALIDATION_DIST_TARGET_HPP
#define CHALET_VALIDATION_DIST_TARGET_HPP

#include "State/Distribution/IDistTarget.hpp"
#include "State/ScriptType.hpp"

namespace chalet
{
struct ValidationDistTarget final : public IDistTarget
{
	explicit ValidationDistTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;

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

#endif // CHALET_VALIDATION_DIST_TARGET_HPP
