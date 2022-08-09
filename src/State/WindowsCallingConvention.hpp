/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WINDOWS_CALLING_CONVENTION_HPP
#define CHALET_WINDOWS_CALLING_CONVENTION_HPP

namespace chalet
{
enum class WindowsCallingConvention : ushort
{
	Cdecl,
	FastCall,
	StdCall,
	VectorCall,
};
}

#endif // CHALET_WINDOWS_CALLING_CONVENTION_HPP
