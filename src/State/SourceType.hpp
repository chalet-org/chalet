/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class SourceType : ushort
{
	Unknown,
	C,
	CPlusPlus,
	CxxPrecompiledHeader,
	ObjectiveC,
	ObjectiveCPlusPlus,
	WindowsResource,
};

using SourceTypeList = std::vector<SourceType>;
}
