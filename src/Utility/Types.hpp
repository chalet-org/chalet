/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include <cstdint>

namespace chalet
{
using i8 = std::int8_t; // Note: signed char, NOT char
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using f32 = float;
using f64 = double;


// use when representing unsigned in char arrays
using uchar = unsigned char;

using size_t = std::size_t;
using ptrdiff_t = std::ptrdiff_t;
using uintmax_t = std::uintmax_t;

template <typename PtrType>
using Unique = std::unique_ptr<PtrType>;

template <typename PtrType>
using Ref = std::shared_ptr<PtrType>;

template <typename MapType>
using OrderedDictionary = std::map<std::string, MapType>;

template <typename MapType>
using Dictionary = std::unordered_map<std::string, MapType>;

template <typename MapType>
using HeapDictionary = std::unordered_map<std::string, Unique<MapType>>;
}
