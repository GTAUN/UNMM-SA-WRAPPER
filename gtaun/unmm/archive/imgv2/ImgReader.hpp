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
#include <fstream>
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

	ImgReader() : state(fail)
	{
	}

	ImgReader(std::string filePath) : state(fail)
	{
		open(filePath);
	}

	~ImgReader() 
	{
		if (stream.is_open()) stream.close();
	}

	void open(std::string filePath)
	{
		if (stream.is_open() || filePath.empty()) return;

		stream.open(filePath, std::ios::in | std::ios::binary);
		if (!stream.is_open()) return;

		stream.read(header.version, sizeof(header.version));
		if (!header.checkVersion())
		{
			stream.close();
			return;
		}

		stream.read((char*)&header.fileEntries, sizeof(uint32_t));
		for (uint32_t i = 0; i < header.fileEntries; i++)
		{
			ImgEntry entry;
			stream.read((char*)&entry.offset, sizeof(uint32_t));
			stream.read((char*)&entry.size, sizeof(uint32_t));
			stream.read(entry.name, 24);
			entries.push_back(entry);
		}

		state = success;
	}

	void read(uint32_t offsetBytes, uint32_t sizeBytes, void* buf)
	{
		stream.seekg(offsetBytes);
		stream.read((char*)buf, sizeBytes);
	}

	std::vector<ImgEntry> getEntries()
	{
		if (!isOpen()) return std::vector<ImgEntry>();
		else return entries;
	}

	bool isOpen()
	{
		return state == success && stream.is_open();
	}

	bool checkVersion() 
	{ 
		if (!isOpen()) return false;
		else return header.checkVersion();
	}

private:
	ImgHeader header;
	std::ifstream stream;
	std::vector<ImgEntry> entries;
	int state;
};

} // namespace imgv2
} // namespace archive
} // namespace unmm
} // namespace gtaun

#endif // GTAUN_UNMM_ARCHIVE_IMGV2_IMGREADER_HPP
