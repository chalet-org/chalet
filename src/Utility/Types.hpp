/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_UTIL_TYPES_HPP
#define CHALET_UTIL_TYPES_HPP

#include <cstdint>

namespace chalet
{
typedef std::int64_t llong;

typedef std::uint8_t uchar;
typedef std::uint16_t ushort;
typedef std::uint32_t uint;
typedef std::uint64_t ullong;

typedef long double ldouble;

template <typename MapType>
using OrderedDictionary = std::map<std::string, MapType>;

template <typename MapType>
using Dictionary = std::unordered_map<std::string, MapType>;

template <typename MapType>
using HeapDictionary = std::unordered_map<std::string, std::unique_ptr<MapType>>;
}

#endif // CHALET_UTIL_TYPES_HPP
