/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerConfigController.hpp"

#include "Compile/CompilerConfig.hpp"
#include "State/BuildState.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerConfig& CompilerConfigController::get(const CodeLanguage inLanguage)
{
	chalet_assert(inLanguage != CodeLanguage::None, "Invalid language requested.");
	chalet_assert(m_configs.find(inLanguage) != m_configs.end(), "getCompilerConfig called before being initialized.");

	auto& config = *m_configs.at(inLanguage);
	return config;
}

/*****************************************************************************/
const CompilerConfig& CompilerConfigController::get(const CodeLanguage inLanguage) const
{
	chalet_assert(inLanguage != CodeLanguage::None, "Invalid language requested.");
	chalet_assert(m_configs.find(inLanguage) != m_configs.end(), "getCompilerConfig called before being initialized.");

	auto& config = *m_configs.at(inLanguage);
	return config;
}

/*****************************************************************************/
void CompilerConfigController::makeConfigForLanguage(const CodeLanguage inLanguage, const BuildState& inState)
{
	if (m_configs.find(inLanguage) != m_configs.end())
		return;

	m_configs.emplace(inLanguage, std::make_unique<CompilerConfig>(inLanguage, inState));
}

/*****************************************************************************/
bool CompilerConfigController::initialize()
{
	for (auto& [_, config] : m_configs)
	{
		if (!config->configureCompilerPaths())
		{
			Diagnostic::error("Error configuring compiler paths.");
			return false;
		}

		if (!config->testCompilerMacros())
		{
			Diagnostic::error("Unimplemented or unknown compiler toolchain.");
			return false;
		}

		if (!config->getSupportedCompilerFlags())
		{
			auto exec = String::getPathFilename(config->compilerExecutable());
			Diagnostic::error("Error collecting supported compiler flags for '{}'.", exec);
			return false;
		}
	}

	return true;
}

}
