/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class ConditionOp : u8
{
	And,
	Or,
	InvalidOr,
};
enum class ConditionResult : u8
{
	Fail,
	Pass,
	Invalid,
};
struct PropertyConditionMatcher
{
	bool match(const std::string& inText, const std::function<bool(const std::string&, const std::string&, bool)>& onMatch) const;

	mutable ConditionOp lastOp = ConditionOp::And;
};
}
