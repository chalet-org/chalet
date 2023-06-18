/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMAND_ADAPTER_MSVC_HPP
#define CHALET_COMMAND_ADAPTER_MSVC_HPP

#include "State/MSVCWarningLevel.hpp"
#include "State/WindowsCallingConvention.hpp"
#include "State/WindowsRuntimeLibraryType.hpp"

namespace chalet
{
class BuildState;
struct SourceTarget;

struct CommandAdapterMSVC
{
	explicit CommandAdapterMSVC(const BuildState& inState, const SourceTarget& inProject);

	static std::string getPlatformToolset(const BuildState& inState);

	MSVCWarningLevel getWarningLevel() const;
	WindowsRuntimeLibraryType getRuntimeLibraryType() const;
	WindowsCallingConvention getCallingConvention() const;

	bool supportsFastMath() const;
	bool supportsFunctionLevelLinking() const;
	bool supportsGenerateIntrinsicFunctions() const;
	bool supportsSDLCheck() const;
	bool supportsConformanceMode() const;
	bool supportsEditAndContinue() const;
	bool supportsJustMyCodeDebugging() const;
	bool supportsAddressSanitizer() const;
	bool supportsWholeProgramOptimization() const;
	bool supportsLinkTimeCodeGeneration() const;
	bool supportsBufferSecurityCheck() const;
	bool supportsRunTimeErrorChecks() const;
	bool supportsExceptions() const;
	bool supportsRunTimeTypeInformation() const;
	bool disableRunTimeTypeInformation() const;
	bool supportsTreatWChartAsBuiltInType() const;
	bool supportsForceConformanceInForLoopScope() const;
	bool supportsRemoveUnreferencedCodeData() const;
	bool supportsExternalWarnings() const;

	bool supportsIncrementalLinking() const;
	bool supportsCOMDATFolding() const;
	bool supportsOptimizeReferences() const;
	std::optional<bool> supportsLongBranchRedirects() const;
	bool supportsProfiling() const;
	bool supportsDataExecutionPrevention() const;
	bool suportsILKGeneration() const;
	bool supportsFixedBaseAddress() const;
	bool disableFixedBaseAddress() const;
	bool enableDebugging() const;
	bool supportsRandomizedBaseAddress() const;

	std::string getPlatformToolset() const;

	std::string getLanguageStandardCpp() const;
	std::string getLanguageStandardC() const;
	char getOptimizationLevel() const;
	char getInlineFuncExpansion() const;

	std::string getSubSystem() const;
	std::string getEntryPoint() const;
	std::string getMachineArchitecture() const;

	StringList getIncludeDirectories() const;
	StringList getAdditionalCompilerOptions(const bool inCharsetFlags = false) const;
	StringList getAdditionalLinkerOptions() const;

	StringList getLibDirectories() const;
	StringList getLinks(const bool inIncludeCore = true) const;

	bool createPrecompiledHeaderSource(const std::string& inSourcePath, const std::string& inPchPath);
	const std::string& pchSource() const noexcept;
	const std::string& pchTarget() const noexcept;
	const std::string& pchMinusLocation() const noexcept;

private:
	const BuildState& m_state;
	const SourceTarget& m_project;

	std::string m_pchSource;
	std::string m_pchTarget;
	std::string m_pchMinusLocation;

	uint m_versionMajorMinor = 0;
	uint m_versionPatch = 0;
};
}

#endif // CHALET_COMMAND_ADAPTER_MSVC_HPP
