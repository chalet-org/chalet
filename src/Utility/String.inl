#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
inline T String::toInt(const std::string& inString)
{
	bool isNumeric = inString.find_first_not_of("1234567890") == std::string::npos;
	if (isNumeric)
	{
		return static_cast<T>(::atoi(inString.c_str()));
	}
	else
	{
		return static_cast<T>(0);
	}
}
template <typename T>
inline T String::toFloat(const std::string& inString)
{
	bool isNumeric = inString.find_first_not_of("1234567890.") == std::string::npos;
	if (isNumeric)
	{
		return static_cast<T>(::atof(inString.c_str()));
	}
	else
	{
		return static_cast<T>(0.0);
	}
}
}
