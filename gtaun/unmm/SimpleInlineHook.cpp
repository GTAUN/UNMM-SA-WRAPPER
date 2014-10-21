/**
* Copyright (C) 2014 MK124
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "SimpleInlineHook.hpp"

namespace gtaun {
namespace unmm {

#if defined(WIN32)

#include <windows.h>

void SimpleInlineHook::removePageProtect(void* address, size_t size)
{
	DWORD oldState;
	VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldState);
}

#else

#include <sys/mman.h>

void SimpleInlineHook::removePageProtect(void* address, size_t size)
{
	mprotect(address, size, PROT_READ | PROT_WRITE | PROT_EXEC);
}

#endif

} // namespace unmm
} // namespace gtaun
