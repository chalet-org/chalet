/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct IJsonFileParser
{
	IJsonFileParser() = default;
	CHALET_DEFAULT_COPY_MOVE(IJsonFileParser);
	virtual ~IJsonFileParser() = default;

protected:
	virtual bool deserialize() = 0;
};
}
