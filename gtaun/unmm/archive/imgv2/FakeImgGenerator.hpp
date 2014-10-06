/**
* Copyright (C) 2014 Shindo
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef GTAUN_UNMM_ARCHIVE_IMGV2_FAKEIMGENERATOR_HPP
#define GTAUN_UNMM_ARCHIVE_IMGV2_FAKEIMGENERATOR_HPP

#include <stdint.h>

#include <memory>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

namespace gtaun {
namespace unmm {
namespace archive {
namespace imgv2 {

class FakeImgGenerator
{
public:
	static const int success = 0;
	static const int fail = 1;
	static const int wrong_version = 2;

	FakeImgGenerator() : sizeBytes(0)
	{
	}

	FakeImgGenerator(std::string sourceImgPath, std::string coveredDirPath) : sourceImgPath(sourceImgPath), coveredDirPath(coveredDirPath), sizeBytes(0)
	{
		WIN32_FIND_DATAA fileData;
		HANDLE iterator;

		std::string listPath = coveredDirPath + "\\*.*";
		if ((iterator = FindFirstFileA(listPath.c_str(), &fileData)) != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) coveredFileList.push_back(fileData.cFileName);
			} while (FindNextFileA(iterator, &fileData));
		}
		FindClose(iterator);

		mapManager = std::shared_ptr<CoveringMapManager>(new CoveringMapManager());
		reader = std::shared_ptr<ImgReader>(new ImgReader());
	}

	static ImgHeader generateHeader(std::vector<ImgEntry>& entries)
	{
		ImgHeader header;
		memcpy(header.version, &header.VERSION_TAG, sizeof(header.version));
		header.fileEntries = entries.size();
		return header;
	}

	bool generate()
	{
		reader->open(sourceImgPath);
		if (!reader->isOpen()) return fail; 
		if (!reader->checkVersion()) return wrong_version;

		auto entries = reader->getEntries();
		for (auto& item : coveredFileList)
		{
			auto it = std::find_if(entries.begin(), entries.end(), [&](ImgEntry& entry) { return item == entry.name; });
			if (it != entries.end()) entries.erase(it);
		}

		uint32_t fakeOffset = 0;
		for (auto& item : entries)
		{
			ImgEntry entry = item;
			entry.fakeOffset = fakeOffset;
			fakeEntries.push_back(entry);
			
			fakeOffset += entry.size;
			sizeBytes += entry.size * ImgEntry::ENTRY_SIZE;
		}

		for (auto& item : coveredFileList)
		{
			ImgEntry entry = { 0 };
			std::string fileFullPath = coveredDirPath + "\\" + item;
			std::ifstream stream(fileFullPath);

			if (stream)
			{
				entry.offset = fakeOffset;
				entry.fakeOffset = fakeOffset;
				memcpy(entry.name, item.c_str(), item.length() <= sizeof(entry.name) ? item.length() : sizeof(entry.name));

				stream.seekg(0, std::ios::end);
				entry.size = (uint32_t) stream.tellg();
				// file size should be multiple of BLOCK_SIZE
				entry.size = entry.size < ImgEntry::BLOCK_SIZE ? 1 : (uint32_t) ceil((double) entry.size / ImgEntry::BLOCK_SIZE);
				fakeEntries.push_back(entry);

				fakeOffset += entry.size;
				sizeBytes += entry.size * ImgEntry::BLOCK_SIZE;
				stream.close();
			}
		}

		fakeHeader = generateHeader(fakeEntries);
		mapManager->registerCoveringBlock(0, ImgHeader::HEADER_SIZE, [&](void* buffer, size_t offset, size_t size) {
			assert(size == ImgHeader::HEADER_SIZE);
			memcpy(buffer, &fakeHeader.version, sizeof(fakeHeader.version));
			memcpy((byte*)buffer + sizeof(fakeHeader.version), &fakeHeader.fileEntries, sizeof(fakeHeader.fileEntries));
		});

		for (auto it = fakeEntries.begin(); it != fakeEntries.end(); it++)
		{
			size_t distance = it - fakeEntries.begin();
			size_t offset = ImgHeader::HEADER_SIZE + distance * ImgEntry::ENTRY_SIZE;

			mapManager->registerCoveringBlock(offset, ImgEntry::ENTRY_SIZE, [&](void* buffer, size_t offset, size_t size) {
				assert(size == ImgEntry::ENTRY_SIZE);
				memcpy(buffer, &it->offset, sizeof(it->offset));
				memcpy((byte*)buffer + sizeof(it->offset), &it->size, sizeof(it->size));
				memcpy((byte*)buffer + sizeof(it->offset) + sizeof(it->size), it->name, sizeof(it->name));
			});

			if (std::find(coveredFileList.begin(), coveredFileList.end(), it->name) != coveredFileList.end())
			{
				mapManager->registerCoveringBlock(it->fakeOffset * ImgEntry::BLOCK_SIZE, it->size * ImgEntry::BLOCK_SIZE, [&](void* buffer, size_t offset, size_t size) {
					assert(size == it->size * ImgEntry::BLOCK_SIZE);
					memset(buffer, 0, it->size * ImgEntry::BLOCK_SIZE);

					std::string fileFullPath = coveredDirPath + "\\" + it->name;
					std::ifstream stream(fileFullPath);
					
					if (stream)
					{
						stream.read((char*)buffer, it->size * ImgEntry::BLOCK_SIZE);
						stream.close();
					}
				});
			}
			else
			{
				mapManager->registerCoveringBlock(it->fakeOffset * ImgEntry::BLOCK_SIZE, it->size * ImgEntry::BLOCK_SIZE, [&](void* buffer, size_t offset, size_t size) {
					assert(size == it->size * ImgEntry::BLOCK_SIZE);
					memset(buffer, 0, it->size * ImgEntry::BLOCK_SIZE);
					reader->read(it->getOffsetBytes(), it->getSizeBytes(), buffer);
				});
			}
		}

		sizeBytes += ImgHeader::HEADER_SIZE + fakeEntries.size() * ImgEntry::ENTRY_SIZE;
		return success;
	}

	uint32_t getSizeBytes() const
	{
		return sizeBytes;
	}

	std::shared_ptr<CoveringMapManager> getMapManager()
	{
		return mapManager;
	}

private:
	std::string sourceImgPath, coveredDirPath;
	std::vector<std::string> coveredFileList;
	
	std::shared_ptr<ImgReader> reader;
	ImgHeader fakeHeader;
	std::vector<ImgEntry> fakeEntries;
	std::shared_ptr<CoveringMapManager> mapManager;

	uint32_t sizeBytes;
};

} // namespace imgv2
} // namespace archive
} // namespace unmm
} // namespace gtaun

#endif // GTAUN_UNMM_ARCHIVE_IMGV2_FAKEIMGENERATOR_HPP
