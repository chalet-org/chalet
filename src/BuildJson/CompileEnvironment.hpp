/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_ENVIRONMENT_HPP
#define CHALET_COMPILE_ENVIRONMENT_HPP

#include "BuildJson/WorkspaceInfo.hpp"
#include "CacheJson/CacheCompilers.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "Compile/Toolchain/ToolchainType.hpp"
#include "State/CommandLineInputs.hpp"

namespace chalet
{
enum class CodeLanguage : ushort
{
	C,
	CPlusPlus
};

struct CompileEnvironment
{
	explicit CompileEnvironment(const CacheCompilers& inCompilers, const std::string& inBuildConfig);

	uint processorCount() const noexcept;

	CodeLanguage language() const noexcept;
	void setLanguage(const std::string& inValue) noexcept;

	StrategyType strategy() const noexcept;
	void setStrategy(const std::string& inValue) noexcept;

	const std::string& platform() const noexcept;
	void setPlatform(const std::string& inValue) noexcept;

	const std::string& modulePath() const noexcept;
	void setModulePath(const std::string& inValue) noexcept;

	bool showCommands() const noexcept;
	void setShowCommands(const bool inValue) noexcept;
	bool cleanOutput() const noexcept;

	bool configureCompilerPaths();
	bool testCompilerMacros();

	CppCompilerType compilerType() const noexcept;
	bool isClang() const noexcept;
	bool isAppleClang() const noexcept;
	bool isGcc() const noexcept;
	bool isMingw() const noexcept;
	bool isMingwGcc() const noexcept;

	const std::string& compilerPathBin() const noexcept;
	const std::string& compilerPathLib() const noexcept;
	const std::string& compilerPathInclude() const noexcept;

	const std::string& compilerExecutable() const noexcept;

	const StringList& path() const noexcept;
	void addPaths(StringList& inList);
	void addPath(std::string& inValue);
	const std::string& getPathString();
	const std::string& originalPath() const noexcept;

private:
	StringList getDefaultPaths();

	const CacheCompilers& m_compilers;
	const std::string& m_buildConfiguration;

	std::string m_platform = "auto";
	std::string m_modulePath{ "chalet_modules" };
	std::string m_compilerPath{ "/usr" };
	std::string m_compilerPathBin{ "/usr/bin" };
	std::string m_compilerPathLib{ "/usr/lib" };
	std::string m_compilerPathInclude{ "/usr/include" };
	StringList m_path;

	std::string m_pathString;
	std::string m_originalPath;

	uint m_processorCount = 0;

	CodeLanguage m_language = CodeLanguage::CPlusPlus;
	StrategyType m_strategy = StrategyType::Makefile;
	CppCompilerType m_compilerType = CppCompilerType::Unknown;

	bool m_showCommands = false;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_HPP
