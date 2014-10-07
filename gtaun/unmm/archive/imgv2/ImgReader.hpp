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

#ifndef GTAUN_UNMM_ARCHIVE_IMGV2_IMGREADER_HPP
#define GTAUN_UNMM_ARCHIVE_IMGV2_IMGREADER_HPP

#include <stdint.h>
#include <string>
#include <vector>

namespace gtaun {
namespace unmm {
namespace archive {
namespace imgv2 {

class ImgReader {
public:
	static const int success = 0;
	static const int fail = 1;

	ImgReader() : state(fail), handle(INVALID_HANDLE_VALUE)
	{
	}

	ImgReader(const std::string& filePath) : state(fail), handle(INVALID_HANDLE_VALUE)
	{
		open(filePath);
	}

	~ImgReader() 
	{
		SCOPE_REMOVE_HOOKES

		if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
	}

	void open(const std::string& filePath)
	{
		if (handle != INVALID_HANDLE_VALUE || filePath.empty()) return;

		SCOPE_REMOVE_HOOKES

		handle = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if (handle == INVALID_HANDLE_VALUE) return;

		ReadFile(handle, header.version, sizeof(header.version), NULL, NULL);
		if (!header.checkVersion())
		{
			CloseHandle(handle);
			return;
		}

		ReadFile(handle, &header.fileEntries, sizeof(header.fileEntries), NULL, NULL);
		for (uint32_t i = 0; i < header.fileEntries; i++)
		{
			ImgEntry entry = { 0 };
			ReadFile(handle, &entry.offset, sizeof(entry.offset), NULL, NULL);
			ReadFile(handle, &entry.size, sizeof(entry.size), NULL, NULL);
			ReadFile(handle, entry.name, sizeof(entry.name), NULL, NULL);
			entries.push_back(entry);
		}

		state = success;
	}

	void read(uint32_t offsetBytes, uint32_t sizeBytes, void* buf)
	{
		if (!isOpen()) return;

		SCOPE_REMOVE_HOOKES

		SetFilePointer(handle, offsetBytes, NULL, FILE_BEGIN);
		ReadFile(handle, buf, sizeBytes, NULL, NULL);
	}

	std::vector<ImgEntry> getEntries()
	{
		if (!isOpen()) return std::vector<ImgEntry>();
		else return entries;
	}

	bool isOpen()
	{
		return state == success && handle != INVALID_HANDLE_VALUE;
	}

	bool checkVersion() 
	{ 
		if (!isOpen()) return false;
		else return header.checkVersion();
	}

private:
	ImgHeader header;
	HANDLE handle;
	std::vector<ImgEntry> entries;
	int state;
};

} // namespace imgv2
} // namespace archive
} // namespace unmm
} // namespace gtaun

#endif // GTAUN_UNMM_ARCHIVE_IMGV2_IMGREADER_HPP
