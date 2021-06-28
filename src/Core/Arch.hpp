#ifndef CHALET_ARCH_HPP
#define CHALET_ARCH_HPP

namespace chalet
{

struct Arch
{
	enum class Cpu : ushort
	{
		Unknown,
		X64,
		X86,
		ARM,
		ARM64,
#if defined(CHALET_MACOS)
		UniversalArm64_X64
#endif
	};

	std::string str;
	Cpu val = Cpu::Unknown;

	static std::string getHostCpuArchitecture();
#if defined(CHALET_WIN32)
	static StringList getAllowedMsvcArchitectures() noexcept;
#endif

	void set(const std::string& inValue) noexcept;
};
}

#endif // CHALET_ARCH_HPP
