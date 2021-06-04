/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/DependencyWalker.hpp"

#include "Libraries/Format.hpp"
#include "Libraries/WindowsApi.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
#if defined(CHALET_WIN32)
/*****************************************************************************/
bool DependencyWalker::read(const std::string& inFile, StringList& outList)
{
	if (!verifyImageFile(inFile))
		return false;

	if (!parseFile(inFile, outList))
		return false;

	return true;
}

/*****************************************************************************/
bool DependencyWalker::verifyImageFile(const std::string& inFile)
{
	if (Commands::pathExists(inFile))
	{
		return String::endsWith({ ".dll", ".DLL", ".exe", ".EXE" }, inFile);
	}

	return false;
}

/*****************************************************************************/
bool DependencyWalker::parseFile(const std::string& inFile, StringList& outList)
{
	std::vector<char> bytes = readAllBytes(inFile.c_str());
	StringList ignoreList{ inFile, "System32", "SYSTEM32", "SysWOW64", "SYSWOW64" };

	// Timer timer;

	// This is extremely crude, but works pretty reliably for the time being

	std::string temp(bytes.data(), bytes.size());

	auto first = temp.find(".dll");
	auto last = temp.rfind(".dll") + 4;
	if (first != std::string::npos && first >= 100 && last != std::string::npos)
	{
		auto begin = first - 100;
		temp = temp.substr(begin, last - begin);

		std::size_t pos = 0;
		while ((pos = temp.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890._+-", pos)) != std::string::npos)
		{
			temp.replace(pos, 1, "\n");
			pos += 1;
		}

		const std::size_t maxSearchLength = 5; // ".dll" + at least 1 char
		auto split = String::split(temp, '\n', maxSearchLength);
		StringList searches{ ".dll", ".DLL" };
		for (auto& line : split)
		{
			if (line.empty() || !String::contains('.', line))
				continue;

			if (!String::endsWith(searches, line))
				continue;

			if (!String::contains(ignoreList, line))
			{
				List::addIfDoesNotExist(outList, String::toLowerCase(line));
			}
		}
	}

	// Output::print(Color::Reset, fmt::format("   Dependency check time: {}", timer.asString()));
	// Output::lineBreak();

	return true;
}

/*****************************************************************************/
std::vector<char> DependencyWalker::readAllBytes(const std::string& inFile)
{
	std::ifstream ifs(inFile, std::ios::binary | std::ios::ate);
	std::size_t length = ifs.tellg();

	std::vector<char> buffer(length);
	ifs.seekg(0, std::ios::beg);
	ifs.read(buffer.data(), length);

	return buffer;
}

#endif
}
