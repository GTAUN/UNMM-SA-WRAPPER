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

#ifndef GTAUN_UNMM_UTILS_UTILS_HPP
#define GTAUN_UNMM_UTILS_UTILS_HPP

#include <string>

namespace gtaun {
namespace unmm {
namespace utils {

bool endWith(const std::string& s, const std::string& tail)
{
	return !s.compare(s.size() - tail.size(), tail.size(), tail);
}

bool startWith(const std::string& s, const std::string& head)
{
	return !s.compare(0, head.size(), head);
}

std::string toLower(std::string &s)
{
	std::string ret = s;
	for (auto& c : ret)
		c = tolower(c);
	return ret;
}

std::string& trim(std::string &s)
{
	if (s.find_first_not_of(' ') != std::string::npos)
		s.erase(0, s.find_first_not_of(' '));

	if (s.find_last_not_of(' ') != std::string::npos)
		if (s.find_last_not_of(' ') + 1 < s.length())
			s.erase(s.find_last_not_of(' ') + 1);

	return s;
}

} // namespace utils
} // namespace unmm
} // namespace gtaun

#define SCOPE_REMOVE_HOOKES \
	gtaun::unmm::SimpleInlineHook::ScopeUnhook createFileHookRemove(createFileHook); \
	gtaun::unmm::SimpleInlineHook::ScopeUnhook closeHandleHookRemove(closeHandleHook); \
	gtaun::unmm::SimpleInlineHook::ScopeUnhook readFileHookRemove(readFileHook); \
	gtaun::unmm::SimpleInlineHook::ScopeUnhook readFileExHookRemove(readFileExHook); \
	gtaun::unmm::SimpleInlineHook::ScopeUnhook setFilePointerHookRemove(setFilePointerHook); \
	// gtaun::unmm::SimpleInlineHook::ScopeUnhook createFileMappingHookRemove(createFileMappingHook);

extern gtaun::unmm::SimpleInlineHook createFileHook, closeHandleHook, readFileHook, readFileExHook, setFilePointerHook, createFileMappingHook;

#endif // GTAUN_UNMM_UTILS_UTILS_HPP
