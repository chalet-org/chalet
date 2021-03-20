/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_JSON_NODE_HPP
#define CHALET_JSON_NODE_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
namespace JsonNode
{
template <typename T>
bool assignFromKey(T& outVariable, const Json& inNode, const std::string& inKey);

template <typename T>
bool containsKeyForType(const Json& inNode, const std::string& inKey);
}
}

#include "Json/JsonNode.inl"

#endif // CHALET_JSON_NODE_HPP
