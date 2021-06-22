/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CACHE_JSON_SCHEMA_HPP
#define CHALET_CACHE_JSON_SCHEMA_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
namespace Schema
{
Json getCacheJson();
Json getGlobalConfigJson();
}
}

#endif // CHALET_CACHE_JSON_SCHEMA_HPP
