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

class main_form : public lecui::form {
	static const float margin_;
	static const float title_font_size_;
	static const float highlight_font_size_;
	static const float detail_font_size_;
	static const float caption_font_size_;
	static const std::string sample_text_;
	static const std::string font_;
	static const lecui::color caption_color_;
	static const lecui::color ok_color_;
	static const lecui::color not_ok_color_;
	static const unsigned long refresh_interval_;
	
	const std::string instance_guid_ = "{F7660410-F00A-4BD0-B4B5-2A76F29D03E0}";
	const std::string install_guid_32_ = "{4366AB0F-A68F-4388-B4FA-2BE684F86FC4}";
	const std::string install_guid_64_ = "{5F794184-4C64-402C-AE99-E88BBF681851}";

	bool restart_now_ = false;

	// 1. If application is installed and running from an install directory this will be true.
	// 2. If application is installed and not running from an install directory this will also
	// be true unless there is a .portable file in the same directory.
	// 3. If application is not installed then portable mode will be used whether or not a .portable
	// file exists in the same directory.
	bool installed_ = false;
	bool setting_darktheme_ = false;

	lecui::controls ctrls_{ *this };
	lecui::page_management page_man_{ *this };
	lecui::appearance apprnc_{ *this };
	lecui::dimensions dim_{ *this };
	lecui::instance_management instance_man_{ *this, instance_guid_ };
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
	void settings();
	bool on_layout(std::string& error) override;
	void add_battery_pane(lecui::containers::page& power_pane, const float top);
	void add_drive_details_pane(lecui::containers::page& drive_pane, const float top);
	void on_refresh();

public:
	main_form(const std::string& caption);
	bool restart_now() {
		return restart_now_;
	}
};
