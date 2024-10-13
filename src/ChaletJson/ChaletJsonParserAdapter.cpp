/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "ChaletJson/ChaletJsonParserAdapter.hpp"

#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool ChaletJsonParserAdapter::matchConditionVariables(const std::string& inText, const std::function<bool(const std::string&, const std::string&, bool)>& onMatch) const
{
	auto squareBracketBegin = inText.find('[');
	if (squareBracketBegin == std::string::npos)
		return true;

	auto squareBracketEnd = inText.find(']', squareBracketBegin);
	if (squareBracketEnd == std::string::npos)
		return true;

	// LOG("evaluating:", inText);

	char op;
	auto raw = inText.substr(squareBracketBegin + 1, (squareBracketEnd - squareBracketBegin) - 1);
	if (raw.find('|') != std::string::npos)
	{
		this->lastOp = ConditionOp::Or;
		op = '|';
	}
	else
	{
		this->lastOp = ConditionOp::And;
		op = '+';
	}

	bool isOr = this->lastOp == ConditionOp::Or;
	if (isOr && raw.find('+') != std::string::npos)
	{
		this->lastOp = ConditionOp::InvalidOr;
		return false;
	}

	bool result = !isOr;
	auto split = String::split(raw, op);
	for (auto& prop : split)
	{
		auto propSplit = String::split(prop, ':');
		auto& key = propSplit.front();
		if (key.find_first_of("!{},") != std::string::npos)
		{
			result = false;
			break;
		}

		if (propSplit.size() > 1)
		{
			auto& inner = propSplit[1];
			if (inner.front() == '{' && inner.back() == '}')
			{
				inner = inner.substr(1);
				inner.pop_back();
			}

			if (String::contains(',', inner))
			{
				bool valid = false;
				auto innerSplit = String::split(inner, ',');
				for (auto& v : innerSplit)
				{
					bool negate = false;
					if (v.front() == '!')
					{
						v = v.substr(1);
						negate = true;
					}

					if (v.find_first_of("!{},") == std::string::npos)
					{
						valid |= onMatch(key, v, negate);
					}
					else
					{
						valid = false;
						break;
					}
				}
				if (isOr)
					result |= valid;
				else
					result &= valid;
			}
			else
			{
				bool negate = false;
				if (inner.front() == '!')
				{
					inner = inner.substr(1);
					negate = true;
				}

				if (inner.find_first_of("!{},") == std::string::npos)
				{
					if (isOr)
						result |= onMatch(key, inner, negate);
					else
						result &= onMatch(key, inner, negate);
				}
				else
				{
					result = false;
					break;
				}
			}
		}
		else
		{
			bool negate = false;
			if (key.front() == '!')
			{
				key = key.substr(1);
				negate = true;
			}

			if (isOr)
				result |= onMatch(key, std::string{}, negate);
			else
				result &= onMatch(key, std::string{}, negate);
		}
	}

	return result;
}
}
