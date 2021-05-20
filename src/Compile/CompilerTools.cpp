/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerTools.hpp"

#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
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
