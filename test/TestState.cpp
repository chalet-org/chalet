/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "TestState.hpp"

#include "Utility/String.hpp"

namespace chalet
{
namespace
{
static std::string g_chalet;
}

bool setChaletPath(const int argc, const char* argv[])
{
	if (argc == 0)
		return false;

	UNUSED(argc);
	auto path = String::getPathFolder(std::string(argv[0]));

#if defined(CHALET_WIN32)
	g_chalet = fmt::format("{}/chalet.exe", path);
#else
	g_chalet = fmt::format("{}/chalet", path);
#endif

	return true;
}

const std::string& chaletExec()
{
	return g_chalet;
}
}
