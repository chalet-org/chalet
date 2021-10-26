/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerConfigController.hpp"

#include "Compile/CompilerConfig.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
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
void CompilerConfigController::makeConfigForLanguage(const CodeLanguage inLanguage, const BuildState& inState, ICompileEnvironment& inEnvironment)
{
	if (m_configs.find(inLanguage) != m_configs.end())
		return;

	m_configs.emplace(inLanguage, std::make_unique<CompilerConfig>(inLanguage, inState, inEnvironment));
}

/*****************************************************************************/
bool CompilerConfigController::initialize()
{
	for (auto& [_, config] : m_configs)
	{
		if (!config->getSupportedCompilerFlags())
		{
			Diagnostic::error("Error collecting supported compiler flags.");
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
ToolchainType CompilerConfigController::type() const noexcept
{
	return m_type;
}

void CompilerConfigController::setToolchainType(const ToolchainType inType) noexcept
{
	m_type = inType;
}

/*****************************************************************************/
bool CompilerConfigController::isWindowsClang() const noexcept
{
#if defined(CHALET_WIN32)
	return m_type == ToolchainType::LLVM
		|| m_type == ToolchainType::IntelLLVM;
#else
	return false;
#endif
}

/*****************************************************************************/
bool CompilerConfigController::isClang() const noexcept
{
	return m_type == ToolchainType::LLVM
		|| m_type == ToolchainType::AppleLLVM
		|| m_type == ToolchainType::IntelLLVM
		|| m_type == ToolchainType::MingwLLVM
		|| m_type == ToolchainType::EmScripten;
}

/*****************************************************************************/
bool CompilerConfigController::isAppleClang() const noexcept
{
	return m_type == ToolchainType::AppleLLVM;
}

/*****************************************************************************/
bool CompilerConfigController::isGcc() const noexcept
{
	return m_type == ToolchainType::GNU
		|| m_type == ToolchainType::MingwGNU
		|| m_type == ToolchainType::IntelClassic;
}

/*****************************************************************************/
bool CompilerConfigController::isIntelClassic() const noexcept
{
	return m_type == ToolchainType::IntelClassic;
}

/*****************************************************************************/
bool CompilerConfigController::isMingw() const noexcept
{
	return m_type == ToolchainType::MingwGNU
		|| m_type == ToolchainType::MingwLLVM;
}

/*****************************************************************************/
bool CompilerConfigController::isMingwGcc() const noexcept
{
	return m_type == ToolchainType::MingwGNU;
}

/*****************************************************************************/
bool CompilerConfigController::isMsvc() const noexcept
{
	return m_type == ToolchainType::VisualStudio;
}

/*****************************************************************************/
bool CompilerConfigController::isClangOrMsvc() const noexcept
{
	return isClang() || isMsvc();
}

}
