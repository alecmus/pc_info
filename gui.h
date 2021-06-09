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

#pragma once

#include "version_info.h"
#include "resource.h"
#include <liblec/lecui/containers/page.h>
#include <liblec/lecui/containers/pane.h>
#include <liblec/lecui/containers/tab_pane.h>
#include <liblec/lecui/appearance.h>
#include <liblec/lecui/controls.h>
#include <liblec/lecui/instance.h>
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/progress_bar.h>
#include <liblec/lecui/widgets/progress_indicator.h>
#include <liblec/lecui/utilities/splash.h>
#include <liblec/lecui/menus/form_menu.h>
#include <liblec/lecui/timer.h>
#include <liblec/leccore/pc_info.h>
using namespace liblec;
using snap_type = lecui::rect::snap_type;

class dashboard : public lecui::form {
	const float margin_ = 10.f;
	const float title_font_size_ = 12.f;
	const float highlight_font_size_ = 14.f;
	const float detail_font_size_ = 10.f;
	const float caption_font_size_ = 8.f;
	const std::string sample_text_ = "<u><strong>Aq</strong></u>";
	const std::string font_ = "Segoe UI";
	const lecui::color caption_color_{ 100, 100, 100 };
	const lecui::color ok_color_{ 0, 150, 0 };
	const lecui::color not_ok_color_{ 200, 0, 0 };
	const unsigned long refresh_interval_ = 3000;
	lecui::controls ctrls_{ *this };
	lecui::page_management page_man_{ *this };
	lecui::appearance apprnc_{ *this };
	lecui::dimensions dim_{ *this };
	lecui::instance_management instance_man_{ *this, "{F7660410-F00A-4BD0-B4B5-2A76F29D03E0}" };
	lecui::timer_management timer_man_{ *this };
	lecui::splash splash_{ *this };
	lecui::form_menu form_menu_{ *this };

	leccore::pc_info pc_info_;
	leccore::pc_info::pc_details pc_details_;
	std::vector<leccore::pc_info::cpu_info> cpus_;
	std::vector<leccore::pc_info::gpu_info> gpus_;
	leccore::pc_info::ram_info ram_;
	std::vector<leccore::pc_info::drive_info> drives_;
	leccore::pc_info::power_info power_;

	float title_height;
	float highlight_height;
	float detail_height;
	float caption_height;

	bool on_initialize(std::string& error) override;
	void on_start() override;
	void start_refresh_timer();
	void stop_refresh_timer();
	void about();
	bool on_layout(std::string& error) override;
	void add_battery_pane(lecui::containers::page& power_pane, const float top);
	void add_drive_details_pane(lecui::containers::page& drive_pane, const float top);
	void on_refresh();

public:
	dashboard(const std::string& caption);
};
