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
Arch Arch::from(const std::string& inValue)
{
	Arch arch;
	arch.set(inValue);
	return arch;
}

/*****************************************************************************/
std::string Arch::getHostCpuArchitecture()
{
#if defined(CHALET_MACOS)
	#if defined(TARGET_CPU_ARM64) && TARGET_CPU_ARM64
	return "arm64";
	#elif defined(TARGET_CPU_X86_64) && TARGET_CPU_X86_64
	return "x86_64";
	#else
	return "i686";
	#endif
#else
	#if (defined(__x86_64__) && __x86_64__) || (defined(_M_AMD64) && _M_AMD64 == 100)
	return "x86_64";
	#elif defined(__aarch64__) && __aarch64__ && !defined(_M_ARM64)
	return "aarch64";
	#elif defined(_M_ARM64) && _M_ARM64 == 1
	return "arm64";
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
	if (String::equals(StringList{ "x64", "amd64" }, inValue))
		return "x86_64";
	else if (String::equals("x86", inValue))
		return "i686";
	else
		return inValue;
}

/*****************************************************************************/
std::string Arch::toVSArch(Arch::Cpu inCpu)
{
	switch (inCpu)
	{
		case Arch::Cpu::X86:
			return "x86";
		case Arch::Cpu::ARM:
		case Arch::Cpu::ARMHF:
			return "arm";
		case Arch::Cpu::ARM64:
			return "arm64";
		case Arch::Cpu::X64:
		default:
			return "x64";
	}
}

/*****************************************************************************/
std::string Arch::toVSArch2(Arch::Cpu inCpu)
{
	switch (inCpu)
	{
		case Arch::Cpu::X86:
			return "Win32";
		case Arch::Cpu::ARM:
		case Arch::Cpu::ARMHF:
			return "ARM";
		case Arch::Cpu::ARM64:
			return "ARM64";
		case Arch::Cpu::X64:
		default:
			return "x64";
	}
}

/*****************************************************************************/
// The goal with this class is to take some kind of architecture input and
// get a valid GNU-style architecture (hopefully a triple) out of it
//
void Arch::set(const std::string& inValue)
{
	// hopefully we got a triple, but it might not be
	triple = inValue;

	bool checkUnderScore = true;
	auto firstDash = triple.find('-');
	if (firstDash != std::string::npos)
	{
		str = triple.substr(0, firstDash);
		suffix = triple.substr(firstDash);
	}
	else
	{
//
#if defined(CHALET_WIN32)
		if (String::endsWith({ "_x64", "_x86", "_arm", "_arm64" }, triple))
		{
			checkUnderScore = false;
		}
#endif
		str = triple;
		suffix.clear();
	}

	std::string preUnderscoreCheck = str;

	auto firstUnderScore = str.find_last_of('_');
	if (checkUnderScore && firstUnderScore != std::string::npos)
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

#if defined(CHALET_LINUX)
	if (String::endsWith("eabihf", triple))
		str += "hf";
#endif

	if (String::equals("x86_64", str))
	{
		val = Arch::Cpu::X64;
	}
	else if (String::equals("i686", str))
	{
		val = Arch::Cpu::X86;
	}
	else if (String::equals({ "arm64", "aarch64" }, str))
	{
		val = Arch::Cpu::ARM64;
	}
	else if (String::equals("arm", str))
	{
		val = Arch::Cpu::ARM;
	}
#if defined(CHALET_LINUX)
	else if (String::equals("armhf", str))
	{
		val = Arch::Cpu::ARMHF;
	}
#endif
#if defined(CHALET_MACOS)
	else if (String::startsWith("universal", str))
	{
		val = Arch::Cpu::UniversalMacOS;
	}
#endif
#if defined(CHALET_WIN32)
	// MSVC host_target style arches
	else if (String::endsWith("_x64", str))
	{
		val = Arch::Cpu::X64;
	}
	else if (String::endsWith("_x86", str))
	{
		val = Arch::Cpu::X86;
	}
	else if (String::endsWith("_arm", str))
	{
		val = Arch::Cpu::ARM;
	}
	else if (String::endsWith("_arm64", str))
	{
		val = Arch::Cpu::ARMHF;
	}
#endif
	else if (String::equals("wasm32", str))
	{
		val = Arch::Cpu::WASM32;
	}
	else
	{
		val = Arch::Cpu::Unknown;
	}
}

/*****************************************************************************/
std::string Arch::toVSArch() const
{
	return toVSArch(val);
}

/*****************************************************************************/
std::string Arch::toVSArch2() const
{
	return toVSArch2(val);
}
}
