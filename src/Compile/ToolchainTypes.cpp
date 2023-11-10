/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/ToolchainTypes.hpp"

#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
std::string ToolchainTypes::getTypeName(const ToolchainType inType) noexcept
{
	switch (inType)
	{
		case ToolchainType::GNU:
			return std::string("gnu");
		case ToolchainType::LLVM:
			return std::string("llvm");
		case ToolchainType::AppleLLVM:
			return std::string("apple-llvm");
		case ToolchainType::VisualStudio:
			return std::string("msvc");
		case ToolchainType::VisualStudioLLVM:
			return std::string("vs-llvm");
		case ToolchainType::MingwGNU:
		case ToolchainType::MingwLLVM:
			return std::string("mingw");
		case ToolchainType::IntelClassic:
			return std::string("intel-classic");
		case ToolchainType::IntelLLVM:
			return std::string("intel-llvm");
		case ToolchainType::Emscripten:
			return std::string("emscripten");
		case ToolchainType::Unknown:
		default:
			return std::string("unknown");
	}
}
}
