/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SOURCE_TYPE_HPP
#define CHALET_SOURCE_TYPE_HPP

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

#endif // CHALET_SOURCE_TYPE_HPP
