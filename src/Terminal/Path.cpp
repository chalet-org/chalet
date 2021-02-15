/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Path.hpp"

#include "Libraries/Format.hpp"
#include "Libraries/Regex.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
void Path::sanitize(std::string& outValue, const bool inRemoveNewLine)
{
	if (outValue.empty())
		return;

	if (inRemoveNewLine)
		String::replaceAll(outValue, "\n", " ");

	// if (Environment::isBash())
	{
		String::replaceAll(outValue, "\\\\", "/");
		String::replaceAll(outValue, "\\", "/");
	}
	// 	else
	// 	{
	// #if defined(CHALET_WIN32)
	// 		String::replaceAll(outValue, "/", "\\\\");
	// #else
	// 		chalet_assert(false, "Bad call to Path::sanitize");
	// #endif
	// 	}

	if (outValue.back() == ' ')
		outValue.pop_back();
}

/*****************************************************************************/
// In theory, give this any path (including full PATH variable)
//
void Path::sanitizeWithDrives(std::string& outPath)
{
	Path::windowsDrivesToMsysDrives(outPath);
	Path::sanitize(outPath, true);

	String::replaceAll(outPath, ';', ':');
}

/*****************************************************************************/
void Path::windowsDrivesToMsysDrives(std::string& outPath)
{
#ifndef CHALET_MSVC
	static constexpr auto driveFwd = ctll::fixed_string{ "^([A-Za-z]):/.*" };
	static constexpr auto driveBck = ctll::fixed_string{ "^([A-Za-z]):\\\\.*" };

	if (auto m = ctre::match<driveBck>(outPath))
	{
		auto capture = m.get<1>().to_string();
		String::replaceAll(outPath, capture + ":\\", "/" + String::toLowerCase(capture) + "/");
	}

	if (auto m = ctre::match<driveFwd>(outPath))
	{
		auto capture = m.get<1>().to_string();
		String::replaceAll(outPath, capture + ":/", "/" + String::toLowerCase(capture) + "/");
	}
#else
	static std::regex driveFwd{ "^([A-Za-z]):/.*" };
	static std::regex driveBck{ "^([A-Za-z]):\\\\.*" };

	if (std::smatch m; std::regex_match(outPath, m, driveBck))
	{
		auto capture = m[1].str();
		String::replaceAll(outPath, capture + ":\\", "/" + String::toLowerCase(capture) + "/");
	}

	if (std::smatch m; std::regex_match(outPath, m, driveFwd))
	{
		auto capture = m[1].str();
		String::replaceAll(outPath, capture + ":/", "/" + String::toLowerCase(capture) + "/");
	}
#endif
}

/*****************************************************************************/
void Path::msysDrivesToWindowsDrives(std::string& outPath)
{
#ifndef CHALET_MSVC
	static constexpr auto driveFwd = ctll::fixed_string{ "^/([A-Za-z])/.*" };

	if (auto m = ctre::match<driveFwd>(outPath))
	{
		auto capture = m.get<1>().to_string();
		String::replaceAll(outPath, fmt::format("/{}/", capture), fmt::format("{}:/", String::toUpperCase(capture)));
	}
#else
	static std::regex driveFwd{ "^/([A-Za-z])/.*" };

	if (std::smatch m; std::regex_match(outPath, m, driveFwd))
	{
		auto capture = m[1].str();
		String::replaceAll(outPath, fmt::format("/{}/", capture), fmt::format("{}:/", String::toUpperCase(capture)));
	}
#endif
}

/*****************************************************************************/
StringList Path::getOSPaths()
{
#if !defined(CHALET_WIN32)
	return {
		"/usr/local/sbin",
		"/usr/local/bin",
		"/usr/sbin",
		"/usr/bin",
		"/sbin",
		"/bin"
	};
#endif

	return {};
}

}
