/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CUSTOM_TOOLCHAIN_TREAT_AS_HPP
#define CHALET_CUSTOM_TOOLCHAIN_TREAT_AS_HPP

namespace chalet
{
enum class CustomToolchainTreatAs : ushort
{
	None,
	LLVM,
	GCC,
};
}

#endif // CHALET_CUSTOM_TOOLCHAIN_TREAT_AS_HPP
