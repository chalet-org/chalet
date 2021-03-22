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
	GNU,
	LLVM,
	MSVC
};

enum class CppCompilerType : ushort
{
	Unknown,
	Gcc,
	Clang,
	AppleClang,
	MingwGcc,
	MingwClang,
	VisualStudio, // TODO!
	Intel,
	EmScripten
	// NVCC / CUDA ?
};

}

#endif // CHALET_COMPILE_TOOLCHAIN_TYPE_HPP
