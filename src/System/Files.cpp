/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "System/Files.hpp"

#include <array>
#include <chrono>
#include <regex>
#include <sys/stat.h>
#include <thread>

#if defined(CHALET_WIN32)
	#include "Libraries/WindowsApi.hpp"

	#include <shellapi.h>
	#include <winuser.h>
#endif

#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

#if defined(CHALET_MSVC)
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
bool copyDirectory(const fs::path& source, const fs::path& dest, fs::copy_options inOptions, const bool inFailExists)
{
	CHALET_TRY
	{
		if (!fs::exists(source) || !fs::is_directory(source))
		{
			Diagnostic::error("Source directory {} does not exist or is not a directory.", source.string());
			return false;
		}
		if (inFailExists)
		{
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
		else
		{
			if (!fs::exists(dest) && !fs::create_directory(dest))
			{
				Diagnostic::error("Unable to create destination directory {}", dest.string());
				return false;
			}
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
				if (!copyDirectory(current, dest / current.filename(), inOptions, inFailExists))
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
std::string Files::getPlatformExecutableExtension()
{
#if defined(CHALET_WIN32)
	return ".exe";
#else
	return std::string();
#endif
}

/*****************************************************************************/
std::string Files::getPlatformSharedLibraryExtension()
{
#if defined(CHALET_WIN32)
	return ".dll";
#elif defined(CHALET_MACOS)
	return ".dylib";
#else
	return ".so";
#endif
}

/*****************************************************************************/
std::string Files::getPlatformFrameworkExtension()
{
#if defined(CHALET_MACOS)
	return ".framework";
#else
	return std::string();
#endif
}

/*****************************************************************************/
i64 Files::getLastWriteTime(const std::string& inFile)
{
	if (::stat(inFile.c_str(), &statBuffer) == 0)
	{
		return statBuffer.st_mtime;
	}
	else
	{
		return 0;
	}
}

/*****************************************************************************/
std::string Files::getWorkingDirectory()
{
	std::error_code error;
	auto cwd = fs::current_path(error);

	if (!error)
		return cwd.string();
	else
		return std::string();
}

/*****************************************************************************/
bool Files::changeWorkingDirectory(const std::string& inPath)
{
	std::error_code error;
	fs::current_path(inPath, error);
	return !error;
}

/*****************************************************************************/
bool Files::pathIsFile(const std::string& inPath)
{
	std::error_code error;
	bool result = fs::is_regular_file(inPath, error);
	return !error && result;
}

/*****************************************************************************/
bool Files::pathIsDirectory(const std::string& inPath)
{
	std::error_code error;
	bool result = fs::is_directory(inPath, error);
	return !error && result;
}

/*****************************************************************************/
bool Files::pathIsSymLink(const std::string& inPath)
{
	std::error_code error;
	bool result = fs::is_symlink(inPath, error);
	return !error && result;
}

/*****************************************************************************/
std::string Files::getCanonicalPath(const std::string& inPath)
{
	// This method doesn't care if the path is real or not, so weakly_canonical is used
	std::error_code error;
	auto path = fs::weakly_canonical(inPath, error);
	if (!error)
	{
		std::string ret = path.string();
		Path::toUnix(ret);
		return ret;
	}
	else
	{
		Diagnostic::error(error.message());
		return inPath;
	}
}

/*****************************************************************************/
std::string Files::getAbsolutePath(const std::string& inPath)
{
	std::error_code error;
	auto path = fs::absolute(inPath, error);
	if (!error)
	{
		std::string ret = path.string();
		Path::toUnix(ret);
		return ret;
	}
	else
	{
		Diagnostic::error(error.message());
		return inPath;
	}
}

/*****************************************************************************/
std::string Files::getProximatePath(const std::string& inPath, const std::string& inBase)
{
	std::error_code error;
	auto path = fs::proximate(inPath, inBase, error);
	if (!error)
	{
		std::string ret = path.string();
		Path::toUnix(ret);
		return ret;
	}
	else
	{
		Diagnostic::error(error.message());
		return inPath;
	}
}

/*****************************************************************************/
std::string Files::resolveSymlink(const std::string& inPath)
{
	std::error_code error;
	auto path = fs::read_symlink(inPath, error);
	if (!error)
	{
		return path.string();
	}
	else
	{
		Diagnostic::error(error.message());
		return inPath;
	}
}

/*****************************************************************************/
uintmax_t Files::getPathSize(const std::string& inPath)
{
	CHALET_TRY
	{
		if (Output::showCommands())
			Output::printCommand(fmt::format("get directory size: {}", inPath));

		auto path = fs::path{ inPath };
		uintmax_t ret = 0;
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
bool Files::makeDirectory(const std::string& inPath)
{
	if (Output::showCommands())
		Output::printCommand(fmt::format("make directory: {}", inPath));

	std::error_code error;
	if (!fs::create_directories(inPath, error))
		return false;

	if (error)
		Diagnostic::error(error.message());

	return !error;
}

/*****************************************************************************/
bool Files::makeDirectories(const StringList& inPaths, bool& outDirectoriesMade)
{
	outDirectoriesMade = false;
	bool result = true;
	for (auto& path : inPaths)
	{
		if (Files::pathExists(path))
			continue;

		result &= Files::makeDirectory(path);
		outDirectoriesMade = true;
	}

	return result;
}

/*****************************************************************************/
bool Files::remove(const std::string& inPath)
{
	if (!Files::pathExists(inPath))
		return true;

	if (Output::showCommands())
		Output::printCommand(fmt::format("remove path: {}", inPath));

	std::error_code error;
	bool result = fs::remove(inPath, error);
	if (error)
		Diagnostic::error(error.message());

	return !error && result;
}

/*****************************************************************************/
bool Files::removeRecursively(const std::string& inPath)
{
	if (Output::showCommands())
		Output::printCommand(fmt::format("remove recursively: {}", inPath));

	std::error_code error;
	bool result = fs::remove_all(inPath, error) > 0;
	if (error)
		Diagnostic::error(error.message());

	return !error && result;
}

/*****************************************************************************/
bool Files::setExecutableFlag(const std::string& inPath)
{
#if defined(CHALET_WIN32)
	UNUSED(inPath);
	return true;
#else
	// if (inPath.front() == '/')
	// 	return false;

	if (Output::showCommands())
		Output::printCommand(fmt::format("set executable permission: {}", inPath));

	std::error_code error;
	fs::permissions(inPath,
		fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
		fs::perm_options::add,
		error);

	if (error)
		Diagnostic::error(error.message());

	return !error;
#endif
}

/*****************************************************************************/
bool Files::createDirectorySymbolicLink(const std::string& inFrom, const std::string& inTo)
{
#if defined(CHALET_WIN32)
	UNUSED(inFrom, inTo);
	return true;
#else
	if (Output::showCommands())
		Output::printCommand(fmt::format("create directory symlink: {} -> {}", inFrom, inTo));

	std::error_code error;
	fs::create_directory_symlink(inFrom, inTo, error);
	if (error)
		Diagnostic::error(error.message());

	return !error;
#endif
}

/*****************************************************************************/
bool Files::createSymbolicLink(const std::string& inFrom, const std::string& inTo)
{
#if defined(CHALET_WIN32)
	UNUSED(inFrom, inTo);
	return true;
#else
	if (Output::showCommands())
		Output::printCommand(fmt::format("create symlink: {} -> {}", inFrom, inTo));

	std::error_code error;
	fs::create_symlink(inFrom, inTo, error);
	if (error)
		Diagnostic::error(error.message());

	return !error;
#endif
}

/*****************************************************************************/
bool Files::copy(const std::string& inFrom, const std::string& inTo, const fs::copy_options inOptions)
{
	CHALET_TRY
	{
		fs::path from{ inFrom };
		fs::path to{ inTo / from.filename() };

		if (Output::showCommands())
			Output::printCommand(fmt::format("copy to path: {} -> {}", inFrom, inTo));
		else
			Output::msgCopying(inFrom, fmt::format("{}/{}", inTo, String::getPathFilename(inFrom)));

		if (fs::is_directory(from))
			return copyDirectory(from, to, inOptions, true);
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
bool Files::copySilent(const std::string& inFrom, const std::string& inTo, const fs::copy_options inOptions)
{
	CHALET_TRY
	{
		fs::path from{ inFrom };
		fs::path to{ inTo / from.filename() };

		if (Output::showCommands())
			Output::printCommand(fmt::format("copy to path: {} -> {}", inFrom, inTo));

		if (fs::is_directory(from))
			return copyDirectory(from, to, inOptions, false);
		else
			fs::copy(from, to, inOptions);

		return true;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
}

/*****************************************************************************/
bool Files::copyRename(const std::string& inFrom, const std::string& inTo, const bool inSilent)
{
	if (!inSilent)
	{
		if (Output::showCommands())
			Output::printCommand(fmt::format("copy: {} -> {}", inFrom, inTo));
		else
			Output::msgCopying(inFrom, inTo);
	}

	std::error_code error;
	fs::copy(inFrom, inTo, fs::copy_options::overwrite_existing, error);
	if (error)
		Diagnostic::error(error.message());

	return !error;
}

/*****************************************************************************/
bool Files::moveSilent(const std::string& inFrom, const std::string& inTo, const fs::copy_options inOptions)
{
	CHALET_TRY
	{
		fs::path from{ inFrom };
		fs::path to{ inTo };

		if (Output::showCommands())
			Output::printCommand(fmt::format("move to path: {} -> {}", inFrom, inTo));

		if (fs::is_directory(from))
		{
			return copyDirectory(from, to, inOptions, false);
		}
		else
		{
			fs::copy(from, to, inOptions);
			fs::remove(from);
		}

		return true;
	}
	CHALET_CATCH(const fs::filesystem_error& err)
	{
		CHALET_EXCEPT_ERROR(err.what())
		return false;
	}
}

/*****************************************************************************/
bool Files::rename(const std::string& inFrom, const std::string& inTo, const bool inSkipNonExisting)
{
	CHALET_TRY
	{
		if (Output::showCommands())
			Output::printCommand(fmt::format("rename: {} -> {}", inFrom, inTo));

		if (!Files::pathExists(inFrom))
			return inSkipNonExisting;

		if (Files::pathExists(inTo))
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
bool Files::pathExists(const std::string& inFile)
{
#if defined(CHALET_WIN32)
	// std::error_code ec;
	// return fs::exists(inFile, ec);
	return ::GetFileAttributesA(inFile.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
	return stat(inFile.c_str(), &statBuffer) == 0;
#endif
}

/*****************************************************************************/
bool Files::pathIsEmpty(const std::string& inPath, const std::vector<fs::path>& inExceptions)
{
	CHALET_TRY
	{
		fs::path path(inPath);
		if (!fs::exists(path))
			return false;

		bool result = true;

		auto dirEnd = fs::directory_iterator();
		for (auto it = fs::directory_iterator(path); it != dirEnd; ++it)
		{
			auto item = *it;
			if (item.is_directory() || item.is_regular_file())
			{
				const auto& itemPath = item.path();
				const auto stem = itemPath.stem().string();

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
bool Files::forEachGlobMatch(const std::string& inPattern, const GlobMatch inSettings, const GlobCallback& onFound)
{
	if (onFound == nullptr)
		return false;

	if (String::contains("${", inPattern))
		return false;

	std::string basePath;
	auto pos = inPattern.find_first_of("*{");
	if (pos != std::string::npos)
	{
		auto tmp = inPattern.substr(0, pos);
		basePath = String::getPathFolder(tmp);
		if (basePath.empty())
			basePath = std::move(tmp);
	}

	if (basePath.empty())
	{
		basePath = Files::getWorkingDirectory();
		Path::toUnix(basePath);
	}

	if (!Files::pathIsDirectory(basePath))
		return false;

	auto pattern = inPattern;
	String::replaceAll(pattern, '(', "\\(");
	String::replaceAll(pattern, ')', "\\)");

	size_t start = 0;
	start = pattern.find("{", start);
	while (start != std::string::npos)
	{
		auto prefix = pattern.substr(0, start);
		start += 1;
		auto end = pattern.find('}', start);
		if (end != std::string::npos)
		{
			auto suffix = pattern.substr(end + 1);
			auto arr = pattern.substr(start, end - start);

			String::replaceAll(arr, ',', '|');
			pattern = fmt::format("{}({}){}", prefix, arr, suffix);
		}
		start = pattern.find('{', start);
	}

	Path::toUnix(pattern);
	String::replaceAll(pattern, '{', "\\{");
	String::replaceAll(pattern, '}', "\\}");
	String::replaceAll(pattern, '[', "\\[");
	String::replaceAll(pattern, '[', "\\]");
	String::replaceAll(pattern, '.', "\\.");
	String::replaceAll(pattern, '+', "\\+");
	String::replaceAll(pattern, '?', '.');
	String::replaceAll(pattern, "**/*", "(.+)");
	String::replaceAll(pattern, "**", "(.+)");
	String::replaceAll(pattern, '*', R"regex((((?!\/).)*))regex");
	String::replaceAll(pattern, "(.+)", "(.*)");

	bool exactMatch = inSettings == GlobMatch::FilesAndFoldersExact;
	if (exactMatch)
	{
		if (!String::startsWith(basePath, pattern))
			pattern = basePath + '/' + pattern;
	}

	static auto matchIsValid = [](const fs::directory_entry& path, const GlobMatch& settings) -> bool {
		bool isDirectory = path.is_directory();
		if (settings == GlobMatch::Files && isDirectory)
			return false;

		bool isRegularFile = path.is_regular_file();
		if (settings == GlobMatch::Folders && isRegularFile)
			return false;

		return isRegularFile || isDirectory;
	};

	constexpr auto reOptions = std::regex_constants::match_default;
	std::regex re(pattern);
	for (auto& it : fs::recursive_directory_iterator(basePath))
	{
		if (matchIsValid(it, inSettings))
		{
			auto p = it.path().string();
#if defined(CHALET_WIN32)
			Path::toUnix(p);
#endif
			if (exactMatch)
			{
				if (std::regex_match(p.begin(), p.end(), re, reOptions))
					onFound(p);
			}
			else
			{
				if (std::regex_search(p.begin(), p.end(), re, reOptions))
					onFound(p);
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool Files::forEachGlobMatch(const StringList& inPatterns, const GlobMatch inSettings, const GlobCallback& onFound)
{
	for (auto& pattern : inPatterns)
	{
		if (!forEachGlobMatch(pattern, inSettings, onFound))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool Files::forEachGlobMatch(const std::string& inPath, const std::string& inPattern, const GlobMatch inSettings, const GlobCallback& onFound)
{
	return forEachGlobMatch(fmt::format("{}/{}", inPath, inPattern), inSettings, onFound);
}

/*****************************************************************************/
bool Files::forEachGlobMatch(const std::string& inPath, const StringList& inPatterns, const GlobMatch inSettings, const GlobCallback& onFound)
{
	for (auto& pattern : inPatterns)
	{
		if (!forEachGlobMatch(inPath, pattern, inSettings, onFound))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool Files::addPathToListWithGlob(const std::string& inValue, StringList& outList, const GlobMatch inSettings)
{
	if (inValue.find_first_of("*{") != std::string::npos)
	{
		if (!Files::forEachGlobMatch(inValue, inSettings, [&outList](const std::string& inPath) {
				outList.emplace_back(inPath);
			}))
			return false;

		List::removeDuplicates(outList);
	}
	else
	{
		List::addIfDoesNotExist(outList, inValue);
	}

	return true;
}

/*****************************************************************************/
// TODO: This doesn't quite fit here
//
bool Files::readFileAndReplace(const std::string& inFile, const std::function<void(std::string&)>& onReplace)
{
	if (!Files::pathExists(inFile))
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
std::string Files::readShebangFromFile(const std::string& inFile)
{
	std::string ret;

	if (Files::pathExists(inFile))
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
void Files::sleep(const f64 inSeconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<i32>(inSeconds * 1000.0)));
}

/*****************************************************************************/
bool Files::createFileWithContents(const std::string& inFile, const std::string& inContents)
{
	auto folder = String::getPathFolder(inFile);
	if (!folder.empty() && !Files::pathExists(folder))
	{
		if (!Files::makeDirectory(folder))
		{
			Diagnostic::error("File with contents could not be created (Folder doesn't exist): {}", inFile);
			return false;
		}
	}

	std::ofstream(inFile) << inContents << std::endl;

	return true;
}

/*****************************************************************************/
std::string Files::getFileContents(const std::string& inFile)
{
	if (!Files::pathExists(inFile))
		return std::string();

	std::stringstream buffer;
	std::ifstream file{ inFile };
	buffer << file.rdbuf();

	return buffer.str();
}

/*****************************************************************************/
std::string Files::getFirstChildDirectory(const std::string& inPath)
{
	std::error_code ec;
	fs::path path(inPath);
	if (fs::exists(path, ec))
	{
		ec = std::error_code();

		auto dirEnd = fs::directory_iterator();
		for (auto it = fs::directory_iterator(inPath, ec); it != dirEnd; ++it)
		{
			auto item = *it;
			if (item.is_directory())
			{
				return item.path().string();
			}
		}
	}

	return std::string();
}

/*****************************************************************************/
std::string Files::isolateVersion(const std::string& outString)
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
std::string Files::which(const std::string& inExecutable, const bool inOutput)
{
	if (inExecutable.empty())
		return std::string();

	if (inOutput && Output::showCommands())
		Output::printCommand(fmt::format("executable search: {}", inExecutable));

	std::string result;
#if defined(CHALET_WIN32)
	LPSTR lpFilePart;
	char filename[MAX_PATH];

	auto exe = Files::getPlatformExecutableExtension();
	if (String::contains('.', inExecutable))
	{
		auto pos = inExecutable.find_last_of('.');
		exe = inExecutable.substr(pos);
	}

	if (SearchPathA(NULL, inExecutable.c_str(), exe.c_str(), MAX_PATH, filename, &lpFilePart))
	{
		result = std::string(filename);
		String::replaceAll(result, '\\', '/');
	}
#else
	if (!Files::pathExists(inExecutable)) // checks working dir
	{
		auto path = Environment::getPath();
		auto home = Environment::getUserDirectory();
		size_t start = 0;
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
			if (Files::pathExists(result))
				break;

			result.clear();
			start = end;
			if (start != std::string::npos)
				++start;
		}
	}

	// Note: cli "which" (original method) has issues when PATH is changed inside chalet
	//   doesn't seem to inherit the env

	if (result.empty())
		return result;

	#if defined(CHALET_MACOS)
	if (String::startsWith("/usr/bin/", result))
	{
		auto& xcodePath = getXcodePath();
		std::string withXcodePath = xcodePath + result;
		if (Files::pathExists(withXcodePath))
		{
			result = std::move(withXcodePath);
		}
		else
		{
			withXcodePath = fmt::format("{}/Toolchains/XcodeDefault.xctoolchain{}", xcodePath, result);
			if (Files::pathExists(withXcodePath))
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
const std::string& Files::getCygPath()
{
	if (state.cygPath.empty())
	{
		auto cygPath = Files::which("cygpath");
		state.cygPath = Process::runOutput({ std::move(cygPath), "-m", "/" });
		Path::toUnix(state.cygPath, true);
		state.cygPath.pop_back();
	}

	return state.cygPath;
}
#endif

#if defined(CHALET_MACOS)
/*****************************************************************************/
const std::string& Files::getXcodePath()
{
	if (state.xcodePath.empty())
	{
		auto xcodeSelect = "/usr/bin/xcode-select";
		state.xcodePath = Process::runOutput({ std::move(xcodeSelect), "-p" });
	}

	return state.xcodePath;
}

/*****************************************************************************/
bool Files::isUsingAppleCommandLineTools()
{
	const auto& xcodePath = Files::getXcodePath();
	return String::startsWith("/Library/Developer/CommandLineTools", xcodePath);
}
#endif
}
