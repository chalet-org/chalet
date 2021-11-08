/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/Arch.hpp"

#if defined(CHALET_MACOS)
	#include <TargetConditionals.h>
#endif

#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
std::string Arch::getHostCpuArchitecture()
{
#if defined(CHALET_MACOS)
	#if defined(TARGET_CPU_ARM64) && TARGET_CPU_ARM64
	return "aarch64";
	#elif defined(TARGET_CPU_X86_64) && TARGET_CPU_X86_64
	return "x86_64";
	#else
	return "i686";
	#endif
#else
	#if (defined(__x86_64__) && __x86_64__) || (defined(_M_AMD64) && _M_AMD64 == 100)
	return "x86_64";
	#elif (defined(__aarch64__) && __aarch64__) || (defined(_M_ARM64) && _M_ARM64 == 1)
	return "aarch64";
	#elif (defined(__arm__) && __arm__) || (defined(_M_ARM) && _M_ARM == 7)
	return "arm";
	#else
	return "i686";
	#endif
#endif
}

/*****************************************************************************/
std::string Arch::toGnuArch(const std::string& inValue)
{
	if (String::equals({ "x64", "amd64" }, inValue))
		return "x86_64";
	else if (String::equals("x86", inValue))
		return "i686";
	else if (String::equals("arm64", inValue))
		return "aarch64";
	else
		return inValue;
}

/*****************************************************************************/
// The goal with this class is to take some kind of architecture input and
// get a valid GNU-style architecture (hopefully a triple) out of it
//
void Arch::set(const std::string& inValue)
{
	// hopefully we got a triple, but it might not be
	triple = inValue;

	auto firstDash = triple.find('-');
	if (firstDash != std::string::npos)
	{
		str = triple.substr(0, firstDash);
		suffix = triple.substr(firstDash);
	}
	else
	{
		str = triple;
		suffix.clear();
	}

	std::string preUnderscoreCheck = str;

	auto firstUnderScore = str.find_last_of('_');
	if (firstUnderScore != std::string::npos)
	{
		str = str.substr(firstUnderScore + 1);
		if (String::equals("64", str))
			str = std::move(preUnderscoreCheck);
	}
	else
	{
		str = Arch::toGnuArch(str);
		triple = fmt::format("{}{}", str, suffix);
	}

	if (String::equals("x86_64", str))
	{
		val = Arch::Cpu::X64;
	}
	else if (String::equals("i686", str))
	{
		val = Arch::Cpu::X86;
	}
	else if (String::equals("aarch64", str))
	{
		val = Arch::Cpu::ARM64;
	}
	else if (String::startsWith("arm", str))
	{
		val = Arch::Cpu::ARM;
	}
#if defined(CHALET_MACOS)
	else if (String::startsWith("universal", str))
	{
		val = Arch::Cpu::UniversalMacOS;
	}
#endif
	else
	{
		val = Arch::Cpu::Unknown;
	}
}

}
