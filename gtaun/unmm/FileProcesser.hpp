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

#ifndef GTAUN_UNMM_FILEPROCESSER_HPP
#define GTAUN_UNMM_FILEPROCESSER_HPP

#define COVERED_DIRECTORY "unmm\\"

#include <string>

namespace gtaun {
namespace unmm {

class FileProcesser
{
public:
	static const int none = 0;
	static const int dir = 1;
	static const int file = 2;

	FileProcesser() : coveredType(none)
	{
	}

	FileProcesser(std::string filePath) : coveredType(none)
	{
		if (filePath.find(':') != std::string::npos)
		{
			originalFullPath = filePath;
			
			if (utils::startWith(originalFullPath, getGameDir()))
				originalPassivePath = originalFullPath.substr(getGameDir().length() + 1);
		}
		else
		{
			originalPassivePath = filePath;
			originalFullPath = getGameDir() + "\\" + originalPassivePath;
		}

		coveredPassivePath = COVERED_DIRECTORY + originalPassivePath;
		coveredFullPath = getGameDir() + "\\" + coveredPassivePath;

		if (originalPassivePath.empty())
		{
			coveredType = none;
			return;
		}

		DWORD dwAttrib = GetFileAttributesA(coveredFullPath.c_str());
		if (dwAttrib == INVALID_FILE_ATTRIBUTES) coveredType = none;
		else if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) coveredType = dir;
		else coveredType = file;
	}

	int coveredBy() const
	{
		return coveredType;
	}

	std::string getCoveredPath() const 
	{
		return coveredFullPath;
	}

	std::string getExtension()
	{
		if (originalPassivePath.find_last_of('.') != std::string::npos)
			return utils::toLower(originalPassivePath.substr(originalPassivePath.find_last_of('.')));
		else
			return std::string();
	}

	static std::string getGameDir()
	{
		static std::string gameDir;

		if (gameDir.empty())
		{
			char buf[MAX_PATH];
			GetModuleFileNameA(NULL, buf, MAX_PATH);

			gameDir = std::string(buf).substr(0, std::string(buf).rfind('\\'));
		}

		return gameDir;
	}

private:
	std::string originalFullPath, originalPassivePath;
	std::string coveredFullPath, coveredPassivePath;
	int coveredType;
};

} // namespace unmm
} // namespace gtaun

#endif // GTAUN_UNMM_FILEPROCESSER_HPP
