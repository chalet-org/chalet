/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/DependencyWalker.hpp"

#include "Libraries/WindowsApi.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

// https://stackoverflow.com/questions/43670731/programmatically-get-list-of-dlls-used-to-build-a-process-or-library-in-a-non-de

#if !defined(CHALET_WIN32)
// This is so MinGW on Linux can read the dependencies of the compiled binary
namespace
{
using DWORD = std::uint32_t;
using WORD = std::uint16_t;
using BYTE = std::uint8_t;

typedef struct
{
	WORD Machine;
	WORD NumberOfSections;
	DWORD TimeDateStamp;
	DWORD PointerToSymbolTable;
	DWORD NumberOfSymbols;
	WORD SizeOfOptionalHeader;
	WORD Characteristics;
} IMAGE_FILE_HEADER;

typedef struct
{
	DWORD VirtualAddress;
	DWORD Size;
} IMAGE_DATA_DIRECTORY;

typedef struct
{
	BYTE Name[8];
	union
	{
		DWORD PhysicalAddress;
		DWORD VirtualSize;
	} Misc;
	DWORD VirtualAddress;
	DWORD SizeOfRawData;
	DWORD PointerToRawData;
	DWORD PointerToRelocations;
	DWORD PointerToLinenumbers;
	WORD NumberOfRelocations;
	WORD NumberOfLinenumbers;
	DWORD Characteristics;
} IMAGE_SECTION_HEADER;

typedef struct
{
	union
	{
		DWORD Characteristics;
		DWORD OriginalFirstThunk;
	};
	DWORD TimeDateStamp;

	DWORD ForwarderChain;
	DWORD Name;
	DWORD FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR;
}
#endif

#if !defined(CHALET_WIN32) && defined(__GNUC__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wcast-align"
#endif

namespace chalet
{
// #if defined(CHALET_WIN32)
/*****************************************************************************/
bool DependencyWalker::read(const std::string& inFile, StringList& outList, StringList* outNotFound)
{
	if (!verifyImageFile(inFile))
		return false;

	if (!parseFile(inFile, outList, outNotFound))
		return false;

	return true;
}

/*****************************************************************************/
bool DependencyWalker::verifyImageFile(const std::string& inFile)
{
	if (Commands::pathExists(inFile))
	{
		auto lower = String::toLowerCase(inFile);
		return String::endsWith(StringList{ ".dll", ".exe" }, lower);
	}

	return false;
}

/*****************************************************************************/
bool DependencyWalker::parseFile(const std::string& inFile, StringList& outList, StringList* outNotFound)
{
	std::vector<char> bytes = readAllBytes(inFile.c_str());
	StringList ignoreList{ inFile, "system32", "syswow64" };

	constexpr auto MAGIC_NUM_32BIT = static_cast<WORD>(0x10b);		// 267
	constexpr auto MAGIC_NUM_64BIT = static_cast<WORD>(0x20b);		// 523
	constexpr auto IMG_SIGNATURE_OFFSET = static_cast<int>(0x3c);	// 60
	constexpr auto IMPORT_TABLE_OFFSET_32 = static_cast<int>(0x68); // 104
	constexpr auto IMPORT_TABLE_OFFSET_64 = static_cast<int>(0x78); // 120
	constexpr auto IMG_SIGNATURE_SIZE = static_cast<int>(0x4);		// 4
	// constexpr auto OPT_HEADER_OFFSET_32 = static_cast<int>(0x1c);	// 28
	// constexpr auto OPT_HEADER_OFFSET_64 = static_cast<int>(0x18);	// 24
	// constexpr auto DATA_DIR_OFFSET_32 = static_cast<int>(0x60);		// 96
	// constexpr auto DATA_DIR_OFFSET_64 = static_cast<int>(0x70);		// 112
	// constexpr auto DATA_IAT_OFFSET_64 = static_cast<int>(0xD0);		// 208
	// constexpr auto DATA_IAT_OFFSET_32 = static_cast<int>(0xC0);		// 192
	// constexpr auto SZ_OPT_HEADER_OFFSET = static_cast<int>(0x10); // 16
	// constexpr auto RVA_AMOUNT_OFFSET_64 = static_cast<int>(0x6c);	// 108
	// constexpr auto RVA_AMOUNT_OFFSET_32 = static_cast<int>(0x5c);	// 92

	const char* KNOWN_IMG_SIGNATURE = static_cast<const char*>("PE\0\0");

	int foundDepends = 0;

	bool is64Bit = false;
	bool is32Bit = false;

	auto signatureOffsetLocation = (DWORD*)&bytes[IMG_SIGNATURE_OFFSET];
	auto signature = (char*)&bytes[*signatureOffsetLocation];

	if (*signature != *KNOWN_IMG_SIGNATURE)
		return false;

	DWORD coffFileHeaderOffset = *signatureOffsetLocation + IMG_SIGNATURE_SIZE;
	auto coffFileHeader = (IMAGE_FILE_HEADER*)&bytes[coffFileHeaderOffset];
	DWORD optionalFileHeaderOffset = coffFileHeaderOffset + sizeof(IMAGE_FILE_HEADER);

	// WORD sizeOfOptionalHeaderOffset = coffFileHeaderOffset + SZ_OPT_HEADER_OFFSET;
	// WORD* sizeOfOptionalHeader = (WORD*)&bytes[sizeOfOptionalHeaderOffset];

	//Magic is a 2-Byte value at offset-zero of the optional file header regardless of 32/64 bit
	WORD* magicNumber = (WORD*)&bytes[optionalFileHeaderOffset];

	if (*magicNumber == MAGIC_NUM_32BIT)
	{
		is32Bit = true;
	}
	else if (*magicNumber == MAGIC_NUM_64BIT)
	{
		is64Bit = true;
	}
	else
	{
		// Diagnostic::error("Could not parse magic number for 32 or 64-bit PE-format Image File.");
		return false;
	}

	if (is64Bit)
	{
		// auto imgOptHeader64 = (IMAGE_OPTIONAL_HEADER64*)&bytes[optionalFileHeaderOffset];
		auto importTableDataDir = (IMAGE_DATA_DIRECTORY*)&bytes[optionalFileHeaderOffset + IMPORT_TABLE_OFFSET_64];
		auto importTableAddress = (DWORD*)importTableDataDir;

		DWORD imageSectionHeaderOffset = optionalFileHeaderOffset + coffFileHeader->SizeOfOptionalHeader;

		for (int i = 0; i < coffFileHeader->NumberOfSections; i++)
		{
			auto queriedSectionHeader = (IMAGE_SECTION_HEADER*)&bytes[imageSectionHeaderOffset];
			if (*importTableAddress >= queriedSectionHeader->VirtualAddress && (*importTableAddress < (queriedSectionHeader->VirtualAddress + queriedSectionHeader->SizeOfRawData)))
			{
				DWORD importTableOffset = *importTableAddress - queriedSectionHeader->VirtualAddress + queriedSectionHeader->PointerToRawData;
				while (true)
				{
					auto importTableDescriptor = (IMAGE_IMPORT_DESCRIPTOR*)&bytes[importTableOffset];
					if (importTableDescriptor->OriginalFirstThunk == 0)
					{
						break; //Signifies end of IMAGE_IMPORT_DESCRIPTORs
					}
					// (VA from data directory _entry_ to Image Import Descriptor's element you want) - VA from section header + section header's PointerToRawData
					DWORD dependencyNameAddress = importTableDescriptor->Name; //VA not RVA; ABSOLUTE
					DWORD nameOffset = dependencyNameAddress - queriedSectionHeader->VirtualAddress + queriedSectionHeader->PointerToRawData;
					char* dependencyName = (char*)&bytes[nameOffset];

					std::string dependency = String::toLowerCase(std::string(dependencyName));
					std::string dependencyResolved = Commands::which(dependency);
					if (outNotFound != nullptr && dependencyResolved.empty())
					{
						outNotFound->push_back(dependency);
					}
					if (!dependency.empty())
					{
						auto lowercase = String::toLowerCase(dependencyResolved);
						if (!String::contains(ignoreList, lowercase))
						{
							// LOG(inFile, dependency);
							if (!dependencyResolved.empty())
								List::addIfDoesNotExist(outList, std::move(dependencyResolved));
							else
								List::addIfDoesNotExist(outList, std::move(dependency));
						}
					}
					++foundDepends;

					importTableOffset = importTableOffset + sizeof(IMAGE_IMPORT_DESCRIPTOR);
				}
			}
			imageSectionHeaderOffset = imageSectionHeaderOffset + sizeof(IMAGE_SECTION_HEADER);
		}
	}
	else if (is32Bit) //32-bit behavior
	{
		// auto imgOptHeader32 = (IMAGE_OPTIONAL_HEADER32*)&bytes[optionalFileHeaderOffset];
		auto importTableDataDir = (IMAGE_DATA_DIRECTORY*)&bytes[optionalFileHeaderOffset + IMPORT_TABLE_OFFSET_32];
		auto importTableAddress = (DWORD*)importTableDataDir;

		DWORD imageSectionHeaderOffset = optionalFileHeaderOffset + coffFileHeader->SizeOfOptionalHeader;

		for (int i = 0; i < coffFileHeader->NumberOfSections; i++)
		{
			auto queriedSectionHeader = (IMAGE_SECTION_HEADER*)&bytes[imageSectionHeaderOffset];
			if (*importTableAddress >= queriedSectionHeader->VirtualAddress && (*importTableAddress < (queriedSectionHeader->VirtualAddress + queriedSectionHeader->SizeOfRawData)))
			{
				DWORD importTableOffset = *importTableAddress - queriedSectionHeader->VirtualAddress + queriedSectionHeader->PointerToRawData;
				while (true)
				{
					auto importTableDescriptor = (IMAGE_IMPORT_DESCRIPTOR*)&bytes[importTableOffset];
					if (importTableDescriptor->OriginalFirstThunk == 0)
					{
						break; //Signifies end of IMAGE_IMPORT_DESCRIPTORs
					}

					// (VA from data directory _entry_ to Image Import Descriptor's element you want) - VA from section header + section header's PointerToRawData
					DWORD dependencyNameAddress = importTableDescriptor->Name; //VA not RVA; ABSOLUTE
					DWORD nameOffset = dependencyNameAddress - queriedSectionHeader->VirtualAddress + queriedSectionHeader->PointerToRawData;
					auto dependencyName = (char*)&bytes[nameOffset];

					std::string dependency = String::toLowerCase(std::string(dependencyName));
					std::string dependencyResolved = Commands::which(dependency);
					if (outNotFound != nullptr && dependencyResolved.empty())
					{
						outNotFound->push_back(dependency);
					}
					if (!dependency.empty())
					{
						auto lowercase = String::toLowerCase(dependencyResolved);
						if (!String::contains(ignoreList, lowercase))
						{
							// LOG(inFile, dependency);
							if (!dependencyResolved.empty())
								List::addIfDoesNotExist(outList, std::move(dependencyResolved));
							else
								List::addIfDoesNotExist(outList, std::move(dependency));
						}
					}
					++foundDepends;

					importTableOffset = importTableOffset + sizeof(IMAGE_IMPORT_DESCRIPTOR);
				}
			}
			imageSectionHeaderOffset = imageSectionHeaderOffset + sizeof(IMAGE_SECTION_HEADER);
		}
	}

	return true;
}

/*****************************************************************************/
std::vector<char> DependencyWalker::readAllBytes(const std::string& inFile)
{
	std::ifstream ifs(inFile, std::ios::binary | std::ios::ate);
	std::size_t length = ifs.tellg();

	std::vector<char> buffer(length);
	ifs.seekg(0, std::ios::beg);
	ifs.read(buffer.data(), length);

	return buffer;
}

// #endif
}

#if !defined(CHALET_WIN32) && defined(__GNUC__)
	#pragma GCC diagnostic pop
#endif
