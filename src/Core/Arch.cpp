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
	return "arm64";
	#elif TARGET_CPU_X86_64
	return "x86_64";
	#else
	return "x86";
	#endif
#else
	#if defined(__x86_64__)
	return "x86_64";
	#elif defined(__aarch64__)
	return "arm64";
	#elif defined(__arm__)
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
	str = inValue;

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
	else if (String::equals("x86", arch))
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
	else
	{
		val = Arch::Cpu::Unknown;
	}
}

}
