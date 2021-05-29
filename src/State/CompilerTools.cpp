/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/CompilerTools.hpp"

#include "Libraries/Format.hpp"
#include "State/WorkspaceInfo.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerTools::CompilerTools(const WorkspaceInfo& inInfo) :
	m_info(inInfo)
{
}

/*****************************************************************************/
void CompilerTools::detectToolchain()
{
#if defined(CHALET_WIN32)
	if (String::endsWith("cl.exe", m_cc) || String::contains("cl.exe", m_cpp))
	{
		m_detectedToolchain = ToolchainType::MSVC;
	}
	else
#endif
		if (String::contains("clang", m_cc) || String::contains("clang", m_cpp))
	{
		m_detectedToolchain = ToolchainType::LLVM;
	}
	else if (String::contains("gcc", m_cc) || String::contains("g++", m_cpp))
	{
		m_detectedToolchain = ToolchainType::GNU;
	}
	else
	{
		m_detectedToolchain = ToolchainType::Unknown;
	}

	m_ccDetected = Commands::pathExists(m_cc);
}

/*****************************************************************************/
bool CompilerTools::initialize()
{
	fetchCompilerVersions();

	if (m_detectedToolchain == ToolchainType::LLVM)
	{
		auto results = Commands::subprocessOutput({ compiler(), "-print-targets" });
#if defined(CHALET_WIN32)
		auto split = String::split(results, "\r\n");
#else
		auto split = String::split(results, '\n');
#endif
		bool valid = false;
		// m_state.info.setTargetArchitecture(arch);
		const auto& targetArch = m_info.targetArchitectureString();
		for (auto& line : split)
		{
			auto start = line.find_first_not_of(' ');
			auto end = line.find_first_of(' ', start);

			auto arch = line.substr(start, end - start);
			if (String::startsWith(arch, targetArch))
				valid = true;
		}

		UNUSED(split);
		return valid;
	}

	return true;
}

/*****************************************************************************/
void CompilerTools::fetchCompilerVersions()
{
	if (m_compilerVersionStringCpp.empty())
	{
		if (!m_cpp.empty() && Commands::pathExists(m_cpp))
		{
			std::string version;
#if defined(CHALET_WIN32)
			if (m_detectedToolchain == ToolchainType::MSVC)
			{
				version = parseVersionMSVC(m_cpp);
			}
			else
			{
				version = parseVersionGNU(m_cpp, "\r\n");
			}
#else
			version = parseVersionGNU(m_cpp);
#endif
			m_compilerVersionStringCpp = std::move(version);
		}
	}

	if (m_compilerVersionStringC.empty())
	{
		if (!m_cc.empty() && Commands::pathExists(m_cc))
		{
			std::string version;
#if defined(CHALET_WIN32)
			if (m_detectedToolchain == ToolchainType::MSVC)
			{
				version = parseVersionMSVC(m_cc);
			}
			else
			{
				version = parseVersionGNU(m_cc, "\r\n");
			}
#else
			version = parseVersionGNU(m_cc);
#endif
			m_compilerVersionStringC = std::move(version);
		}
	}
}

/*****************************************************************************/
std::string CompilerTools::parseVersionMSVC(const std::string& inExecutable) const
{
	std::string ret;

	// Microsoft (R) C/C++ Optimizing Compiler Version 19.28.29914 for x64
	std::string rawOutput = Commands::subprocessOutput({ inExecutable });
	auto splitOutput = String::split(rawOutput, "\r\n");
	if (splitOutput.size() >= 2)
	{
		auto start = splitOutput[1].find("Version");
		auto end = splitOutput[1].find(" for ");
		if (start != std::string::npos && end != std::string::npos)
		{
			const auto versionString = splitOutput[1].substr(start, end - start);
			// const auto arch = splitOutput[1].substr(end + 5);
			const auto arch = m_info.targetArchitectureString();
			ret = fmt::format("Microsoft{} Visual C/C++ {} [{}]", Unicode::registered(), versionString, arch);
		}
	}

	return ret;
}

/*****************************************************************************/
std::string CompilerTools::parseVersionGNU(const std::string& inExecutable, const std::string_view inEol) const
{
	std::string ret;

	// gcc version 10.2.0 (Ubuntu 10.2.0-13ubuntu1)
	// gcc version 10.2.0 (Rev10, Built by MSYS2 project)
	// Apple clang version 12.0.5 (clang-1205.0.22.9)
	const auto exec = String::getPathBaseName(inExecutable);
	const bool isCpp = String::contains("++", exec);
	// const bool isC = String::startsWith({ "gcc", "cc" }, exec);
	std::string rawOutput;
	if (String::contains("clang", inExecutable))
	{
		rawOutput = Commands::subprocessOutput({ inExecutable, "-target", m_info.targetArchitectureString(), "-v" });
	}
	else
	{
		rawOutput = Commands::subprocessOutput({ inExecutable, "-v" });
	}

	auto splitOutput = String::split(rawOutput, inEol);
	if (splitOutput.size() >= 2)
	{
		std::string versionString;
		std::string compilerRaw;
		std::string arch;
		std::string threadModel;
		for (auto& line : splitOutput)
		{
			if (String::contains("version", line))
			{
				auto start = line.find("version");
				compilerRaw = line.substr(0, start - 1);
				versionString = line.substr(start + 8);

				while (versionString.back() == ' ')
					versionString.pop_back();
			}
			else if (String::startsWith("Target:", line))
			{
				arch = line.substr(8);
			}
			/*else if (String::startsWith("Thread model:", line))
			{
				threadModel = line.substr(14);
			}*/
			// Supported LTO compression algorithms:
		}
		UNUSED(threadModel);

		if (!compilerRaw.empty())
		{
			if (String::startsWith("gcc", compilerRaw))
			{
				ret = fmt::format("GNU Compiler Collection {} Version {} [{}]", isCpp ? "C++" : "C", versionString, arch);
			}
			else if (String::startsWith("Apple clang", compilerRaw))
			{
				ret = fmt::format("Apple Clang {} Version {} [{}]", isCpp ? "C++" : "C", versionString, arch);
			}
		}
		else
		{
			ret = "Unrecognized";
		}
	}

	return ret;
}

/*****************************************************************************/
ToolchainType CompilerTools::detectedToolchain() const
{
	return m_detectedToolchain;
}

/*****************************************************************************/
const std::string& CompilerTools::compiler() const noexcept
{
	if (m_ccDetected)
		return m_cc;
	else
		return m_cpp;
}

/*****************************************************************************/
const std::string& CompilerTools::compilerVersionStringCpp() const noexcept
{
	return m_compilerVersionStringCpp;
}

const std::string& CompilerTools::compilerVersionStringC() const noexcept
{
	return m_compilerVersionStringC;
}

/*****************************************************************************/
const std::string& CompilerTools::archiver() const noexcept
{
	return m_archiver;
}
void CompilerTools::setArchiver(const std::string& inValue) noexcept
{
	m_archiver = inValue;

	m_isArchiverLibTool = String::endsWith("libtool", inValue);
}
bool CompilerTools::isArchiverLibTool() const noexcept
{
	return m_isArchiverLibTool;
}

/*****************************************************************************/
const std::string& CompilerTools::cpp() const noexcept
{
	return m_cpp;
}
void CompilerTools::setCpp(const std::string& inValue) noexcept
{
	m_cpp = inValue;
}

/*****************************************************************************/
const std::string& CompilerTools::cc() const noexcept
{
	return m_cc;
}
void CompilerTools::setCc(const std::string& inValue) noexcept
{
	m_cc = inValue;
}

/*****************************************************************************/
const std::string& CompilerTools::linker() const noexcept
{
	return m_linker;
}
void CompilerTools::setLinker(const std::string& inValue) noexcept
{
	m_linker = inValue;
}

/*****************************************************************************/
const std::string& CompilerTools::rc() const noexcept
{
	return m_rc;
}
void CompilerTools::setRc(const std::string& inValue) noexcept
{
	m_rc = inValue;
}

/*****************************************************************************/
std::string CompilerTools::getRootPathVariable()
{
	auto originalPath = Environment::getPath();
	Path::sanitize(originalPath);

	StringList outList;

	if (auto ccRoot = String::getPathFolder(m_cc); !List::contains(outList, ccRoot))
		outList.push_back(std::move(ccRoot));

	if (auto cppRoot = String::getPathFolder(m_cpp); !List::contains(outList, cppRoot))
		outList.push_back(std::move(cppRoot));

	for (auto& p : Path::getOSPaths())
	{
		if (!Commands::pathExists(p))
			continue;

		auto path = Commands::getCanonicalPath(p); // probably not needed, but just in case

		if (!List::contains(outList, path))
			outList.push_back(std::move(path));
	}

	char separator = Path::getSeparator();
	for (auto& path : String::split(originalPath, separator))
	{
		if (!List::contains(outList, path))
			outList.push_back(std::move(path));
	}

	std::string ret = String::join(outList, separator);
	Path::sanitize(ret);

	return ret;
}

/*****************************************************************************/
CompilerConfig& CompilerTools::getConfig(const CodeLanguage inLanguage) const
{
	chalet_assert(inLanguage != CodeLanguage::None, "Invalid language requested.");
	if (m_configs.find(inLanguage) != m_configs.end())
	{
		return m_configs.at(inLanguage);
	}

	m_configs.emplace(inLanguage, CompilerConfig(inLanguage, *this));
	auto& config = m_configs.at(inLanguage);

	UNUSED(config.configureCompilerPaths());
	if (!config.testCompilerMacros())
	{
		Diagnostic::errorAbort("Unimplemented or unknown compiler toolchain.");
	}

	return config;
}
}
