/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Commands.hpp"

#include <array>
#include <thread>

#include "Libraries/WindowsApi.hpp"
#if defined(CHALET_WIN32)
	#include <shellapi.h>
	#include <winuser.h>
#endif

#include "Libraries/Format.hpp"
#include "Libraries/Glob.hpp"
#include "State/CommandLineInputs.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Subprocess.hpp"

#ifdef CHALET_MSVC
	#define popen _popen
	#define pclose _pclose
#endif

namespace chalet
{
namespace
{
#if defined(CHALET_WIN32)
std::string kCygPath;
#elif defined(CHALET_MACOS)
std::string kXcodePath;
#endif

/*****************************************************************************/
void stripLastEndLine(std::string& inString)
{
	if (!inString.empty() && inString.back() == '\n')
	{
		inString.pop_back();
#if defined(CHALET_WIN32)
		if (!inString.empty() && inString.back() == '\r')
			inString.pop_back();
#endif
	}
}

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
std::uintmax_t Commands::getPathSize(const std::string& inPath, const bool inCleanOutput)
{
	try
	{
		if (!inCleanOutput)
			Output::print(Color::Blue, fmt::format("get directory size: {}", inPath));

		const auto path = fs::path{ inPath };
		std::uintmax_t ret = 0;
		for (const fs::directory_entry& entry : fs::recursive_directory_iterator(path))
		{
			if (entry.is_regular_file())
			{
				ret += entry.file_size();
			}
		}

		return ret;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return 0;
	}
}

/*****************************************************************************/
bool Commands::makeDirectory(const std::string& inPath, const bool inCleanOutput)
{
	try
	{
		if (!inCleanOutput)
			Output::print(Color::Blue, fmt::format("make directory: {}", inPath));

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
			Output::print(Color::Blue, fmt::format("remove file: {}", inPath));

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
			Output::print(Color::Blue, fmt::format("remove recursively: {}", inPath));

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
		// if (inPath.front() == '/')
		// 	return false;

		if (!inCleanOutput)
			Output::print(Color::Blue, fmt::format("set executable permission: {}", inPath));

		fs::permissions(inPath,
			fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
			fs::perm_options::add);

		return true;
	}
	catch (const fs::filesystem_error&)
	{
		// std::cout << err.what() << std::endl;
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
			Output::print(Color::Blue, fmt::format("create directory symlink: {} {}", inFrom, inTo));

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
			Output::print(Color::Blue, fmt::format("create symlink: {} {}", inFrom, inTo));

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
			Output::print(Color::Blue, fmt::format("copy to path: {} {}", inFrom, inTo));

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
bool Commands::copySkipExisting(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput)
{
	try
	{
		fs::path from{ inFrom };
		fs::path to{ inTo / from.filename() };

		if (inCleanOutput)
			Output::msgCopying(inFrom, inTo);
		else
			Output::print(Color::Blue, fmt::format("copy to path: {} {}", inFrom, inTo));

		fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::skip_existing);

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
			Output::print(Color::Blue, fmt::format("copy: {} {}", inFrom, inTo));

		fs::copy(inFrom, inTo);

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		std::cout << err.what() << std::endl;
		return false;
	}
}

/*****************************************************************************/
bool Commands::rename(const std::string& inFrom, const std::string& inTo, const bool inCleanOutput)
{
	try
	{
		if (!inCleanOutput)
			Output::print(Color::Blue, fmt::format("rename: {} {}", inFrom, inTo));

		if (fs::exists(inTo))
			fs::remove(inTo);

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
bool Commands::pathIsEmpty(const fs::path& inPath, const bool inCheckExists)
{
	try
	{
		if (inCheckExists && !fs::exists(inPath))
			return false;

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
void Commands::forEachFileMatch(const std::string& inPattern, const std::function<void(const fs::path&)>& onFound)
{
	if (onFound == nullptr)
		return;

	for (auto& res : glob::rglob(inPattern))
	{
		onFound(res);
	}

	if (!String::contains('*', inPattern))
		return;

	std::string pattern = inPattern;
	String::replaceAll(pattern, "*", "**/*");
	for (auto& res : glob::rglob(pattern))
	{
		onFound(res);
	}
}

/*****************************************************************************/
void Commands::forEachFileMatch(const StringList& inPatterns, const std::function<void(const fs::path&)>& onFound)
{
	for (auto& pattern : inPatterns)
	{
		forEachFileMatch(pattern, onFound);
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
void Commands::sleep(const double inSeconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(inSeconds * 1000.0)));
}

/*****************************************************************************/
bool Commands::createFileWithContents(const std::string& inFile, const std::string& inContents)
{
	std::ofstream(inFile) << inContents << std::endl;

	return true;
}

/*****************************************************************************/
bool Commands::subprocess(const StringList& inCmd, std::string inCwd, CreateSubprocessFunc inOnCreate, const PipeOption inStdOut, const PipeOption inStdErr, EnvMap inEnvMap, const bool inCleanOutput)
{
	if (!inCleanOutput)
		Output::print(Color::Blue, inCmd);

	chalet_assert(inStdOut != PipeOption::Pipe, "Commands::subprocess must implement onStdOut");
	chalet_assert(inStdErr != PipeOption::Pipe, "Commands::subprocess must implement onStdErr");

	SubprocessOptions options;
	options.cwd = std::move(inCwd);
	options.env = std::move(inEnvMap);
	options.stdoutOption = inStdOut;
	options.stderrOption = inStdErr;
	options.onCreate = std::move(inOnCreate);

	return Subprocess::run(inCmd, std::move(options)) == EXIT_SUCCESS;
}

/*****************************************************************************/
std::string Commands::subprocessOutput(const StringList& inCmd, const bool inCleanOutput, const PipeOption inStdErr)
{
	if (!inCleanOutput)
		Output::print(Color::Blue, inCmd);

	std::string ret;

	SubprocessOptions options;
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = inStdErr;
	options.onStdOut = [&ret](std::string inData) {
		ret += std::move(inData);
	};
	if (options.stderrOption == PipeOption::Pipe)
	{
		options.onStdErr = options.onStdOut;
	}

	UNUSED(Subprocess::run(inCmd, std::move(options)));

	stripLastEndLine(ret);

	return ret;
}

/*****************************************************************************/
bool Commands::subprocessOutputToFile(const StringList& inCmd, const std::string& inOutputFile, const PipeOption inStdErr, const bool inCleanOutput)
{
	if (!inCleanOutput)
		Output::print(Color::Blue, inCmd);

	std::ofstream outputStream(inOutputFile);

	SubprocessOptions options;
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = inStdErr;
	options.onStdOut = [&outputStream](std::string inData) {
#if defined(CHALET_WIN32)
		String::replaceAll(inData, "\r\n", "\n");
#endif
		outputStream << std::move(inData);
	};
	if (options.stderrOption == PipeOption::Pipe)
	{
		options.onStdErr = options.onStdOut;
	}

	bool result = Subprocess::run(inCmd, std::move(options)) == EXIT_SUCCESS;
	outputStream << std::endl;
	return result;
}

/*****************************************************************************/
std::string Commands::which(const std::string& inExecutable, const bool inCleanOutput)
{
	StringList command;
	const bool isBash = Environment::isBash();

#if defined(CHALET_WIN32)
	if (isBash)
#endif
		command = { "which", inExecutable };
#if defined(CHALET_WIN32)
	else
		command = { "cmd.exe", "/c", "where", fmt::format("{}.exe", inExecutable) };
#endif

	std::string result = Commands::subprocessOutput(command, inCleanOutput);
	if (isBash && String::contains("which: no", result))
		return std::string();

#if defined(CHALET_WIN32)
	if (!isBash)
	{
		char eol = '\r';
		if (String::contains(eol, result))
		{
			const auto splitResult = String::split(result, eol);
			if (splitResult.size() > 1)
				result = splitResult[0];
		}
		String::replaceAll(result, '\\', '/');
		if (String::startsWith("INFO:", result))
			return std::string();
	}
	else
	{
		Path::msysDrivesToWindowsDrives(result);
	}

	if (String::startsWith('/', result))
	{
		auto& cygPath = getCygPath();
		std::string withCygPath = cygPath + result + ".exe";
		if (Commands::pathExists(withCygPath))
		{
			result = std::move(withCygPath);
		}
		else
		{
			String::replaceAll(withCygPath, ".exe", "");
			if (Commands::pathExists(withCygPath))
				result = std::move(withCygPath);
		}
	}
	else
	{
		if (!String::contains('.', result))
		{
			if (!result.empty() && !String::endsWith(".exe", result))
				result += ".exe";
		}
	}

#elif defined(CHALET_MACOS)
	if (String::startsWith("/usr/bin/", result))
	{
		auto& xcodePath = getXcodePath();
		std::string withXcodePath = xcodePath + result;
		if (Commands::pathExists(withXcodePath))
			result = std::move(withXcodePath);
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
	std::string null = "nul";
#else
	std::string null = "/dev/null";
#endif

	StringList command = { inCompilerExec, "-x", "c", std::move(null), "-dM", "-E" };
	auto result = Commands::subprocessOutput(command, inCleanOutput);

	return result;
}

#if defined(CHALET_WIN32)
/*****************************************************************************/
const std::string& Commands::getCygPath()
{
	if (kCygPath.empty())
	{
		kCygPath = Commands::subprocessOutput({ "cygpath", "-m", "/" });
		Path::sanitize(kCygPath, true);
		kCygPath.pop_back();
	}

	return kCygPath;
}
#endif

#if defined(CHALET_MACOS)
/*****************************************************************************/
const std::string& Commands::getXcodePath()
{
	if (kXcodePath.empty())
	{
		kXcodePath = Commands::subprocessOutput({ "xcode-select", "-p" });
	}

	return kXcodePath;
}
#endif
}
