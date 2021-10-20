/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_TYPE_HPP
#define CHALET_COMPILE_TOOLCHAIN_TYPE_HPP

namespace chalet
{
enum class ToolchainType : ushort
{
	Unknown,
	GNU,
	LLVM,
	Apple,
	MSVC,
	IntelClassic,
	IntelLLVM
};

enum class CppCompilerType : ushort
{
	Unknown,
	Gcc,
	Clang,
	AppleClang,
	MingwGcc,
	MingwClang,
	VisualStudio,
	IntelClassic,
	IntelClang,
	EmScripten
	// NVCC / CUDA ?
};

}

#endif // CHALET_COMPILE_TOOLCHAIN_TYPE_HPP
