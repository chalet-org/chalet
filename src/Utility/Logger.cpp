/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Logger.hpp"

// #ifdef _DEBUG
namespace
{
enum class LoggingLevel
{
	Normal,
	Detailed
};
LoggingLevel p_loggingLevel = LoggingLevel::Normal;
}

namespace chalet
{
namespace priv
{
/*****************************************************************************/
void logNormal()
{
	p_loggingLevel = LoggingLevel::Normal;
}
void logDetailed()
{
	p_loggingLevel = LoggingLevel::Detailed;
}

/*****************************************************************************/
Logger::Logger(const std::string& inString)
{
	stream << inString;
}

/*****************************************************************************/
Logger::Logger(const char* const inFile, const std::string& inFunction)
{
	if (p_loggingLevel == LoggingLevel::Detailed)
		stream << Logger::classMethod(inFile, inFunction) << ": ";
}

/*****************************************************************************/
Logger::~Logger()
{
	if (uncaught >= std::uncaught_exceptions())
		std::cout << stream.str() << std::endl;
}

/*****************************************************************************/
// Takes __FILE__ and __FUNCTION__ and parses __FILE__ to base name
//
std::string Logger::classMethod(const std::string& inFile, const std::string& inFunction)
{
	std::size_t lastSlash = inFile.find_last_of("/\\") + 1;
	std::size_t period = inFile.find_last_of(".");

	return inFile.substr(lastSlash, period - lastSlash) + "::" + inFunction + "()";
}
}
}
// #endif