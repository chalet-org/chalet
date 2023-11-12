/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "System/Logger.hpp"

// #ifdef _DEBUG
namespace
{
enum class LoggingLevel
{
	Normal,
	Detailed
};
static struct
{
	LoggingLevel loggingLevel = LoggingLevel::Normal;
} state;
}

namespace chalet
{
namespace priv
{
/*****************************************************************************/
void logNormal()
{
	state.loggingLevel = LoggingLevel::Normal;
}
void logDetailed()
{
	state.loggingLevel = LoggingLevel::Detailed;
}

/*****************************************************************************/
Logger::Logger(const std::string& inString)
{
	stream << inString;
}

/*****************************************************************************/
Logger::Logger(const char* const inFile, const std::string& inFunction)
{
	if (state.loggingLevel == LoggingLevel::Detailed)
		stream << Logger::classMethod(inFile, inFunction) << ": ";
}

/*****************************************************************************/
Logger::~Logger()
{
	if (uncaught >= std::uncaught_exceptions())
	{
		auto str = stream.str();
		std::cout.write(str.data(), str.size());
		std::cout.put('\n');
		std::cout.flush();
	}
}

/*****************************************************************************/
// Takes __FILE__ and __FUNCTION__ and parses __FILE__ to base name
//
std::string Logger::classMethod(const std::string& inFile, const std::string& inFunction)
{
	size_t lastSlash = inFile.find_last_of("/\\") + 1;
	size_t period = inFile.find_last_of('.');

	return inFile.substr(lastSlash, period - lastSlash) + "::" + inFunction + "()";
}
}
}
// #endif