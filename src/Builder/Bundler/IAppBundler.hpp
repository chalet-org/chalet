/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IAPP_BUNDLER_HPP
#define CHALET_IAPP_BUNDLER_HPP

namespace chalet
{
struct IAppBundler
{
	virtual ~IAppBundler() = default;

	virtual bool removeOldFiles(const bool inCleanOutput) = 0;
	virtual bool bundleForPlatform(const bool inCleanOutput) = 0;

	virtual std::string getBundlePath() const = 0;
	virtual std::string getExecutablePath() const = 0;
	virtual std::string getResourcePath() const = 0;
};
}

#endif // CHALET_IAPP_BUNDLER_HPP
