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

#include "gui.h"

// gui app using main
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

/// <summary>
/// Main application entry point.
/// </summary>
/// <returns>
/// Returns 1 if an error was encountered else returns 0.
/// </returns>
/// <remarks>
/// Supported command-line flags:
/// /cleanup: delete all application settings (prompts user if they would like to delete the app settings).
/// Designed to be used by the uninstaller.
/// /update: update exe running in temp directory. For overwriting files in install directory with the unzipped update files.
/// /recentupdate: new exe running from the install directory for the first time after an update.
/// /systemtray: start application in the background. Only the system tray will be visible and no splash screen will be displayed.
/// </remarks>
int main() {
	bool restart = false;

	do {
		std::string error;
		main_form fm(appname, restart);
		if (!fm.create(error)) {
			fm.message(error);
			return 1;
		}

		restart = fm.restart_now();
	} while (restart);

	return 0;
}
