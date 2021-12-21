/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROCESS_DIST_TARGET_HPP
#define CHALET_PROCESS_DIST_TARGET_HPP

#include "State/Distribution/IDistTarget.hpp"

namespace chalet
{
struct ProcessDistTarget final : public IDistTarget
{
	explicit ProcessDistTarget();

	virtual bool validate() final;

	const std::string& path() const noexcept;
	void setPath(std::string&& inValue) noexcept;

	const StringList& arguments() const noexcept;
	void addArguments(StringList&& inList);
	void addArgument(std::string&& inValue);

private:
	std::string m_path;
	StringList m_arguments;
};
}

#endif // CHALET_PROCESS_DIST_TARGET_HPP
