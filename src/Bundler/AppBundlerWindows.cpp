/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundlerWindows.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Platform/Arch.hpp"
#include "Process/Environment.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "System/Files.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
AppBundlerWindows::AppBundlerWindows(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap) :
	IAppBundler(inState, inBundle, inDependencyMap)
{
}

/*****************************************************************************/
bool AppBundlerWindows::removeOldFiles()
{
	return true;
}

/*****************************************************************************/
bool AppBundlerWindows::bundleForPlatform()
{
#if defined(CHALET_WIN32)
	if (m_state.environment->isMsvc() || m_state.environment->isMsvcClang())
	{
		Arch::Cpu targetArch = m_state.info.targetArchitecture();
		if (targetArch == Arch::Cpu::X86 || targetArch == Arch::Cpu::X64 || targetArch == Arch::Cpu::ARM64)
		{
			auto vcToolsRedistDir = Environment::getString("VCToolsRedistDir");
			auto targetArchVS = Arch::toVSArch(targetArch);
			auto vcRedist = fmt::format("{}/vc_redist.{}.exe", vcToolsRedistDir, targetArchVS);
			Path::toUnix(vcRedist);

			auto& distributionDirectory = m_state.inputs.distributionDirectory();
			Files::copy(vcRedist, distributionDirectory);
		}
	}
#endif
	return true;
}
}
