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
	#if TARGET_CPU_ARM64
	return "arm64-apple-darwin";
	#elif TARGET_CPU_X86_64
	return "x86_64-apple-darwin";
	#else
	return "i686-apple-darwin";
	#endif
#else
	#if (defined(__x86_64__) && __x86_64__) || (defined(_M_AMD64) && _M_AMD64 == 100)
	return "x86_64";
	#elif (defined(__aarch64__) && __aarch64__) || (defined(_M_ARM64) && _M_ARM64 == 1)
	return "arm64";
	#elif (defined(__arm__) && __arm__) || (defined(_M_ARM) && _M_ARM == 7)
	return "arm";
	#else
	return "x86";
	#endif
#endif
}

/*****************************************************************************/
void Arch::set(const std::string& inValue) noexcept
{
	// https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html

	// Note: Standalone "mingw32" is used in older mingw builds (4.x)
	if (String::equals("mingw32", inValue))
	{
		str = "i686-pc-mingw32";
	}
	else
	{
		str = inValue;
	}

	std::string arch = str;
	if (String::contains('-', arch))
	{
		auto split = String::split(str, '-');
		arch = split.front();
	}

	if (String::equals({ "x86_64", "x64" }, arch))
	{
		val = Arch::Cpu::X64;
	}
	else if (String::equals({ "i686", "x86" }, arch))
	{
		val = Arch::Cpu::X86;
	}
	else if (String::equals("arm64", arch))
	{
		val = Arch::Cpu::ARM64;
	}
	else if (String::startsWith("arm", arch))
	{
		val = Arch::Cpu::ARM;
	}
#if defined(CHALET_MACOS)
	else if (String::startsWith("universal", arch))
	{
		val = Arch::Cpu::UniversalArm64_X64;
	}
#endif
	else
	{
		val = Arch::Cpu::Unknown;
	}
}
}
