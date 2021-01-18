/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCache.hpp"

#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
const std::string& CompilerCache::cpp() const noexcept
{
	return m_cpp;
}
void CompilerCache::setCpp(const std::string& inValue) noexcept
{
	m_cpp = inValue;
}

/*****************************************************************************/
const std::string& CompilerCache::cc() const noexcept
{
	return m_cc;
}
void CompilerCache::setCc(const std::string& inValue) noexcept
{
	m_cc = inValue;
}

/*****************************************************************************/
const std::string& CompilerCache::rc() const noexcept
{
	return m_rc;
}
void CompilerCache::setRc(const std::string& inValue) noexcept
{
	m_rc = inValue;
}

/*****************************************************************************/
std::string CompilerCache::getRootPathVariable()
{
	auto originalPath = Environment::getPath();
	Path::sanitize(originalPath);

	StringList outList;

	if (auto ccRoot = String::getPathFolder(m_cc); !List::contains(outList, ccRoot))
		outList.push_back(std::move(ccRoot));

	if (auto cppRoot = String::getPathFolder(m_cpp); !List::contains(outList, cppRoot))
		outList.push_back(std::move(cppRoot));

	for (auto& path : Path::getOSPaths())
	{
		if (!Commands::pathExists(path))
			continue;

		if (!List::contains(outList, path))
			outList.push_back(std::move(path));
	}

	auto separator = Path::getSeparator();
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
CompilerConfig& CompilerCache::getConfig(const CodeLanguage inLanguage) const
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
