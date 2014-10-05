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

#ifndef GTAUN_UNMM_ARCHIVE_IMGV2_IMGENTRY_HPP
#define GTAUN_UNMM_ARCHIVE_IMGV2_IMGENTRY_HPP

#include <stdint.h>

namespace gtaun {
namespace unmm {
namespace archive {
namespace imgv2 {

struct ImgEntry
{
	static const uint32_t BLOCK_SIZE = 2048;


	uint32_t offset;
	uint32_t size;
	char name[24];

	uint32_t getOffsetBytes() 
	{
		return offset * BLOCK_SIZE;
	}

	uint32_t getSizeBytes()
	{
		return size * BLOCK_SIZE;
	}
};

} // namespace imgv2
} // namespace archive
} // namespace unmm
} // namespace gtaun

#endif // GTAUN_UNMM_ARCHIVE_IMGV2_IMGENTRY_HPP
