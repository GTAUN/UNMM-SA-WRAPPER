/**
 * Copyright (C) 2013 MK124
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

#ifndef GTAUN_UNMM_ARCHIVE_IMGV2_IMGHEADER_HPP
#define GTAUN_UNMM_ARCHIVE_IMGV2_IMGHEADER_HPP

#include <stdint.h>
#include <memory.h>

namespace gtaun {
namespace unmm {
namespace archive {
namespace imgv2 {

struct ImgHeader
{
	static const uint32_t VERSION_TAG = 0x32524556;		// ASCII: VER2


	char version[4];
	uint32_t fileEntries;


	bool checkVersion()
	{
		return memcmp(version, &VERSION_TAG, sizeof(version)) == 0;
	}
};

} // namespace imgv2
} // namespace archive
} // namespace unmm
} // namespace gtaun

#endif // GTAUN_UNMM_ARCHIVE_IMGV2_IMGHEADER_HPP
