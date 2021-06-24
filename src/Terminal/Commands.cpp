/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Commands.hpp"

#include <array>
#include <chrono>
#include <thread>

#include "Libraries/WindowsApi.hpp"
#if defined(CHALET_WIN32)
	#include <shellapi.h>
	#include <winuser.h>
#endif

#include "Core/CommandLineInputs.hpp"

#include "Libraries/Glob.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
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

template <typename T>
std::time_t timePointToTime(T tp)
{
	using system_clock = std::chrono::system_clock;
	auto sctp = std::chrono::time_point_cast<system_clock::duration>(tp - T::clock::now() + system_clock::now());
	return system_clock::to_time_t(sctp);
}

/*****************************************************************************/
// NOTE: fs::copy_options::recursive follows all symlinks (bad!)
//   This is a custom version that more or less does the same thing,
//   but preserves symlinks (needed for copying frameworks)
//
bool copyDirectory(const fs::path& source, const fs::path& dest, fs::copy_options inOptions)
{
	try
	{
		if (!fs::exists(source) || !fs::is_directory(source))
		{
			Diagnostic::error("Source directory {} does not exist or is not a directory.", source.string());
			return false;
		}
		if (fs::exists(dest))
		{
			Diagnostic::error("Destination directory {} already exists.", dest.string());
			return false;
		}
		if (!fs::create_directory(dest))
		{
			Diagnostic::error("Unable to create destination directory {}", dest.string());
			return false;
		}
	}
	catch (fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}

	for (const auto& file : fs::directory_iterator(source))
	{
		try
		{
			const auto& current = file.path();
			if (file.is_symlink())
			{
				fs::copy_options options = inOptions | fs::copy_options::copy_symlinks;
				fs::copy(current, dest / current.filename(), options);
			}
			else if (file.is_directory())
			{
				if (!copyDirectory(current, dest / current.filename(), inOptions))
					return false;
			}
			else
			{
				fs::copy(current, dest / current.filename(), inOptions);
			}
		}
		catch (const fs::filesystem_error& err)
		{
			Diagnostic::error(err.what());
		}
	}

	return true;
}
}

/*****************************************************************************/
std::int64_t Commands::getLastWriteTime(const std::string& inFile)
{
	try
	{
		auto lastWrite = fs::last_write_time(inFile);
		auto outTime = timePointToTime(lastWrite);
		return static_cast<std::int64_t>(outTime);
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return 0;
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
		Diagnostic::error(err.what());
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
		Diagnostic::error(err.what());
		return fs::path{ "" };
	}
}

/*****************************************************************************/
bool Commands::changeWorkingDirectory(const std::string& inPath)
{
	try
	{
		std::error_code errorCode;
		fs::current_path(inPath, errorCode);
		return errorCode.operator bool();
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}
}

/*****************************************************************************/
bool Commands::pathIsFile(const std::string& inPath)
{
	try
	{
		return fs::is_regular_file(inPath);
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}
}

/*****************************************************************************/
bool Commands::pathIsDirectory(const std::string& inPath)
{
	try
	{
		return fs::is_directory(inPath);
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}
}

/*****************************************************************************/
bool Commands::pathIsSymLink(const std::string& inPath)
{
	try
	{
		return fs::is_symlink(inPath);
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
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
		Diagnostic::error(err.what());
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
		Diagnostic::error(err.what());
		return inPath;
	}
}

/*****************************************************************************/
std::string Commands::resolveSymlink(const std::string& inPath)
{
	try
	{
		auto path = fs::read_symlink(inPath);
		auto out = path.string();
		return out;
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return inPath;
	}
}

/*****************************************************************************/
std::uintmax_t Commands::getPathSize(const std::string& inPath)
{
	try
	{
		if (Output::showCommands())
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
		Diagnostic::error(err.what());
		return 0;
	}
}

/*****************************************************************************/
bool Commands::makeDirectory(const std::string& inPath)
{
	try
	{
		if (Output::showCommands())
			Output::print(Color::Blue, fmt::format("make directory: {}", inPath));

		fs::create_directories(inPath);

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}
}

/*****************************************************************************/
bool Commands::makeDirectories(const StringList& inPaths)
{
	bool result = true;
	for (auto& path : inPaths)
	{
		if (Commands::pathExists(path))
			continue;

		result &= Commands::makeDirectory(path);
	}

	return result;
}

/*****************************************************************************/
bool Commands::remove(const std::string& inPath)
{
	try
	{
		if (!fs::exists(inPath))
			return true;

		if (Output::showCommands())
			Output::print(Color::Blue, fmt::format("remove file: {}", inPath));

		bool result = fs::remove(inPath);
		return result;
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}
}

/*****************************************************************************/
bool Commands::removeRecursively(const std::string& inPath)
{
	try
	{
		if (Output::showCommands())
			Output::print(Color::Blue, fmt::format("remove recursively: {}", inPath));

		bool result = fs::remove_all(inPath) > 0;
		return result;
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}
}

/*****************************************************************************/
bool Commands::setExecutableFlag(const std::string& inPath)
{
#if defined(CHALET_WIN32)
	UNUSED(inPath);
	return true;
#else
	try
	{
		// if (inPath.front() == '/')
		// 	return false;

		if (Output::showCommands())
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
bool Commands::createDirectorySymbolicLink(const std::string& inFrom, const std::string& inTo)
{
#if defined(CHALET_WIN32)
	UNUSED(inFrom, inTo);
	return true;
#else
	try
	{
		if (Output::showCommands())
			Output::print(Color::Blue, fmt::format("create directory symlink: {} {}", inFrom, inTo));

		fs::create_directory_symlink(inFrom, inTo);

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}
#endif
}

/*****************************************************************************/
bool Commands::createSymbolicLink(const std::string& inFrom, const std::string& inTo)
{
#if defined(CHALET_WIN32)
	UNUSED(inFrom, inTo);
	return true;
#else
	try
	{
		if (Output::showCommands())
			Output::print(Color::Blue, fmt::format("create symlink: {} {}", inFrom, inTo));

		fs::create_symlink(inFrom, inTo);

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}
#endif
}

/*****************************************************************************/
bool Commands::copy(const std::string& inFrom, const std::string& inTo, const fs::copy_options inOptions)
{
	try
	{
		fs::path from{ inFrom };
		fs::path to{ inTo / from.filename() };

		if (Output::showCommands())
			Output::print(Color::Blue, fmt::format("copy to path: {} {}", inFrom, inTo));
		else
			Output::msgCopying(inFrom, inTo);

		if (fs::is_directory(from))
			return copyDirectory(from, to, inOptions);

		fs::copy(from, to, inOptions);
		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}
}

/*****************************************************************************/
bool Commands::copySilent(const std::string& inFrom, const std::string& inTo)
{
	try
	{
		fs::path from{ inFrom };
		fs::path to{ inTo / from.filename() };

		if (fs::is_directory(from))
			return copyDirectory(from, to, fs::copy_options::overwrite_existing);

		fs::copy(from, to, fs::copy_options::overwrite_existing);
		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}
}

/*****************************************************************************/
bool Commands::copySkipExisting(const std::string& inFrom, const std::string& inTo)
{
	try
	{
		fs::path from{ inFrom };
		fs::path to{ inTo / from.filename() };

		if (Output::showCommands())
			Output::print(Color::Blue, fmt::format("copy to path: {} {}", inFrom, inTo));
		else
			Output::msgCopying(inFrom, inTo);

		if (fs::is_directory(from))
			return copyDirectory(from, to, fs::copy_options::skip_existing);

		fs::copy(from, to, fs::copy_options::skip_existing);
		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}
}

/*****************************************************************************/
bool Commands::copyRename(const std::string& inFrom, const std::string& inTo)
{
	try
	{
		if (Output::showCommands())
			Output::print(Color::Blue, fmt::format("copy: {} {}", inFrom, inTo));
		else
			Output::msgCopying(inFrom, inTo);

		fs::copy(inFrom, inTo);

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}
}

/*****************************************************************************/
bool Commands::rename(const std::string& inFrom, const std::string& inTo, const bool inSkipNonExisting)
{
	try
	{
		if (Output::showCommands())
			Output::print(Color::Blue, fmt::format("rename: {} {}", inFrom, inTo));

		if (!fs::exists(inFrom))
			return inSkipNonExisting;

		if (fs::exists(inTo))
			fs::remove(inTo);

		fs::rename(inFrom, inTo);

		return true;
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
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
		Diagnostic::error(err.what());
		return false;
	}
}

/*****************************************************************************/
bool Commands::pathIsEmpty(const fs::path& inPath, const std::vector<fs::path>& inExceptions, const bool inCheckExists)
{
	try
	{
		if (inCheckExists && !fs::exists(inPath))
			return false;

		if (!fs::is_directory(inPath))
		{
			throw std::runtime_error("Not a directory");
		}

		bool result = fs::is_directory(inPath);
		if (!result)
			return false;

		auto dirEnd = fs::directory_iterator();
		for (auto it = fs::directory_iterator(inPath); it != dirEnd; ++it)
		{
			auto item = *it;
			if (item.is_directory() || item.is_regular_file())
			{
				const auto& path = item.path();
				const auto stem = path.stem().string();

				bool foundException = false;
				for (auto& excep : inExceptions)
				{
					if (excep.stem().string() == stem)
					{
						foundException = true;
						break;
					}
				}
				if (!foundException)
				{
					result = false;
					break;
				}
			}
			else
			{
				result = false;
				break;
			}
		}

		return result;
	}
	catch (const fs::filesystem_error& err)
	{
		Diagnostic::error(err.what());
		return false;
	}
	catch (const std::runtime_error& err)
	{
		Diagnostic::error(err.what());
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
std::string Commands::readShebangFromFile(const fs::path& inFile)
{
	std::string ret;

	if (Commands::pathExists(inFile))
	{
		std::ifstream file{ inFile };
		std::getline(file, ret);

		if (String::startsWith("#!", ret))
		{
			ret = ret.substr(2);

			// TODO: For now, don't support this kind of stuff
			// example: https://github.com/google/zx
			auto space = ret.find_first_of(" ");
			if (space != std::string::npos)
				ret = std::string();
		}
		else
			ret = std::string();
	}

	return ret;
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
bool Commands::subprocess(const StringList& inCmd, std::string inCwd, CreateSubprocessFunc inOnCreate, const PipeOption inStdOut, const PipeOption inStdErr, EnvMap inEnvMap)
{
	if (Output::showCommands())
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
std::string Commands::subprocessOutput(const StringList& inCmd, const PipeOption inStdErr)
{
	if (Output::showCommands())
		Output::print(Color::Blue, inCmd);

	std::string ret;

	SubprocessOptions options;
	options.cwd = getWorkingDirectory();
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
bool Commands::subprocessOutputToFile(const StringList& inCmd, const std::string& inOutputFile, const PipeOption inStdErr)
{
	if (Output::showCommands())
		Output::print(Color::Blue, inCmd);

	std::ofstream outputStream(inOutputFile);

	SubprocessOptions options;
	options.cwd = getWorkingDirectory();
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
std::string Commands::isolateVersion(const std::string& outString)
{
	std::string ret = outString;

	auto firstEol = ret.find('\n');
	if (firstEol != std::string::npos)
	{
		ret = ret.substr(0, firstEol);
	}

	auto lastSpace = ret.find_last_of(' ');
	if (lastSpace != std::string::npos)
	{
		ret = ret.substr(lastSpace + 1);
	}

	return ret;
}

/*****************************************************************************/
std::string Commands::which(const std::string& inExecutable)
{
	std::string result;
#if defined(CHALET_WIN32)
	if (Output::showCommands())
		Output::print(Color::Blue, fmt::format("executable search: {}", inExecutable));

	LPSTR lpFilePart;
	char filename[MAX_PATH];

	std::string extension{ ".exe" };
	if (String::contains('.', inExecutable))
	{
		auto pos = inExecutable.find_last_of('.');
		extension = inExecutable.substr(pos);
	}

	if (SearchPathA(NULL, inExecutable.c_str(), extension.c_str(), MAX_PATH, filename, &lpFilePart))
	{
		result = std::string(filename);
		String::replaceAll(result, '\\', '/');
	}
#else
	StringList command;
	command = { "which", inExecutable };

	result = Commands::subprocessOutput(command);
	if (String::contains("which: no", result))
		return std::string();

	#if defined(CHALET_MACOS)
	if (String::startsWith("/usr/bin/", result))
	{
		auto& xcodePath = getXcodePath();
		std::string withXcodePath = xcodePath + result;
		if (Commands::pathExists(withXcodePath))
		{
			result = std::move(withXcodePath);
		}
		else
		{
			withXcodePath = fmt::format("{}/Toolchains/XcodeDefault.xctoolchain{}", xcodePath, result);
			if (Commands::pathExists(withXcodePath))
			{
				result = std::move(withXcodePath);
			}
		}
	}
	#endif
#endif

	return result;
}

/*****************************************************************************/
std::string Commands::testCompilerFlags(const std::string& inCompilerExec)
{
	if (inCompilerExec.empty())
		return std::string();

#if defined(CHALET_WIN32)
	std::string null = "nul";
#else
	std::string null = "/dev/null";
#endif

	StringList command = { inCompilerExec, "-x", "c", std::move(null), "-dM", "-E" };
	auto result = Commands::subprocessOutput(command);

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
