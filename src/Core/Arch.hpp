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
		UniversalMacOS
#endif
	};

	std::string triple;
	std::string str;
	std::string suffix;
	Cpu val = Cpu::Unknown;

	static std::string getHostCpuArchitecture();
	static std::string toGnuArch(const std::string& inValue);
	static std::string toVSArch(Arch::Cpu inCpu);

	void set(const std::string& inValue);

	std::string toVSArch() const;
};
}

#endif // CHALET_ARCH_HPP
