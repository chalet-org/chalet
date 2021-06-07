/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SOURCE_FILE_CACHE_HPP
#define CHALET_SOURCE_FILE_CACHE_HPP

namespace chalet
{
struct BuildCache;

struct SourceFileCache
{
	explicit SourceFileCache(BuildCache& inCache);

	bool initialize();
	bool save();

	bool fileChangedOrDoesNotExist(const std::string& inFile) const;

private:
	struct LastWrite
	{
		std::time_t lastWrite = 0;
		bool needsUpdate = true;
	};

	bool add(const std::string& inFile) const;
	LastWrite& getLastWrite(const std::string& inFile) const;

	BuildCache& m_cache;

	std::string m_filename;

	mutable std::unordered_map<std::string, LastWrite> m_lastWrites;

	std::time_t m_initializedTime = 0;
	std::time_t m_lastBuildTime = 0;

	mutable bool m_dirty = true;
};
}

#endif // CHALET_FILE_CACHE_HPP
