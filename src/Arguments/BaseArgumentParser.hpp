/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BaseArgumentParser
{
	using RawArgumentList = std::unordered_map<std::string, std::string>;

public:
	BaseArgumentParser() = default;
	CHALET_DISALLOW_COPY_MOVE(BaseArgumentParser);
	virtual ~BaseArgumentParser() = default;

protected:
	bool parse(const int argc, const char* argv[], const int inPositionalArgs = 0);

	virtual StringList getTruthyArguments() const = 0;

	bool containsOption(const std::string& inOption);
	bool containsOption(const std::string& inShort, const std::string& inLong);

	RawArgumentList m_rawArguments;

private:
	std::optional<std::string> getOptionValue(const char** inBegin, const char** inEnd, const std::string& inOption);
	bool optionExists(const char** inBegin, const char** inEnd, const std::string& inOption);
};
}
