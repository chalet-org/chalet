/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_TERMINAL_TEST_HPP
#define CHALET_TERMINAL_TEST_HPP

namespace chalet
{
struct TerminalTest
{
	TerminalTest();

	bool run();

private:
	void printTerminalCapabilities();
	void printUnicodeCharacters();
	void printChaletColorThemes();

	void printBanner(const std::string& inText);

	const char kEsc;
	const std::size_t kWidth;

	const std::string kSeparator;

	std::string m_gray;
	std::string m_reset;
	std::string m_white;
	std::string m_separator;
};
}

#endif // CHALET_TERMINAL_TEST_HPP