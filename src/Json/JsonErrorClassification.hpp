/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_JSON_ERROR_CLASSIFICATION_HPP
#define CHALET_JSON_ERROR_CLASSIFICATION_HPP

namespace chalet
{
enum class JsonErrorClassification : ushort
{
	None,
	Fatal,
	Warning
};
}

#endif // CHALET_JSON_ERROR_CLASSIFICATION_HPP
