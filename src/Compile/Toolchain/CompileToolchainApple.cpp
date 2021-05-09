/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainApple.hpp"

// TODO: Find a nice way to separate out the clang/appleclang stuff from CompileToolchainGNU

namespace chalet
{
/*****************************************************************************/
CompileToolchainApple::CompileToolchainApple(const BuildState& inState, const ProjectConfiguration& inProject, const CompilerConfig& inConfig) :
	CompileToolchainLLVM(inState, inProject, inConfig)
{
}

/*****************************************************************************/
ToolchainType CompileToolchainApple::type() const
{
	return ToolchainType::Apple;
}

/*****************************************************************************/
// Note: Noops mean a flag/feature isn't supported

/*****************************************************************************/
// Linking
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainApple::addProfileInformationLinkerOption(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void CompileToolchainApple::addLibStdCppLinkerOption(StringList& inArgList)
{
	CompileToolchainLLVM::addLibStdCppLinkerOption(inArgList);

	// TODO: Apple has a "-stdlib=libstdc++" flag that is pre-C++11 for compatibility
}

/*****************************************************************************/
// Objective-C / Objective-C++
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainApple::addObjectiveCxxLink(StringList& inArgList)
{
	// Unused in AppleClang
	UNUSED(inArgList);
}

/*****************************************************************************/
void CompileToolchainApple::addObjectiveCxxCompileOption(StringList& inArgList, const CxxSpecialization specialization)
{
	const bool isObjCpp = specialization == CxxSpecialization::ObjectiveCpp;
	const bool isObjC = specialization == CxxSpecialization::ObjectiveC;
	const bool isObjCxx = specialization == CxxSpecialization::ObjectiveCpp || specialization == CxxSpecialization::ObjectiveC;
	if (m_project.objectiveCxx() && isObjCxx)
	{
		inArgList.push_back("-x");
		if (isObjCpp)
			inArgList.push_back("objective-c++");
		else if (isObjC)
			inArgList.push_back("objective-c");
	}
}

/*****************************************************************************/
void CompileToolchainApple::addObjectiveCxxRuntimeOption(StringList& inArgList, const CxxSpecialization specialization)
{
	// Unused in AppleClang
	UNUSED(inArgList, specialization);
}

}
