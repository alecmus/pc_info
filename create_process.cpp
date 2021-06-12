/*
** MIT License
**
** Copyright(c) 2021 Alec Musasa
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files(the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions :
**
** The above copyright noticeand this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
*/

#include "create_process.h"
#include <Windows.h>

// for PathQuoteSpaces
#include <Shlwapi.h>
#pragma comment (lib, "Shlwapi.lib")

bool create_process(const std::string& fullpath,
	const std::vector<std::string>& args, std::string& error) {
	CHAR szAppPath[MAX_PATH];
	lstrcpynA(szAppPath, fullpath.c_str(), static_cast<int>(fullpath.length() + 1));
	PathQuoteSpacesA(szAppPath);

	// add commandline flags
	for (const auto& it : args) {
		lstrcatA(szAppPath, " ");
		lstrcatA(szAppPath, it.c_str());
	}

	STARTUPINFOA			si = { 0 };
	PROCESS_INFORMATION		pi = { 0 };
	si.cb = sizeof(STARTUPINFO);
	if (!CreateProcessA(NULL, szAppPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		error = "Creating process failed: " + fullpath;
		return false;
	}

	return true;
}
