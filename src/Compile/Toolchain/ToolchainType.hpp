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
	AppleLLVM,
	VisualStudio,
	MingwGNU,
	MingwLLVM,
	IntelClassic,
	IntelLLVM,
	EmScripten
	// NVCC / CUDA ?
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_TYPE_HPP
