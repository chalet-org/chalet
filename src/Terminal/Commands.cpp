/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Commands.hpp"

#include <array>

#include "Libraries/Format.hpp"
#include "Libraries/Glob.hpp"
#include "State/CommandLineInputs.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
#if defined(CHALET_MACOS)
std::string kXcodePath;
#endif
}
/*****************************************************************************/
std::string Commands::getWorkingDirectory()
{
	try
	{
		auto cwd = fs::current_path();
		return cwd.string();
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return std::string();
	}
}

/*****************************************************************************/
fs::path Commands::getWorkingDirectoryPath()
{
	try
	{
		auto cwd = fs::current_path();
		return cwd;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return fs::path{ "" };
	}
}

/*****************************************************************************/
std::string Commands::getCanonicalPath(const std::string& inPath)
{
	try
	{
		auto path = fs::canonical(inPath);
		std::string ret = path.string();
		Path::sanitize(ret);
		return ret;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return inPath;
	}
}

/*****************************************************************************/
std::string Commands::getAbsolutePath(const std::string& inPath)
{
	try
	{
		auto path = fs::absolute(inPath);
		std::string ret = path.string();
		Path::sanitize(ret);
		return ret;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return inPath;
	}
}

/*****************************************************************************/
bool Commands::makeDirectory(const std::string& inPath, const bool inCleanOutput)
{
	try
	{
		if (!inCleanOutput)
			Output::print(Color::blue, fmt::format("mkdir -p '{}'", inPath));

		fs::create_directories(inPath);

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
}

/*****************************************************************************/
bool Commands::makeDirectories(const StringList& inPaths, const bool inCleanOutput)
{
	bool result = true;
	for (auto& path : inPaths)
	{
		if (Commands::pathExists(path))
			continue;

		result &= Commands::makeDirectory(path, inCleanOutput);
	}

	return result;
}

/*****************************************************************************/
bool Commands::remove(const std::string& inPath, const bool inCleanOutput)
{
	try
	{
		if (!fs::exists(inPath))
			return true;

		if (!inCleanOutput)
			Output::print(Color::blue, fmt::format("rm -rf '{}'", inPath));

		bool result = fs::remove(inPath);
		return result;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
}

/*****************************************************************************/
bool Commands::removeRecursively(const std::string& inPath, const bool inCleanOutput)
{
	try
	{
		if (!inCleanOutput)
			Output::print(Color::blue, fmt::format("rm -rf '{}'", inPath));

		bool result = fs::remove_all(inPath) > 0;
		return result;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
}

/*****************************************************************************/
bool Commands::setExecutableFlag(const std::string& inPath, const bool inCleanOutput)
{
#if defined(CHALET_WIN32)
	UNUSED(inPath, inCleanOutput);
	return true;
#else
	try
	{
		if (!inCleanOutput)
			Output::print(Color::blue, fmt::format("chmod +x '{}'", inPath));

		fs::permissions(inPath,
			fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
			fs::perm_options::add);

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
#endif
}

/*****************************************************************************/
bool Commands::createDirectorySymbolicLink(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput)
{
#if defined(CHALET_WIN32)
	UNUSED(inFrom, inTo, inCleanOutput);
	return true;
#else
	try
	{
		if (!inCleanOutput)
			Output::print(Color::blue, fmt::format("ln -s '{}' '{}'", inFrom, inTo));

		fs::create_directory_symlink(inFrom, inTo);

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
#endif
}

/*****************************************************************************/
bool Commands::createSymbolicLink(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput)
{
#if defined(CHALET_WIN32)
	UNUSED(inFrom, inTo, inCleanOutput);
	return true;
#else
	try
	{
		if (!inCleanOutput)
			Output::print(Color::blue, fmt::format("ln -s '{}' '{}'", inFrom, inTo));

		fs::create_symlink(inFrom, inTo);

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
#endif
}

/*****************************************************************************/
bool Commands::copy(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput)
{
	try
	{
		fs::path from{ inFrom };
		fs::path to{ inTo / from.filename() };

		if (inCleanOutput)
			Output::msgCopying(inFrom, inTo);
		else
			Output::print(Color::blue, fmt::format("cp -r '{}' '{}'", inFrom, inTo));

		fs::copy(from, to, fs::copy_options::recursive);

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
}

/*****************************************************************************/
bool Commands::copyRename(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput)
{
	try
	{
		if (inCleanOutput)
			Output::msgCopying(inFrom, inTo);
		else
			Output::print(Color::blue, fmt::format("cp -r '{}' '{}'", inFrom, inTo));

		fs::copy(inFrom, inTo);

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
}

bool Commands::rename(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput)
{
	try
	{
		if (!inCleanOutput)
			Output::print(Color::blue, fmt::format("mv '{}' '{}'", inFrom, inTo));

		fs::rename(inFrom, inTo);

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
}

/*****************************************************************************/
bool Commands::pathExists(const fs::path& inPath)
{
	try
	{
		bool result = fs::exists(inPath);
		return result;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
}

/*****************************************************************************/
bool Commands::pathIsEmpty(const fs::path& inPath)
{
	try
	{
		if (!fs::is_directory(inPath))
		{
			throw std::runtime_error("Not a directory");
		}

		bool result = fs::is_empty(inPath);
		return result;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
	catch (const std::runtime_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
}

/*****************************************************************************/
void Commands::forEachFileMatch(const std::string& inPath, const std::string& inPattern, const std::function<void(const fs::path&)>& onFound)
{
	if (onFound == nullptr)
		return;

	for (auto& res : glob::rglob(fmt::format("{}/{}", inPath, inPattern)))
	{
		onFound(res);
	}
	for (auto& res : glob::rglob(fmt::format("{}/**/{}", inPath, inPattern)))
	{
		onFound(res);
	}
}

/*****************************************************************************/
void Commands::forEachFileMatch(const std::string& inPath, const StringList& inPatterns, const std::function<void(const fs::path&)>& onFound)
{
	for (auto& pattern : inPatterns)
	{
		forEachFileMatch(inPath, pattern, onFound);
	}
}

/*****************************************************************************/
// TODO: This doesn't quite fit here
//
bool Commands::readFileAndReplace(const fs::path& inFile, const std::function<void(std::string&)>& onReplace)
{
	if (!Commands::pathExists(inFile))
		return false;

	if (onReplace == nullptr)
		return false;

	std::ostringstream buffer;
	std::ifstream file{ inFile };
	buffer << file.rdbuf();

	std::string fileContents{ buffer.str() };

	onReplace(fileContents);

	std::ofstream(inFile) << fileContents;

	return true;
}

/*****************************************************************************/
bool Commands::createFileWithContents(const std::string& inFile, const std::string& inContents)
{
	std::ofstream(inFile) << inContents << std::endl;

	return true;
}

/*****************************************************************************/
bool Commands::shell(const std::string& inCmd, const bool inCleanOutput)
{
	// UNUSED(inCleanOutput);
	if (!inCleanOutput)
		Output::print(Color::blue, inCmd);

	bool result = std::system(inCmd.c_str()) == EXIT_SUCCESS;
	return result;
}

/*****************************************************************************/
bool Commands::shellAlternate(const std::string& inCmd, const bool inCleanOutput)
{
	if (!inCleanOutput)
		Output::print(Color::blue, inCmd);

#if defined(CHALET_WIN32)
	bool result = std::system(inCmd.c_str()) == EXIT_SUCCESS;
	return result;
#else
	// popen is about 4x faster than std::system
	auto cmd = fmt::format("{} 2>&1", inCmd);
	FILE* output = popen(cmd.c_str(), "r");
	if (output == nullptr)
	{
		throw std::runtime_error("popen() failed!");
	}

	return pclose(output) == 0;
#endif
}

/*****************************************************************************/
std::string Commands::shellWithOutput(const std::string& inCmd, const bool inCleanOutput)
{
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(inCmd.c_str(), "r"), pclose);

	if (!inCleanOutput)
		Output::print(Color::blue, inCmd);

	if (!pipe)
	{
		throw std::runtime_error("popen() failed!");
	}

	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
	{
		result += buffer.data();
	}

	return result;
}

/*****************************************************************************/
bool Commands::shellRemove(const std::string& inPath, const bool inCleanOutput)
{
	return Commands::shell(fmt::format("rm -rf '{}'", inPath), inCleanOutput);
}

/*****************************************************************************/
std::string Commands::which(const std::string_view& inExecutable, const bool inCleanOutput)
{
#if defined(CHALET_WIN32)
	const std::string null = "2> nul";
#else
	const std::string null = "2> /dev/null";
#endif
	std::string result = Commands::shellWithOutput(fmt::format("which {} {null}", inExecutable, FMT_ARG(null)), inCleanOutput);
	String::replaceAll(result, "\n", "");

#if defined(CHALET_WIN32)
	Path::msysDrivesToWindowsDrives(result);

	if (!result.empty() && !String::endsWith(".exe", result) && !String::startsWith("/", result))
		result += ".exe";

	// MINGW hell - there might be a better solution for this
	if (String::startsWith("/mingw64/bin/", result))
		String::replaceAll(result, "/mingw64/bin/", "");

	if (String::startsWith("/usr/bin/", result))
		String::replaceAll(result, "/usr/bin/", "");
#elif defined(CHALET_MACOS)

	if (String::startsWith("/usr/bin/", result))
	{
		auto& xcodePath = getXcodePath();
		std::string withXcodepath = xcodePath + result;
		if (Commands::pathExists(withXcodepath))
			result = std::move(withXcodepath);
	}
#endif
	return result;
}

/*****************************************************************************/
std::string Commands::testCompilerFlags(const std::string& inCompilerExec, const bool inCleanOutput)
{
	if (inCompilerExec.empty())
		return std::string();

#if defined(CHALET_WIN32)
	std::string command = fmt::format("{} -x c nul -dM -E", inCompilerExec);
#else
	std::string command = fmt::format("{} -x c /dev/null -dM -E | grep -e '__clang__' -e '__GNUC__' -e 'Apple LLVM'", inCompilerExec);
#endif
	std::string result = Commands::shellWithOutput(command, inCleanOutput);
	return result;
}

#if defined(CHALET_MACOS)
/*****************************************************************************/
const std::string& Commands::getXcodePath()
{
	if (kXcodePath.empty())
	{
		kXcodePath = Commands::shellWithOutput("xcode-select -p", true);
		String::replaceAll(kXcodePath, "\n", "");
	}

	return kXcodePath;
}
#endif
}
