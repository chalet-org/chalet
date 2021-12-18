/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_JSON_NODE_READ_STATUS_HPP
#define CHALET_JSON_NODE_READ_STATUS_HPP

namespace chalet
{
enum class JsonNodeReadStatus
{
	Unread,
	ValidKeyUnreadValue,
	ValidKeyReadValue,
};
}

#endif // CHALET_JSON_NODE_READ_STATUS_HPP
