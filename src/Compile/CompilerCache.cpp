/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCache.hpp"

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
CompilerConfig& CompilerCache::getConfig(const CodeLanguage inLanguage) const
{
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
