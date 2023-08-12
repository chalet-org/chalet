/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DEPENDENCY_WALKER_HPP
#define CHALET_DEPENDENCY_WALKER_HPP

// https://stackoverflow.com/questions/43670731/programmatically-get-list-of-dlls-used-to-build-a-process-or-library-in-a-non-de

namespace chalet
{
struct DependencyWalker
{
	DependencyWalker() = default;

	bool read(const std::string& inFile, StringList& outList, StringList* outNotFound, const bool includeWinUCRT);

private:
	bool verifyImageFile(const std::string& inFile);
	bool parseFile(const std::string& inFile, StringList& outList, StringList* outNotFound, const bool includeWinUCRT);
	std::vector<char> readAllBytes(const std::string& inFile);
};
}

#endif // CHALET_DEPENDENCY_WALKER_HPP
