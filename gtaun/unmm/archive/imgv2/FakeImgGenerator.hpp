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
#include <map>
#include <string>
#include <vector>
#include <tuple>
#include <algorithm>
#include <functional>

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

private:
	typedef std::function<void(void*, size_t, size_t)> read_func_type;

public:

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
				if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					coveredFileList.push_back(fileData.cFileName);
					coveredFileSizeBytes[fileData.cFileName] = fileData.nFileSizeLow;
				}
				
			} while (FindNextFileA(iterator, &fileData));
			FindClose(iterator);
		}

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

		uint32_t sectionOriginalSizeBytes = ImgHeader::HEADER_SIZE + (entries.size() + coveredFileList.size()) * ImgEntry::ENTRY_SIZE;
		uint32_t sectionPaddingSizeBytes = sectionOriginalSizeBytes % ImgEntry::BLOCK_SIZE ? (sectionOriginalSizeBytes / ImgEntry::BLOCK_SIZE + 1) * ImgEntry::BLOCK_SIZE : sectionOriginalSizeBytes;
		sizeBytes = sectionPaddingSizeBytes;

		uint32_t fakeOffset = sectionPaddingSizeBytes / ImgEntry::BLOCK_SIZE;
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
			entry.fakeOffset = fakeOffset;
			memcpy(entry.name, item.c_str(), item.length() <= sizeof(entry.name) ? item.length() : sizeof(entry.name));

			// file size should be multiple of BLOCK_SIZE
			entry.size = coveredFileSizeBytes[item] < ImgEntry::BLOCK_SIZE ? 1 : (uint32_t)ceil((double)coveredFileSizeBytes[item] / ImgEntry::BLOCK_SIZE);
			fakeEntries.push_back(entry);

			fakeOffset += entry.size;
			sizeBytes += entry.size * ImgEntry::BLOCK_SIZE;
		}

		fakeHeader = generateHeader(fakeEntries);
		mapManager->registerCoveringBlock(0, ImgHeader::HEADER_SIZE, [&](void* buffer, size_t offset, size_t size) {
			std::vector<byte> data(ImgHeader::HEADER_SIZE, 0);
			memcpy(&data[0], &ImgHeader::VERSION_TAG, sizeof(ImgHeader::VERSION_TAG));
			memcpy(&data[0 + sizeof(ImgHeader::VERSION_TAG)], &fakeHeader.fileEntries, sizeof(fakeHeader.fileEntries));
			assert(offset + size <= data.size());
			memcpy(buffer, &data[0 + offset], size);
		});

		for (auto it = fakeEntries.begin(); it != fakeEntries.end(); ++it)
		{
			size_t distance = it - fakeEntries.begin();
			size_t offset = ImgHeader::HEADER_SIZE + distance * ImgEntry::ENTRY_SIZE;

			mapManager->registerCoveringBlock(offset, ImgEntry::ENTRY_SIZE, [&, distance](void* buffer, size_t offset, size_t size) {
				ImgEntry& entry = fakeEntries[distance];

				std::vector<byte> data(ImgEntry::ENTRY_SIZE, 0);
				memcpy(&data[0], &entry.fakeOffset, sizeof(entry.fakeOffset));
				memcpy(&data[0 + sizeof(entry.fakeOffset)], &entry.size, sizeof(entry.size));
				memcpy(&data[0 + sizeof(entry.fakeOffset) + sizeof(entry.size)], entry.name, sizeof(entry.name));
				assert(offset + size <= data.size());
				memcpy(buffer, &data[0 + offset], size);
			});

			if (std::find(coveredFileList.begin(), coveredFileList.end(), it->name) != coveredFileList.end())
			{
				mapManager->registerCoveringBlock(it->fakeOffset * ImgEntry::BLOCK_SIZE, it->size * ImgEntry::BLOCK_SIZE, [&, distance](void* buffer, size_t offset, size_t size) {
					assert(distance < fakeEntries.size());

					ImgEntry& entry = fakeEntries[distance];
					assert(size <= entry.getSizeBytes() - offset);
					std::string fileFullPath = coveredDirPath + "\\" + entry.name;

					SCOPE_REMOVE_HOOKES;

					HANDLE handle = CreateFileA(fileFullPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
					if (handle != INVALID_HANDLE_VALUE)
					{
						SetFilePointer(handle, offset, NULL, FILE_BEGIN);
						DWORD readBytes = 0;
						ReadFile(handle, buffer, size, &readBytes, NULL);
						CloseHandle(handle);
					}
				});
			}
			else
			{
				mapManager->registerCoveringBlock(it->fakeOffset * ImgEntry::BLOCK_SIZE, it->size * ImgEntry::BLOCK_SIZE, [&, distance](void* buffer, size_t offset, size_t size) {
					assert(distance < fakeEntries.size());

					ImgEntry& entry = fakeEntries[distance];
					assert(size <= entry.getSizeBytes() - offset);
					reader->read(entry.getOffsetBytes() + offset, size, buffer);
				});
			}
		}

		if (sectionPaddingSizeBytes > sectionOriginalSizeBytes)
		{
			mapManager->registerCoveringBlock(sectionOriginalSizeBytes, sectionPaddingSizeBytes - sectionOriginalSizeBytes, [&](void* buffer, size_t offset, size_t size) {
				memset(buffer, 0, size);
			});
		}

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
	std::map<std::string, uint32_t> coveredFileSizeBytes;
	
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
