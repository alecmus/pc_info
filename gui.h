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
#include <liblec/lecui/utilities/timer.h>
#include <liblec/lecui/utilities/splash.h>
#include <liblec/lecui/utilities/tray_icon.h>
#include <liblec/lecui/menus/form_menu.h>
#include <liblec/leccore/pc_info.h>
#include <liblec/leccore/web_update.h>
#include <liblec/leccore/settings.h>

using namespace liblec;
using snap_type = lecui::rect::snap_type;

// the main form
class main_form : public lecui::form {
	const std::string _instance_guid = "{F7660410-F00A-4BD0-B4B5-2A76F29D03E0}";
	const std::string _install_guid_32 = "{4366AB0F-A68F-4388-B4FA-2BE684F86FC4}";
	const std::string _install_guid_64 = "{5F794184-4C64-402C-AE99-E88BBF681851}";
	const std::string _update_xml_url = "https://raw.githubusercontent.com/alecmus/pc_info/master/latest_update.xml";

	static const float _margin;
	static const float _title_font_size;
	static const float _highlight_font_size;
	static const float _detail_font_size;
	static const float _caption_font_size;
	static const std::string _sample_text;
	static const std::string _font;
	static const lecui::color _caption_color;
	static const lecui::color _ok_color;
	static const lecui::color _not_ok_color;
	static const unsigned long _refresh_interval;

	bool _restart_now = false;

	// 1. If application is installed and running from an install directory this will be true.
	// 2. If application is installed and not running from an install directory this will also
	// be true unless there is a .portable file in the same directory.
	// 3. If application is not installed then portable mode will be used whether or not a .portable
	// file exists in the same directory.
	bool _installed;
	bool _real_portable_mode;
	bool _system_tray_mode;
	std::string _install_location_32, _install_location_64;
	leccore::settings& _settings;
	leccore::registry_settings _reg_settings{ leccore::registry::scope::current_user };
	leccore::ini_settings _ini_settings{ "pc_info.ini" };
	bool _setting_darktheme = false;
	bool _setting_milliunits = true;
	bool _setting_milliunits_old = _setting_milliunits;
	bool _setting_autocheck_updates = true;
	leccore::check_update _check_update{ _update_xml_url };
	leccore::check_update::update_info _update_info;
	bool _setting_autodownload_updates = false;
	bool _update_check_initiated_manually = false;
	leccore::download_update _download_update;
	std::string _update_directory;
	bool _setting_autostart = false;

	const bool _cleanup_mode;
	const bool _update_mode;
	const bool _recent_update_mode;

	bool _settings_open = false;
	bool _about_open = false;

	lecui::controls _ctrls{ *this };
	lecui::page_manager _page_man{ *this };
	lecui::appearance _apprnc{ *this };
	lecui::dimensions _dim{ *this };
	lecui::instance_manager _instance_man{ *this, _instance_guid };
	lecui::timer_manager _timer_man{ *this };
	lecui::splash _splash{ *this };
	lecui::form_menu _form_menu{ *this };
	lecui::tray_icon _tray_icon{ *this };

	leccore::pc_info _pc_info;
	leccore::pc_info::pc_details _pc_details;
	std::vector<leccore::pc_info::cpu_info> _cpus;
	std::vector<leccore::pc_info::gpu_info> _gpus;
	leccore::pc_info::ram_info _ram;
	std::vector<leccore::pc_info::drive_info> _drives;
	leccore::pc_info::power_info _power;

	float title_height;
	float highlight_height;
	float detail_height;
	float caption_height;

	bool on_initialize(std::string& error) override;
	void on_start() override;
	void on_close() override;
	void start_refresh_timer();
	void stop_refresh_timer();
	void about();
	void settings();
	void updates();
	bool on_layout(std::string& error) override;
	void add_battery_pane(lecui::containers::page& power_pane, const float top);
	void add_drive_details_pane(lecui::containers::page& drive_pane, const float top);
	void on_refresh();
	void on_update_check();
	void on_update_download();
	bool installed();

public:
	main_form(const std::string& caption);
	~main_form();
	bool restart_now() {
		return _restart_now;
	}
};

// helper class for managing asynchronous access to a method
// sets the param to true for as long as the object is within scope
class manage_async_access {
	bool& _param;

public:
	manage_async_access(bool& param) :
		_param(param) {
		param = true;
	}
	~manage_async_access() { _param = false; }

private:
	manage_async_access() = delete;
};
