/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

// #ifdef _DEBUG
#define LOG(...) priv::Logger(__FILE__, __FUNCTION__, __VA_ARGS__)
#define LOG_CLASS_METHOD() priv::Logger(priv::Logger::classMethod(__FILE__, __FUNCTION__))
#define LOG_LEVEL_NORMAL() priv::logNormal()
#define LOG_LEVEL_DETAILED() priv::logDetailed()
// #else
// 	#define LOG(...)
// 	#define LOG_CLASS_METHOD()
// 	#define LOG_LEVEL_NORMAL()
// 	#define LOG_LEVEL_DETAILED()
// #endif

// #ifdef _DEBUG

namespace chalet
{
namespace priv
{
void logNormal();
void logDetailed();

struct Logger
{
	explicit Logger(const std::string& inString);
	explicit Logger(const char* const inFile, const std::string& inFunction);
	~Logger();

	template <typename... Args>
	explicit Logger(const char* const inFile, const std::string& inFunction, Args&&... args);

	static std::string classMethod(const std::string& inFile, const std::string& inFunction);

private:
	std::stringstream stream;
	i32 uncaught = std::uncaught_exceptions();
};
} // namespace priv
}

#include "System/Logger.inl"

// #endif
