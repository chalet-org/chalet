/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Json/JsonFile.hpp"

namespace chalet
{
struct IJsonFileReader
{
	IJsonFileReader() = default;
	CHALET_DEFAULT_COPY_MOVE(IJsonFileReader);
	virtual ~IJsonFileReader() = default;

protected:
	virtual bool readFrom(JsonFile& inJsonFile) = 0;
};
}
