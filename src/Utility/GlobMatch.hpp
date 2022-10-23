/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_GLOB_MATCH_HPP
#define CHALET_GLOB_MATCH_HPP

namespace chalet
{
enum class GlobMatch : uchar
{
	Files,
	Folders,
	FilesAndFolders,
	FilesAndFoldersExact,
};
}

#endif // CHALET_GLOB_MATCH_HPP
