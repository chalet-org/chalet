#pragma once

namespace chalet
{
struct Arch
{
	enum class Cpu : u16
	{
		Unknown,
		X64,
		X86,
		ARM,
		ARMHF,
		ARM64,
#if defined(CHALET_MACOS)
		UniversalMacOS,
#endif
		WASM32,
	};

	std::string triple;
	std::string str;
	std::string suffix;
	Cpu val = Cpu::Unknown;

	static Arch from(const std::string& inValue);
	static std::string getHostCpuArchitecture() noexcept;
	static std::string toGnuArch(const std::string& inValue);
	static std::string toVSArch(Arch::Cpu inCpu);
	static std::string toVSArch2(Arch::Cpu inCpu);

	void set(const std::string& inValue);

	std::string toVSArch() const;
	std::string toVSArch2() const;
};
}
