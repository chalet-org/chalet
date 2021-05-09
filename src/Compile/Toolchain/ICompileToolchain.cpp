/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/ICompileToolchain.hpp"

namespace chalet
{
/*****************************************************************************/
// Compile
/*****************************************************************************/
/*****************************************************************************/
void ICompileToolchain::addIncludes(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addWarnings(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addDefines(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addPchInclude(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addOptimizationOption(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLanguageStandard(StringList& inArgList, const CxxSpecialization specialization) const
{
	UNUSED(inArgList, specialization);
}

/*****************************************************************************/
void ICompileToolchain::addDebuggingInformationOption(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addProfileInformationCompileOption(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addCompileOptions(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addDiagnosticColorOption(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLibStdCppCompileOption(StringList& inArgList, const CxxSpecialization specialization) const
{
	UNUSED(inArgList, specialization);
}

/*****************************************************************************/
void ICompileToolchain::addPositionIndependentCodeOption(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addNoRunTimeTypeInformationOption(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addThreadModelCompileOption(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
// Linking
/*****************************************************************************/
/*****************************************************************************/
void ICompileToolchain::addLibDirs(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLinks(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addRunPath(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addStripSymbolsOption(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLinkerOptions(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addProfileInformationLinkerOption(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLinkTimeOptimizationOption(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addThreadModelLinkerOption(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLinkerScripts(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLibStdCppLinkerOption(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addStaticCompilerLibraryOptions(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addPlatformGuiApplicationFlag(StringList& inArgList) const
{
	UNUSED(inArgList);
}
}
