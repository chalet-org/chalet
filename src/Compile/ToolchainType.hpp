/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class ToolchainType : ushort
{
	Unknown,
	GNU,
	LLVM,
	AppleLLVM,
	VisualStudio,
	VisualStudioLLVM,
	MingwGNU,
	MingwLLVM,
	IntelClassic,
	IntelLLVM,
	Emscripten
	// NVCC / CUDA ?
};
}
