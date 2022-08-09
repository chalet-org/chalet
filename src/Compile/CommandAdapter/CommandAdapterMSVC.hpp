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
	CommandAdapterMSVC(const BuildState& inState, const SourceTarget& inProject);

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
	bool supportsBufferSecurityCheck() const;
	bool supportsRunTimeErrorChecks() const;
	bool supportsExceptions() const;
	bool supportsRunTimeTypeInformation() const;
	bool disableRunTimeTypeInformation() const;
	bool supportsTreatWChartAsBuiltInType() const;
	bool supportsForceConformanceInForLoopScope() const;
	bool supportsRemoveUnreferencedCodeData() const;

	std::string getPlatformToolset() const;

	std::string getLanguageStandardCpp() const;
	std::string getLanguageStandardC() const;
	char getOptimizationLevel() const;
	char getInlineFuncExpansion() const;

	std::string getSubSystem() const;
	std::string getEntryPoint() const;

	StringList getAdditionalOptions() const;

private:
	const BuildState& m_state;
	const SourceTarget& m_project;

	uint m_versionMajorMinor = 0;
	uint m_versionPatch = 0;
};
}

#endif // CHALET_COMMAND_ADAPTER_MSVC_HPP
