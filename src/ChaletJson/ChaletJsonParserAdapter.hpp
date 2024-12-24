/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class ConditionOp
{
	And,
	Or,
	InvalidOr,
};
enum class ConditionResult
{
	Fail,
	Pass,
	Invalid,
};
struct ChaletJsonParserAdapter
{
	bool matchConditionVariables(const std::string& inText, const std::function<bool(const std::string&, const std::string&, bool)>& onMatch) const;

	mutable ConditionOp lastOp = ConditionOp::And;
};
}
