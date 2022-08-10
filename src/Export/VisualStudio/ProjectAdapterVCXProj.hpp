/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROJECT_ADAPTER_VCXPROJ_HPP
#define CHALET_PROJECT_ADAPTER_VCXPROJ_HPP

#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"

namespace chalet
{
class BuildState;
struct SourceTarget;

struct ProjectAdapterVCXProj
{
	ProjectAdapterVCXProj(const BuildState& inState, const SourceTarget& inProject);

	std::string getBoolean(const bool inValue) const;
	std::string getBooleanIfTrue(const bool inValue) const;

	// General
	std::string getConfigurationType() const;
	std::string getUseDebugLibraries() const;
	std::string getPlatformToolset() const;
	std::string getWholeProgramOptimization() const;
	std::string getCharacterSet() const;

	std::string getFunctionLevelLinking() const;
	std::string getIntrinsicFunctions() const;
	std::string getSDLCheck() const;
	std::string getConformanceMode() const;
	std::string getWarningLevel() const;
	std::string getExternalWarningLevel() const;
	std::string getPreprocessorDefinitions() const;
	std::string getLanguageStandardCpp() const;
	std::string getLanguageStandardC() const;
	std::string getTreatWarningsAsError() const;
	std::string getDiagnosticsFormat() const;
	std::string getDebugInformationFormat() const;
	std::string getSupportJustMyCode() const;
	std::string getEnableAddressSanitizer() const;
	std::string getOptimization() const;
	std::string getInlineFunctionExpansion() const;
	std::string getFavorSizeOrSpeed() const;
	std::string getWholeProgramOptimizationCompileFlag() const;
	std::string getBufferSecurityCheck() const;
	std::string getFloatingPointModel() const;
	std::string getBasicRuntimeChecks() const;
	std::string getRuntimeLibrary() const;
	std::string getExceptionHandling() const;
	std::string getRunTimeTypeInfo() const;
	std::string getTreatWChartAsBuiltInType() const;
	std::string getForceConformanceInForLoopScope() const;
	std::string getRemoveUnreferencedCodeData() const;
	std::string getCallingConvention() const;
	std::string getAdditionalOptions() const;

	std::string getSubSystem() const;
	std::string getEntryPoint() const;
	std::string getLinkTimeCodeGeneration() const;
	std::string getTargetMachine() const;

	StringList getSourceExtensions() const;
	StringList getHeaderExtensions() const;
	StringList getResourceExtensions() const;

private:
	const BuildState& m_state;
	const SourceTarget& m_project;

	CommandAdapterMSVC m_msvcAdapter;

	uint m_versionMajorMinor = 0;
	uint m_versionPatch = 0;
};
}

#endif // CHALET_PROJECT_ADAPTER_VCXPROJ_HPP
