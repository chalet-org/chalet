/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/CompileEnvironment.hpp"

#include <thread>

#include "Libraries/FileSystem.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironment::CompileEnvironment(const CacheCompilers& inCompilers, const std::string& inBuildConfig) :
	m_compilers(inCompilers),
	m_buildConfiguration(inBuildConfig),
	m_processorCount(std::thread::hardware_concurrency())
{
	// std::cout << "Processor count: " << m_processorCount << std::endl;
}

/*****************************************************************************/
uint CompileEnvironment::processorCount() const noexcept
{
	return m_processorCount;
}

/*****************************************************************************/
CodeLanguage CompileEnvironment::language() const noexcept
{
	return m_language;
}

void CompileEnvironment::setLanguage(const std::string& inValue) noexcept
{
	if (String::equals(inValue, "C++"))
		m_language = CodeLanguage::CPlusPlus;

	if (String::equals(inValue, "C"))
		m_language = CodeLanguage::C;
}

/*****************************************************************************/
StrategyType CompileEnvironment::strategy() const noexcept
{
	return m_strategy;
}

void CompileEnvironment::setStrategy(const std::string& inValue) noexcept
{
	if (String::equals(inValue, "makefile"))
	{
		m_strategy = StrategyType::Makefile;
	}
	else if (String::equals(inValue, "native-experimental"))
	{
		m_strategy = StrategyType::Native;
	}
	else if (String::equals(inValue, "ninja-experimental"))
	{
		m_strategy = StrategyType::Ninja;
	}
	else
	{
		chalet_assert(false, "Invalid strategy type");
	}
}

/*****************************************************************************/
const std::string& CompileEnvironment::platform() const noexcept
{
	return m_platform;
}

void CompileEnvironment::setPlatform(const std::string& inValue) noexcept
{
	m_platform = inValue;
}

/*****************************************************************************/
const std::string& CompileEnvironment::modulePath() const noexcept
{
	return m_modulePath;
}

void CompileEnvironment::setModulePath(const std::string& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_modulePath = inValue;

	if (m_modulePath.back() == '/')
		m_modulePath.pop_back();
}

/*****************************************************************************/
bool CompileEnvironment::showCommands() const noexcept
{
	return m_showCommands;
}

void CompileEnvironment::setShowCommands(const bool inValue) noexcept
{
	m_showCommands = inValue;
}

bool CompileEnvironment::cleanOutput() const noexcept
{
	return !m_showCommands;
}

/*****************************************************************************/
bool CompileEnvironment::configureCompilerPaths()
{
	std::string exec = compilerExecutable();
	if (exec.empty())
	{
		auto language = m_language == CodeLanguage::CPlusPlus ? "C++" : "C";
		Diagnostic::errorAbort(fmt::format("No compiler was found for language '{}'.", language));
		return false;
	}

	std::string path = String::getPathFolder(exec);
	if (!String::endsWith("/bin", path))
	{
		auto language = m_language == CodeLanguage::CPlusPlus ? "C++" : "C";
		Diagnostic::errorAbort(fmt::format("Invalid compiler structure found for language '{}' (no 'bin' folder).", language));
		return false;
	}
	String::replaceAll(path, "/bin", "");

#if defined(CHALET_MACOS)
	auto& xcodePath = Commands::getXcodePath();
	String::replaceAll(path, xcodePath, "");
#endif

	m_compilerPathBin = path + "/bin";
	m_compilerPathLib = path + "/lib";
	m_compilerPathInclude = path + "/include";

	return true;
}

/*****************************************************************************/
bool CompileEnvironment::testCompilerMacros()
{
	Environment::set("PATH", getPathString()); // PATH must be set before any of this

	std::string exec = compilerExecutable();
	if (exec.empty())
		return false;

	std::string macroResult = Commands::testCompilerFlags(exec);
	String::replaceAll(macroResult, "\n", " ");
	String::replaceAll(macroResult, "#include ", "");

	// Notes:
	// GCC will just have __GNUC__
	// Clang will have both __clang__ & __GNUC__ (based on GCC 4)
	// Emscription will have __EMSCRIPTEN__, __clang__ & __GNUC__ (based on Clang)
	// Apple Clang (Xcode/CommandLineTools) is detected from the __VERSION__ macro (for now),
	//   since one can install both GCC and Clang from Homebrew, which will also contain __APPLE__ & __APPLE_CC__
	// GCC in MinGW 32, MinGW-w64 32-bit will have both __GNUC__ and __MINGW32__
	// GCC in MinGW-w64 64-bit will also have __MINGW64__
	// Intel will have __INTEL_COMPILER (or at the very least __INTEL_COMPILER_BUILD_DATE) & __GNUC__ (Also GCC-based)

	// TODO: Visual Studio will need its own detection method to check for _MSC_VER

	auto predefMacros = String::split(macroResult, " ");
	const bool clang = List::contains<std::string>(predefMacros, "__clang__");
	const bool gcc = List::contains<std::string>(predefMacros, "__GNUC__");
	const bool mingw32 = List::contains<std::string>(predefMacros, "__MINGW32__");
	const bool mingw64 = List::contains<std::string>(predefMacros, "__MINGW64__");
	const bool mingw = (mingw32 || mingw64);
	const bool emscripten = List::contains<std::string>(predefMacros, "__EMSCRIPTEN__");
	const bool intel = List::contains<std::string>(predefMacros, "__INTEL_COMPILER") || List::contains<std::string>(predefMacros, "__INTEL_COMPILER_BUILD_DATE");

	bool appleClang = false;
	if (clang)
	{
		std::string appleClangMacroResult = Commands::testAppleClang(exec);
		appleClang = !appleClangMacroResult.empty();
	}

	if (emscripten)
		m_compilerType = CppCompilerType::EmScripten;
	else if (appleClang)
		m_compilerType = CppCompilerType::AppleClang;
	else if (clang && mingw)
		m_compilerType = CppCompilerType::MingwClang;
	else if (clang)
		m_compilerType = CppCompilerType::Clang;
	else if (intel)
		m_compilerType = CppCompilerType::Intel;
	else if (gcc && mingw)
		m_compilerType = CppCompilerType::MingwGcc;
	else if (gcc)
		m_compilerType = CppCompilerType::Gcc;
	else
	{
		m_compilerType = CppCompilerType::Unknown;
		return false;
	}

	// std::cout << fmt::format("gcc: {}, clang: {}, appleClang: {}, mingw32: {}, mingw64: {}, emscripten: {}, intel: {},", gcc, clang, appleClang, mingw32, mingw64, emscripten, intel) << std::endl;
	// std::cout << "m_compilerType: " << static_cast<int>(m_compilerType) << std::endl;

	return true;
}

/*****************************************************************************/
CppCompilerType CompileEnvironment::compilerType() const noexcept
{
	return m_compilerType;
}

bool CompileEnvironment::isClang() const noexcept
{
	return m_compilerType == CppCompilerType::Clang || m_compilerType == CppCompilerType::AppleClang || m_compilerType == CppCompilerType::MingwClang || m_compilerType == CppCompilerType::EmScripten;
}

bool CompileEnvironment::isAppleClang() const noexcept
{
	return m_compilerType == CppCompilerType::AppleClang;
}

bool CompileEnvironment::isGcc() const noexcept
{
	return m_compilerType == CppCompilerType::Gcc || m_compilerType == CppCompilerType::MingwGcc || m_compilerType == CppCompilerType::Intel;
}

bool CompileEnvironment::isMingw() const noexcept
{
	return m_compilerType == CppCompilerType::MingwGcc || m_compilerType == CppCompilerType::MingwClang;
}

bool CompileEnvironment::isMingwGcc() const noexcept
{
	return m_compilerType == CppCompilerType::MingwGcc;
}

/*****************************************************************************/
const std::string& CompileEnvironment::compilerPathBin() const noexcept
{
	return m_compilerPathBin;
}
const std::string& CompileEnvironment::compilerPathLib() const noexcept
{
	return m_compilerPathLib;
}
const std::string& CompileEnvironment::compilerPathInclude() const noexcept
{
	return m_compilerPathInclude;
}

/*****************************************************************************/
const std::string& CompileEnvironment::compilerExecutable() const noexcept
{
	if (m_language == CodeLanguage::CPlusPlus)
		return m_compilers.cpp();
	else
		return m_compilers.cc();
}

/*****************************************************************************/
const StringList& CompileEnvironment::path() const noexcept
{
	return m_path;
}

void CompileEnvironment::addPaths(StringList& inList)
{
	List::forEach(inList, this, &CompileEnvironment::addPath);
}

void CompileEnvironment::addPath(std::string& inValue)
{
	if (inValue.back() == '/')
		inValue.pop_back();

	String::replaceAll(inValue, "${configuration}", m_buildConfiguration);
	Path::sanitize(inValue);
	List::addIfDoesNotExist(m_path, std::move(inValue));
}

/*****************************************************************************/
const std::string& CompileEnvironment::getPathString()
{
	if (m_pathString.empty())
	{
		StringList outList;

		m_originalPath = Environment::getPath();

		//
		if (!m_compilerPathBin.empty())
		{
#if defined(CHALET_WIN32)
			Path::sanitize(m_compilerPathBin);
#endif
			if (!String::contains(m_compilerPathBin, m_originalPath))
				outList.push_back(m_compilerPathBin);
		}

		//
		for (auto& p : m_path)
		{
			// TODO: function for this:
			if (!Commands::pathExists(p))
				continue;

			std::string path = Commands::getCanonicalPath(p);

			if (!String::contains(path, m_originalPath))
				outList.push_back(std::move(path));
		}

		StringList osPaths = getDefaultPaths();
		for (auto& p : osPaths)
		{
			if (!Commands::pathExists(p))
				continue;

			std::string path = Commands::getCanonicalPath(p);

			if (!String::contains(path, m_originalPath))
				outList.push_back(std::move(path));
		}

		//
		if (!m_originalPath.empty())
		{
#if defined(CHALET_WIN32)
			Path::sanitize(m_originalPath);
#endif
			outList.push_back(m_originalPath);
		}

		const std::string del = Path::getSeparator();

		m_pathString = String::join(outList, del);
		Path::sanitize(m_pathString);
	}

	return m_pathString;
}

/*****************************************************************************/
StringList CompileEnvironment::getDefaultPaths()
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

const std::string& CompileEnvironment::originalPath() const noexcept
{
	return m_originalPath;
}
}