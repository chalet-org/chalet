/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CONFIGURE_FILE_PARSER_HPP
#define CHALET_CONFIGURE_FILE_PARSER_HPP

namespace chalet
{
class BuildState;
struct SourceTarget;

struct ConfigureFileParser
{
	explicit ConfigureFileParser(const BuildState& inState, const SourceTarget& inProject);

	bool run();

private:
	std::string getReplaceValue(std::string inKey) const;
	std::string getReplaceValueFromSubString(const std::string& inKey, const bool isTarget = false) const;

	const BuildState& m_state;
	const SourceTarget& m_project;
};
}

#endif // CHALET_CONFIGURE_FILE_PARSER_HPP