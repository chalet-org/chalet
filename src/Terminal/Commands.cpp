/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Commands.hpp"

#include <array>
#include <chrono>
#include <sys/stat.h>
#include <thread>

#include "Libraries/WindowsApi.hpp"
#if defined(CHALET_WIN32)
	#include <shellapi.h>
	#include <winuser.h>
#endif

#include "Libraries/Glob.hpp"
#include "Process/ProcessController.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

#ifdef CHALET_MSVC
	#define popen _popen
	#define pclose _pclose
	#define stat _stat
#endif

namespace chalet
{
namespace
{
#if defined(CHALET_WIN32)
static struct
{
	std::string cygPath;
} state;
#elif defined(CHALET_MACOS)
static struct
{
	std::string xcodePath;
} state;
#endif

struct stat statBuffer;

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
	CHALET_TRY
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
	CHALET_CATCH(fs::filesystem_error & err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}

	CHALET_TRY
	{
		for (const auto& file : fs::directory_iterator(source))
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
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR(err.what());
	}

	return true;
}
}

/*****************************************************************************/
std::int64_t Commands::getLastWriteTime(const std::string& inFile)
{
	if (stat(inFile.c_str(), &statBuffer) == 0)
	{
		return statBuffer.st_mtime;
	}
	else
	{
		return 0;
	}
}

/*****************************************************************************/
std::string Commands::getWorkingDirectory()
{
	CHALET_TRY
	{
		auto cwd = fs::current_path();
		return cwd.string();
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return std::string();
	}
}

/*****************************************************************************/
bool Commands::changeWorkingDirectory(const std::string& inPath)
{
	CHALET_TRY
	{
		std::error_code errorCode;
		fs::current_path(inPath, errorCode);
		return errorCode.operator bool();
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
}

/*****************************************************************************/
bool Commands::pathIsFile(const std::string& inPath)
{
	CHALET_TRY
	{
		return fs::is_regular_file(inPath);
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
}

/*****************************************************************************/
bool Commands::pathIsDirectory(const std::string& inPath)
{
	CHALET_TRY
	{
		return fs::is_directory(inPath);
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
}

/*****************************************************************************/
bool Commands::pathIsSymLink(const std::string& inPath)
{
	CHALET_TRY
	{
		return fs::is_symlink(inPath);
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
}

/*****************************************************************************/
std::string Commands::getCanonicalPath(const std::string& inPath)
{
	CHALET_TRY
	{
		auto path = fs::canonical(inPath);
		std::string ret = path.string();
		Path::sanitize(ret);
		return ret;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return inPath;
	}
}

/*****************************************************************************/
std::string Commands::getAbsolutePath(const std::string& inPath)
{
	CHALET_TRY
	{
		auto path = fs::absolute(inPath);
		std::string ret = path.string();
		Path::sanitize(ret);
		return ret;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return inPath;
	}
}

/*****************************************************************************/
std::string Commands::getProximatePath(const std::string& inPath, const std::string& inBase)
{
	CHALET_TRY
	{
		auto path = fs::proximate(inPath, inBase);
		std::string ret = path.string();
		Path::sanitize(ret);
		return ret;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return inPath;
	}
}

/*****************************************************************************/
std::string Commands::resolveSymlink(const std::string& inPath)
{
	CHALET_TRY
	{
		auto path = fs::read_symlink(inPath);
		auto out = path.string();
		return out;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return inPath;
	}
}

/*****************************************************************************/
std::uintmax_t Commands::getPathSize(const std::string& inPath)
{
	CHALET_TRY
	{
		if (Output::showCommands())
			Output::printCommand(fmt::format("get directory size: {}", inPath));

		const auto path = fs::path{ inPath };
		std::uintmax_t ret = 0;
		if (fs::is_directory(path))
		{
			for (const auto& entry : fs::recursive_directory_iterator(path))
			{
				if (entry.is_regular_file())
				{
					ret += entry.file_size();
				}
			}
		}
		else
		{
			if (fs::is_regular_file(path))
			{
				ret = fs::file_size(path);
			}
		}

		return ret;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return 0;
	}
}

/*****************************************************************************/
bool Commands::makeDirectory(const std::string& inPath)
{
	CHALET_TRY
	{
		if (Output::showCommands())
			Output::printCommand(fmt::format("make directory: {}", inPath));

		if (!fs::create_directories(inPath))
			return false;

		return true;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
}

/*****************************************************************************/
bool Commands::makeDirectories(const StringList& inPaths, bool& outDirectoriesMade)
{
	outDirectoriesMade = false;
	bool result = true;
	for (auto& path : inPaths)
	{
		if (Commands::pathExists(path))
			continue;

		result &= Commands::makeDirectory(path);
		outDirectoriesMade = true;
	}

	return result;
}

/*****************************************************************************/
bool Commands::remove(const std::string& inPath)
{
	CHALET_TRY
	{
		if (!Commands::pathExists(inPath))
			return true;

		if (Output::showCommands())
			Output::printCommand(fmt::format("remove file: {}", inPath));

		bool result = fs::remove(inPath);
		return result;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
}

/*****************************************************************************/
bool Commands::removeRecursively(const std::string& inPath)
{
	CHALET_TRY
	{
		if (Output::showCommands())
			Output::printCommand(fmt::format("remove recursively: {}", inPath));

		bool result = fs::remove_all(inPath) > 0;
		return result;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
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
	CHALET_TRY
	{
		// if (inPath.front() == '/')
		// 	return false;

		if (Output::showCommands())
			Output::printCommand(fmt::format("set executable permission: {}", inPath));

		fs::permissions(inPath,
			fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
			fs::perm_options::add);

		return true;
	}
	CHALET_CATCH(const fs::filesystem_error&)
	{
		// std::string error(err.what());

		// std::cout.write(error.data(), error.size());
		// std::cout.put(std::cout.widen('\n'));
		// std::cout.flush();
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
	CHALET_TRY
	{
		if (Output::showCommands())
			Output::printCommand(fmt::format("create directory symlink: {} {}", inFrom, inTo));

		fs::create_directory_symlink(inFrom, inTo);

		return true;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
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
	CHALET_TRY
	{
		if (Output::showCommands())
			Output::printCommand(fmt::format("create symlink: {} {}", inFrom, inTo));

		fs::create_symlink(inFrom, inTo);

		return true;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
#endif
}

/*****************************************************************************/
bool Commands::copy(const std::string& inFrom, const std::string& inTo, const fs::copy_options inOptions)
{
	CHALET_TRY
	{
		fs::path from{ inFrom };
		fs::path to{ inTo / from.filename() };

		if (Output::showCommands())
			Output::printCommand(fmt::format("copy to path: {} {}", inFrom, inTo));
		else
			Output::msgCopying(inFrom, inTo);

		if (fs::is_directory(from))
			return copyDirectory(from, to, inOptions);
		else
			fs::copy(from, to, inOptions);

		return true;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
}

/*****************************************************************************/
bool Commands::copySilent(const std::string& inFrom, const std::string& inTo)
{
	CHALET_TRY
	{
		fs::path from{ inFrom };
		fs::path to{ inTo / from.filename() };

		if (Output::showCommands())
			Output::printCommand(fmt::format("copy to path: {} {}", inFrom, inTo));

		if (fs::is_directory(from))
			return copyDirectory(from, to, fs::copy_options::overwrite_existing);
		else
			fs::copy(from, to, fs::copy_options::overwrite_existing);

		return true;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
}

/*****************************************************************************/
bool Commands::copySkipExisting(const std::string& inFrom, const std::string& inTo)
{
	CHALET_TRY
	{
		fs::path from{ inFrom };
		fs::path to{ inTo / from.filename() };

		if (Output::showCommands())
			Output::printCommand(fmt::format("copy to path: {} {}", inFrom, inTo));
		else
			Output::msgCopying(inFrom, inTo);

		if (fs::is_directory(from))
			return copyDirectory(from, to, fs::copy_options::skip_existing);
		else
			fs::copy(from, to, fs::copy_options::skip_existing);

		return true;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
}

/*****************************************************************************/
bool Commands::copyRename(const std::string& inFrom, const std::string& inTo, const bool inSilent)
{
	CHALET_TRY
	{
		if (!inSilent)
		{
			if (Output::showCommands())
				Output::printCommand(fmt::format("copy: {} {}", inFrom, inTo));
			else
				Output::msgCopying(inFrom, inTo);
		}

		fs::copy(inFrom, inTo, fs::copy_options::overwrite_existing);

		return true;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
}

/*****************************************************************************/
bool Commands::rename(const std::string& inFrom, const std::string& inTo, const bool inSkipNonExisting)
{
	CHALET_TRY
	{
		if (Output::showCommands())
			Output::printCommand(fmt::format("rename: {} {}", inFrom, inTo));

		if (!Commands::pathExists(inFrom))
			return inSkipNonExisting;

		if (Commands::pathExists(inTo))
			fs::remove(inTo);

		fs::rename(inFrom, inTo);

		return true;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
}

/*****************************************************************************/
bool Commands::pathExists(const std::string& inFile)
{
	return stat(inFile.c_str(), &statBuffer) == 0;
}

/*****************************************************************************/
bool Commands::pathIsEmpty(const fs::path& inPath, const std::vector<fs::path>& inExceptions, const bool inCheckExists)
{
	CHALET_TRY
	{
		if (inCheckExists && !fs::exists(inPath))
			return false;

		if (!fs::is_directory(inPath))
		{
			CHALET_THROW(std::runtime_error("Not a directory"));
		}

		bool result = true;

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

				if (foundException)
					continue;
			}

			result = false;
			break;
		}

		return result;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
}

/*****************************************************************************/
// Should match:
//   https://www.digitalocean.com/community/tools/glob?comments=true&glob=src%2F%2A%2A%2F%2A.cpp&matches=false&tests=src&tests=src%2Fmain.cpp&tests=src%2Fpch.hpp&tests=src%2Ffoo&tests=src%2Ffoo%2Ffoo.cpp&tests=src%2Ffoo%2Ffoo.hpp&tests=src%2Fbar&tests=src%2Fbar%2Fbar&tests=src%2Fbar%2Fbar%2Fbar.cpp&tests=src%2Fbar%2Fbar%2Fbar.hpp
//
void Commands::forEachGlobMatch(const std::string& inPattern, const GlobMatch inSettings, const std::function<void(const fs::path&)>& onFound)
{
	if (onFound == nullptr)
		return;

	if (!String::contains('*', inPattern))
		return;

	auto matchIsValid = [&](const fs::path& inmatch) -> bool {
		bool isDirectory = fs::is_directory(inmatch);
		bool isRegularFile = fs::is_regular_file(inmatch);

		if (inSettings == GlobMatch::Files && isDirectory)
			return false;

		if (inSettings == GlobMatch::Folders && isRegularFile)
			return false;

		return isRegularFile || isDirectory;
	};

	bool recursive = true;
	bool dirOnly = inSettings == GlobMatch::Folders;

	if (String::contains("**/*", inPattern))
	{
		auto pattern = inPattern;
		// this will get the root
		String::replaceAll(pattern, "**/*", "*");

		for (auto& match : glob::glob(pattern, recursive, dirOnly))
		{
			if (matchIsValid(match))
				onFound(match);
		}
	}

	for (auto& match : glob::glob(inPattern, recursive, dirOnly))
	{
		if (matchIsValid(match))
			onFound(match);
	}
}

/*****************************************************************************/
void Commands::forEachGlobMatch(const StringList& inPatterns, const GlobMatch inSettings, const std::function<void(const fs::path&)>& onFound)
{
	for (auto& pattern : inPatterns)
	{
		forEachGlobMatch(pattern, inSettings, onFound);
	}
}

/*****************************************************************************/
void Commands::forEachGlobMatch(const std::string& inPath, const std::string& inPattern, const GlobMatch inSettings, const std::function<void(const fs::path&)>& onFound)
{
	forEachGlobMatch(fmt::format("{}/{}", inPath, inPattern), inSettings, onFound);
}

/*****************************************************************************/
void Commands::forEachGlobMatch(const std::string& inPath, const StringList& inPatterns, const GlobMatch inSettings, const std::function<void(const fs::path&)>& onFound)
{
	for (auto& pattern : inPatterns)
	{
		forEachGlobMatch(inPath, pattern, inSettings, onFound);
	}
}

/*****************************************************************************/
void Commands::addPathToListWithGlob(std::string&& inValue, StringList& outList, const GlobMatch inSettings)
{
	if (String::contains('*', inValue))
	{
		Commands::forEachGlobMatch(inValue, inSettings, [&](const fs::path& inPath) {
			auto path = inPath.string();
			Path::sanitize(path);

			List::addIfDoesNotExist(outList, std::move(path));
		});
	}
	else
	{
		List::addIfDoesNotExist(outList, std::move(inValue));
	}
}

/*****************************************************************************/
// TODO: This doesn't quite fit here
//
bool Commands::readFileAndReplace(const std::string& inFile, const std::function<void(std::string&)>& onReplace)
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
std::string Commands::readShebangFromFile(const std::string& inFile)
{
	std::string ret;

	if (Commands::pathExists(inFile))
	{
		std::ifstream file{ inFile };
		std::getline(file, ret);

		if (String::startsWith("#!", ret))
		{
			ret = ret.substr(2);

			if (String::startsWith("/usr/bin/env ", ret))
			{
				// we'll parse the rest later
			}
			else
			{
				auto space = ret.find_first_of(" ");
				if (space != std::string::npos)
					ret = std::string();
			}
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
bool Commands::subprocess(const StringList& inCmd, std::string inCwd, CreateSubprocessFunc inOnCreate, const PipeOption inStdOut, const PipeOption inStdErr)
{
	if (Output::showCommands())
		Output::printCommand(inCmd);

	chalet_assert(inStdOut != PipeOption::Pipe, "Commands::subprocess must implement onStdOut");
	chalet_assert(inStdErr != PipeOption::Pipe, "Commands::subprocess must implement onStdErr");

	ProcessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = inStdOut;
	options.stderrOption = inStdErr;
	options.onCreate = std::move(inOnCreate);

	return ProcessController::run(inCmd, options) == EXIT_SUCCESS;
}

/*****************************************************************************/
bool Commands::subprocessWithInput(const StringList& inCmd, std::string inCwd, CreateSubprocessFunc inOnCreate, const PipeOption inStdOut, const PipeOption inStdErr)
{
	if (Output::showCommands())
		Output::printCommand(inCmd);

	chalet_assert(inStdOut != PipeOption::Pipe, "Commands::subprocess must implement onStdOut");
	chalet_assert(inStdErr != PipeOption::Pipe, "Commands::subprocess must implement onStdErr");

	ProcessOptions options;
	options.cwd = std::move(inCwd);
	options.stdinOption = PipeOption::StdIn;
	options.stdoutOption = inStdOut;
	options.stderrOption = inStdErr;
	options.onCreate = std::move(inOnCreate);

	return ProcessController::run(inCmd, options) == EXIT_SUCCESS;
}

/*****************************************************************************/
std::string Commands::subprocessOutput(const StringList& inCmd, const PipeOption inStdOut, const PipeOption inStdErr)
{
	return subprocessOutput(inCmd, getWorkingDirectory(), inStdOut, inStdErr);
}

/*****************************************************************************/
std::string Commands::subprocessOutput(const StringList& inCmd, std::string inWorkingDirectory, const PipeOption inStdOut, const PipeOption inStdErr)
{
	if (Output::showCommands())
		Output::printCommand(inCmd);

	std::string ret;

	ProcessOptions options;
	options.cwd = std::move(inWorkingDirectory);
	options.stdoutOption = inStdOut;
	options.stderrOption = inStdErr;
	if (options.stdoutOption == PipeOption::Pipe)
	{
		options.onStdOut = [&ret](std::string inData) {
#if defined(CHALET_WIN32)
			String::replaceAll(inData, "\r\n", "\n");
#endif
			ret += std::move(inData);
		};
	}
	if (options.stderrOption == PipeOption::Pipe)
	{
		options.onStdErr = [&ret](std::string inData) {
#if defined(CHALET_WIN32)
			String::replaceAll(inData, "\r\n", "\n");
#endif
			ret += std::move(inData);
		};
	}

	UNUSED(ProcessController::run(inCmd, options));

	stripLastEndLine(ret);

	return ret;
}

/*****************************************************************************/
bool Commands::subprocessOutputToFile(const StringList& inCmd, const std::string& inOutputFile, const PipeOption inStdErr)
{
	if (Output::showCommands())
		Output::printCommand(inCmd);

	std::ofstream outputStream(inOutputFile);

	ProcessOptions options;
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

	bool result = ProcessController::run(inCmd, options) == EXIT_SUCCESS;
	outputStream << std::endl;
	return result;
}

/*****************************************************************************/
bool Commands::subprocessNinjaBuild(const StringList& inCmd, std::string inCwd)
{
	if (Output::showCommands())
		Output::printCommand(inCmd);

	struct
	{
		std::string data;
		std::string eol = String::eol();
		std::string endlineReplace = fmt::format("{}\n", Output::getAnsiStyle(Color::Reset));
	} cap;

	ProcessOptions::PipeFunc onStdOut = [&cap](std::string inData) -> void {
		String::replaceAll(inData, cap.eol, cap.endlineReplace);
		std::cout.write(inData.data(), inData.size());
		std::cout.flush();

		auto lineBreak = inData.find('\n');
		if (lineBreak == std::string::npos)
		{
			cap.data += std::move(inData);
		}
		else
		{
			cap.data += inData.substr(0, lineBreak + 1);
			auto tmp = inData.substr(lineBreak + 1);
			if (!tmp.empty())
			{
				cap.data = std::move(tmp);
			}
		}
	};

	ProcessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::StdErr;
	options.onStdOut = std::move(onStdOut);

	int result = ProcessController::run(inCmd, options);

	if (cap.data.size() > 0)
	{
		std::string noWork = fmt::format("ninja: no work to do.{}", cap.endlineReplace);
		if (String::endsWith(noWork, cap.data))
			Output::previousLine();
		else
			Output::lineBreak();
	}

	return result == EXIT_SUCCESS;
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
	if (inExecutable.empty())
		return std::string();

	std::string result;
#if defined(CHALET_WIN32)
	if (Output::showCommands())
		Output::printCommand(fmt::format("executable search: {}", inExecutable));

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
	{
		if (!Commands::pathExists(inExecutable)) // checks working dir
		{
			auto path = Environment::getPath();
			auto home = Environment::getUserDirectory();
			std::size_t start = 0;
			while (start != std::string::npos)
			{
				auto end = path.find(':', start);
				auto tmp = path.substr(start, end - start);
				while (tmp.back() == '/')
					tmp.pop_back();

				if (String::startsWith("~/", tmp))
				{
					tmp = fmt::format("{}/{}", home, tmp.substr(2));
				}

				result = fmt::format("{}/{}", tmp, inExecutable);
				if (Commands::pathExists(result))
					break;

				result.clear();
				start = end;
				if (start != std::string::npos)
					++start;
			}
		}
	}

	// which (original method) has issues when PATH is changed inside chalet - doesn't seem to inherit the env
	// StringList command;
	// command = { "which", inExecutable };

	// result = Commands::subprocessOutput(command);
	// if (String::contains("which: no", result))
	// 	return std::string();

	if (result.empty())
		return result;

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

#if defined(CHALET_WIN32)
/*****************************************************************************/
const std::string& Commands::getCygPath()
{
	if (state.cygPath.empty())
	{
		auto cygPath = Commands::which("cygpath");
		state.cygPath = Commands::subprocessOutput({ std::move(cygPath), "-m", "/" });
		Path::sanitize(state.cygPath, true);
		state.cygPath.pop_back();
	}

	return state.cygPath;
}
#endif

#if defined(CHALET_MACOS)
/*****************************************************************************/
const std::string& Commands::getXcodePath()
{
	if (state.xcodePath.empty())
	{
		auto xcodeSelect = "/usr/bin/xcode-select";
		state.xcodePath = Commands::subprocessOutput({ std::move(xcodeSelect), "-p" });
	}

	return state.xcodePath;
}
#endif
}
