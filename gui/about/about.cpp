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

#include "../../gui.h"

void main_form::about() {
	if (minimized())
		restore();
	else
		show();

	if (_about_open)
		return;

	manage_async_access _a(_about_open);

	std::string display_text =
		"<span style = 'font-size: 9.0pt;'>" +
		std::string(appname) + " " + std::string(appversion) + " (" + std::string(architecture) + "), " + std::string(appdate) +
		"</span>";

	display_text += "\n<span style = 'font-size: 8.0pt;'>© 2021 Alec Musasa</span>";

	display_text += "\n\n<strong>For more info</strong>\nVisit https://github.com/alecmus/pc_info";

	display_text += "\n\n<strong>Libraries used</strong>";
	display_text += "\n" + leccore::version();
	display_text += "\n" + lecui::version();

	display_text += "\n\n<strong>Additional credits</strong>\nMain icon designed by DinosoftLabs\nhttps://www.flaticon.com/authors/dinosoftlabs";
	display_text += "\nfrom https://www.flaticon.com";

	display_text += "\n\nThis app is free software released under the MIT License.";

	message(display_text);
}
