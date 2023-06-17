/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SOURCE_DATA_TYPE_HPP
#define CHALET_SOURCE_DATA_TYPE_HPP

namespace chalet
{

// Note: this doesn't necessarily have to be for modules...
//   It can be anything that describes the source file other than what's in SourceType
//   At the moment, it's just internally used for optimizations
//
enum class SourceDataType : ushort
{
	None,
	UserHeaderUnit,
	SystemHeaderUnit,
	SystemModule,
	Module,
};
}

#endif // CHALET_SOURCE_DATA_TYPE_HPP
