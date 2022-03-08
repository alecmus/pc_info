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

// liblec
#include <liblec/lecui/containers/pane.h>
#include <liblec/lecui/containers/tab_pane.h>
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/progress_bar.h>
#include <liblec/lecui/widgets/progress_indicator.h>
#include <liblec/lecui/utilities/filesystem.h>

// leccore
#include <liblec/leccore/system.h>
#include <liblec/leccore/app_version_info.h>
#include <liblec/leccore/hash.h>
#include <liblec/leccore/file.h>

// STL
#include <filesystem>

const float main_form::_margin = 10.f;
const float main_form::_title_font_size = 12.f;
const float main_form::_highlight_font_size = 14.f;
const float main_form::_detail_font_size = 10.f;
const float main_form::_caption_font_size = 8.f;
const std::string main_form::_sample_text = "<u><strong>Aq</strong></u>";
const std::string main_form::_font = "Segoe UI";
const lecui::color main_form::_ok_color{ lecui::color().red(0).green(150).blue(0) };
const lecui::color main_form::_not_ok_color{ lecui::color().red(200).green(0).blue(0) };
const unsigned long main_form::_refresh_interval = 3000;

void main_form::updates() {
	if (_check_update.checking() || _timer_man.running("update_check"))
		return;

	if (_timer_man.running("start_update_check"))
		_timer_man.stop("start_update_check");

	if (_download_update.downloading() || _timer_man.running("update_download"))
		return;

	std::string error, value;
	if (!_settings.read_value("updates", "readytoinstall", value, error)) {}

	if (!value.empty()) {
		// file integrity confirmed ... install update
		if (prompt("An update has already been downloaded and is ready to be installed.\nWould you like to apply the update now?")) {
			_restart_now = true;
			close();
		}
		return;
	}

	_update_check_initiated_manually = true;

	// create update status
	create_update_status();

	// start checking for updates
	_check_update.start();

	// start timer to keep progress of the update check
	_timer_man.add("update_check", 1500, [&]() { on_update_check(); });
}

void main_form::copy_pc_info() {
	// make text string containing full pc info
	std::string text;
	text += pc_details_text();
	
	if (!_power.batteries.empty())
		text += power_details_text();

	text += cpu_details_text();
	text += graphics_details_text();
	text += ram_details_text();
	text += drive_details_text();

	// set the text to the clipboard
	std::string error;
	if (!leccore::clipboard::set_text(text, error))
		message(error);
	else
		message("All PC info copied to the clipboard.");
}

void main_form::export_pc_info() {
	// make text string containing full pc info
	std::string text;
	text += pc_details_text();

	if (!_power.batteries.empty())
		text += power_details_text();

	text += cpu_details_text();
	text += graphics_details_text();
	text += ram_details_text();
	text += drive_details_text();
	text += "\n-------------------------------------------------------------------------------\n";
	text += "Exported from " + std::string(appname) + " " + std::string(appversion) + " (" + std::string(architecture) + ")";

	lecui::filesystem _file_system(*this);

	lecui::save_file_params params;
	params
		.title(std::string(appname) + " - Export all info")
		.include_all_files(false)
		.file_types({ { "txt", "Text Document" } });

	// get the full path to the file (prompt user)
	const auto full_path = _file_system.save_file("pc_info - " + _pc_details.name + ".txt", params);

	if (!full_path.empty()) {
		// save the file
		std::string error;
		if (!leccore::file::write(full_path, text, error))
			message(error);
		else {
			// open the file
			if (!leccore::shell::open(full_path, error))
				message(error);
		}
	}
}

void main_form::on_start() {
	start_refresh_timer();

	if (_installed) {
		std::string error;
		if (!_tray_icon.add(ico_resource, std::string(appname) + " " +
			std::string(appversion) + " (" + std::string(architecture) + ")",
			{
			{ "<strong>Show PC Info</strong>", [this]() {
				if (minimized())
					restore();
				else
					show();
			} },
			{ "" },
			{ "Settings", [this]() { settings(); } },
			{ "Updates", [this]() { updates(); } },
			{ "About", [this]() { about(); } },
			{ "" },
			{ "Exit", [this]() { close(); } }
			},
			"Show PC Info", error)) {
		}
	}

	_splash.remove();
}

void main_form::start_refresh_timer() {
	_timer_man.add("refresh", _refresh_interval, [&]() { on_refresh(); });
}

void main_form::stop_refresh_timer() {
	_timer_man.stop("refresh");
}

void main_form::on_refresh() {
	if (!visible())
		return;

	stop_refresh_timer();
	bool refresh_ui = false;

	std::string error;
	std::vector<leccore::pc_info::monitor_info> _monitors_old = _monitors;
	if (!_pc_info.monitor(_monitors, error)) {}

	std::vector<leccore::pc_info::drive_info> _drives_old = _drives;
	if (!_pc_info.drives(_drives, error)) {}

	leccore::pc_info::power_info _power_old = _power;
	if (!_pc_info.power(_power, error)) {}

	try {
		// refresh pc details
		if (_monitors_old.size() != _monitors.size()) {
			auto& monitor_summary = get_label("home/pc_details_pane/monitor_summary");
			monitor_summary.text(std::to_string(_monitors.size()) + "<span style = 'font-size: 8.0pt;'>" +
				std::string(_monitors.size() == 1 ? " monitor" : " monitors") +
				"</span>");
		}

		if (_drives_old.size() != _drives.size()) {
			auto& drive_summary = get_label("home/pc_details_pane/drive_summary");
			drive_summary.text(std::to_string(_drives.size()) + "<span style = 'font-size: 8.0pt;'>" +
				std::string(_drives.size() == 1 ? " drive" : " drives") +
				"</span>");
		}

		if (_power_old.batteries.size() != _power.batteries.size()) {
			auto& battery_summary = get_label("home/pc_details_pane/battery_summary");
			battery_summary.text(std::to_string(_power.batteries.size()) + "<span style = 'font-size: 8.0pt;'>" +
				std::string(_power.batteries.size() == 1 ? " battery" : " batteries") +
				"</span>");
		}
	}
	catch (const std::exception) {}

	try {
		// refresh power details
		if (_power_old.ac != _power.ac) {
			auto& power_status = get_label("home/power_pane/power_status");
			power_status.text() = _power.ac ? "On AC" : "On Battery";
			power_status.text() += ", ";
			power_status.text() += ("<span style = 'font-size: 8.0pt;'>" +
				_pc_info.to_string(_power.status) + "</span>");
			refresh_ui = true;
		}

		if (_power_old.level != _power.level) {
			auto& level = get_label("home/power_pane/level");
			level.text((_power.level != -1 ?
				(std::to_string(_power.level) + "% ") : std::string("<em>Unknown</em> ")) +
				"<span style = 'font-size: 8.0pt;'>overall power level</span>");

			auto& level_bar = get_progress_bar("home/power_pane/level_bar");
			level_bar.percentage(static_cast<float>(_power.level));
			refresh_ui = true;
		}

		if (_power_old.lifetime_remaining != _power.lifetime_remaining) {
			auto& life_remaining = get_label("home/power_pane/life_remaining");
			life_remaining.text(_power.lifetime_remaining.empty() ? std::string() :
				(_power.lifetime_remaining + " remaining"));
			refresh_ui = true;
		}

		if (_power_old.batteries.size() != _power.batteries.size()) {
			auto& cpu_pane = get_pane("home/cpu_pane");
			auto& graphics_pane = get_pane("home/graphics_pane");
			auto& ram_pane = get_pane("home/ram_pane");
			auto& drive_pane = get_pane("home/drive_pane");

			if (_power_old.batteries.empty()) {
				add_power_pane();
				add_battery_pane();

				auto& power_pane = get_pane("home/power_pane");

				// move panes to accomodate power pane
				cpu_pane.rect().move(power_pane.rect().right() + _margin, cpu_pane.rect().top());
				graphics_pane.rect().move(power_pane.rect().right() + _margin, graphics_pane.rect().top());
				ram_pane.rect().move(cpu_pane.rect().right() + _margin, ram_pane.rect().top());
				drive_pane.rect().move(ram_pane.rect().left(), drive_pane.rect().top());
			}
			else {
				if (_power.batteries.empty()) {
					_page_man.close("home/power_pane");

					auto& pc_details_pane = get_pane("home/pc_details_pane");

					// move panes since power pane has been removed
					cpu_pane.rect().move(pc_details_pane.rect().right() + _margin, cpu_pane.rect().top());
					graphics_pane.rect().move(pc_details_pane.rect().right() + _margin, graphics_pane.rect().top());
					ram_pane.rect().move(cpu_pane.rect().right() + _margin, ram_pane.rect().top());
					drive_pane.rect().move(ram_pane.rect().left(), drive_pane.rect().top());
				}
				else {
					// close old battery pane
					_page_man.close("home/power_pane/battery_tab_pane");

					// add new battery pane
					add_battery_pane();
				}
			}

			refresh_ui = true;
		}
		else {
			for (size_t battery_number = 0; battery_number < _power.batteries.size(); battery_number++) {
				auto& battery_old = _power_old.batteries[battery_number];
				auto& battery = _power.batteries[battery_number];

				if (battery_old != battery ||
					_setting_milliunits_old != _setting_milliunits) {
					if (battery_old.current_capacity != battery.current_capacity ||
						_setting_milliunits_old != _setting_milliunits) {
						auto& current_capacity = get_label("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/current_capacity");
						current_capacity.text(battery.current_capacity == -1 ? "Unknown" :
							_setting_milliunits ?
							std::to_string(battery.current_capacity) + "mWh" :
							leccore::round_off::to_string(battery.current_capacity / 1000.f, 1) + "Wh");
					}

					if (battery_old.level != battery.level) {
						auto& charge_level = get_label("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/charge_level");
						charge_level.text(leccore::round_off::to_string(battery.level, 1) + "%");
					}

					if (battery_old.current_charge_rate != battery.current_charge_rate) {
						auto& charge_rate = get_label("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/charge_rate");
						charge_rate.text(_setting_milliunits ?
							std::to_string(battery.current_charge_rate) + "mW" :
							leccore::round_off::to_string(battery.current_charge_rate / 1000.f, 1) + "W");
					}

					if (battery_old.current_voltage != battery.current_voltage ||
						_setting_milliunits_old != _setting_milliunits) {
						auto& current_voltage = get_label("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/current_voltage");
						current_voltage.text(battery.current_voltage == -1 ? "Unknown" :
							_setting_milliunits ?
							std::to_string(battery.current_voltage) + "mV" :
							leccore::round_off::to_string(battery.current_voltage / 1000.f, 2) + "V");
					}

					if (battery_old.status != battery.status) {
						auto& status = get_label("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/status");
						status.text(_pc_info.to_string(battery.status));
					}

					if (battery_old.designed_capacity != battery.designed_capacity ||
						_setting_milliunits_old != _setting_milliunits) {
						auto& designed_capacity = get_label("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/designed_capacity");
						designed_capacity.text(_setting_milliunits ?
							std::to_string(battery.designed_capacity) + "mWh" :
							leccore::round_off::to_string(battery.designed_capacity / 1000.f, 1) + "Wh");
					}

					if (battery_old.fully_charged_capacity != battery.fully_charged_capacity ||
						_setting_milliunits_old != _setting_milliunits) {
						auto& fully_charged_capacity = get_label("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/fully_charged_capacity");
						fully_charged_capacity.text(_setting_milliunits ?
							std::to_string(battery.fully_charged_capacity) + "mWh" :
							leccore::round_off::to_string(battery.fully_charged_capacity / 1000.f, 1) + "Wh");
					}

					if (battery_old.health != battery.health) {
						auto& health = get_progress_indicator("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/health");
						health.percentage(static_cast<float>(battery.health));
					}

					refresh_ui = true;
				}
			}

			_setting_milliunits_old = _setting_milliunits;
		}
	}
	catch (const std::exception) {}

	try {
		// to-do: refresh monitor details
		if (_monitors_old.size() != _monitors.size()) {
			// close old monitor tab pane
			_page_man.close("home/graphics_pane/monitor_tab_pane");

			// add new monitor tab pane
			add_monitor_tab_pane();

			refresh_ui = true;
		}
	}
	catch (const std::exception) {}

	try {
		// to-do: refresh drive details
		if (_drives_old.size() != _drives.size()) {
			// close old drive tab pane
			_page_man.close("home/drive_pane/drive_tab_pane");

			// add new drive tab pane
			add_drive_tab_pane();

			refresh_ui = true;
		}
		else {
			for (size_t drive_number = 0; drive_number < _drives.size(); drive_number++) {
				auto& drive_old = _drives_old[drive_number];
				auto& drive = _drives[drive_number];

				if (drive_old.status != drive.status) {
					auto& status = get_label("home/drive_pane/drive_tab_pane/Drive " + std::to_string(drive_number) + "/status");
					status.text(drive.status);
					if (drive.status == "OK")
						status.color_text(_ok_color);
					else {
						status.color_text(_not_ok_color);
						// to-do: handle more cases
					}

					refresh_ui = true;
				}

				if (drive_old.storage_type.empty()) {
					auto& storage_type = get_label("home/drive_pane/drive_tab_pane/Drive " + std::to_string(drive_number) + "/storage_type");
					storage_type.text(drive.storage_type);

					refresh_ui = true;
				}

				if (drive_old.bus_type.empty()) {
					auto& bus_type = get_label("home/drive_pane/drive_tab_pane/Drive " + std::to_string(drive_number) + "/bus_type");
					bus_type.text(drive.bus_type);

					refresh_ui = true;
				}
			}
		}
	}
	catch (const std::exception) {}

	if (refresh_ui)
		update();

	start_refresh_timer();
}

void main_form::on_update_check() {
	if (_check_update.checking())
		return;

	// update status label
	try {
		auto& text = get_label("home/update_status").text();
		const size_t dot_count = std::count(text.begin(), text.end(), '.');

		if (dot_count == 0)
			text = "Checking for updates ...";
		else
			if (dot_count < 10)
				text += ".";
			else
				text = "Checking for updates ...";
		
		update();
	}
	catch (const std::exception&) {}

	// stop the update check timer
	_timer_man.stop("update_check");

	std::string error;
	if (!_check_update.result(_update_info, error)) {
		// update status label
		try {
			get_label("home/update_status").text("Error while checking for updates");
			update();
			close_update_status();
		}
		catch (const std::exception&) {}

		if (!_setting_autocheck_updates || _update_check_initiated_manually)
			message("An error occurred while checking for updates:\n" + error);

		return;
	}
	
	// update found
	const std::string current_version(appversion);
	const int result = leccore::compare_versions(current_version, _update_info.version);
	if (result == -1) {
		// newer version available
		
		// update status label
		try {
			get_label("home/update_status").text("Update available: " + _update_info.version);
			update();
		}
		catch (const std::exception&) {}

		if (!_setting_autodownload_updates) {
			if (!prompt("<span style = 'font-size: 11.0pt;'>Update Available</span>\n\n"
				"Your version:\n" + current_version + "\n\n"
				"New version:\n<span style = 'color: rgb(0, 150, 0);'>" + _update_info.version + "</span>, " + _update_info.date + "\n\n"
				"<span style = 'font-size: 11.0pt;'>Description</span>\n" +
				_update_info.description + "\n\n"
				"Would you like to download the update now? (" +
				leccore::format_size(_update_info.size, 2) + ")\n\n"))
				return;
		}

		// create update download folder
		_update_directory = leccore::user_folder::temp() + "\\" + leccore::hash_string::uuid();

		if (!leccore::file::create_directory(_update_directory, error))
			return;	// to-do: perhaps try again one or two more times? But then again, why would this method fail?

		// update status label
		try {
			get_label("home/update_status").text("Downloading update ...");
			update();
		}
		catch (const std::exception&) {}

		// download update
		_download_update.start(_update_info.download_url, _update_directory);
		_timer_man.add("update_download", 1000, [&]() { on_update_download(); });
	}
	else {
		// update status label
		try {
			get_label("home/update_status").text("Latest version is already installed");
			update();
			close_update_status();
		}
		catch (const std::exception&) {}

		if (!_setting_autocheck_updates || _update_check_initiated_manually)
			message("The latest version is already installed.");
	}
}

void main_form::on_close() {
	if (_installed)
		hide();
	else
		close();
}

void main_form::on_update_download() {
	leccore::download_update::download_info progress;
	if (_download_update.downloading(progress)) {
		// update status label
		try {
			auto& text = get_label("home/update_status").text();
			text = "Downloading update ...";

			if (progress.file_size > 0)
				text += " " + leccore::round_off::to_string(100. * (double)progress.downloaded / progress.file_size, 0) + "%";

			update();
		}
		catch (const std::exception&) {}
		return;
	}

	// stop the update download timer
	_timer_man.stop("update_download");

	auto delete_update_directory = [&]() {
		std::string error;
		if (!leccore::file::remove_directory(_update_directory, error)) {}
	};

	std::string error, fullpath;
	if (!_download_update.result(fullpath, error)) {
		// update status label
		try {
			get_label("home/update_status").text("Downloading update failed");
			update();
			close_update_status();
		}
		catch (const std::exception&) {}

		message("Download of update failed:\n" + error);
		delete_update_directory();
		return;
	}
	
	// update downloaded ... check file hash
	leccore::hash_file hash;
	hash.start(fullpath, { leccore::hash_file::algorithm::sha256 });

	while (hash.hashing()) {
		if (!keep_alive()) {
			// to-do: implement stopping mechanism
			//hash.stop()
			delete_update_directory();
			close_update_status();
			return;
		}
	}

	leccore::hash_file::hash_results results;
	if (!hash.result(results, error)) {
		// update status label
		try {
			get_label("home/update_status").text("Update file integrity check failed");
			update();
			close_update_status();
		}
		catch (const std::exception&) {}

		message("Update downloaded but file integrity check failed:\n" + error);
		delete_update_directory();
		return;
	}

	try {
		const auto& result_hash = results.at(leccore::hash_file::algorithm::sha256);
		if (result_hash != _update_info.hash) {
			// update status label
			try {
				get_label("home/update_status").text("Update files seem to be corrupt");
				update();
				close_update_status();
			}
			catch (const std::exception&) {}

			// update file possibly corrupted
			message("Update downloaded but files seem to be corrupt and so cannot be installed. "
				"If the problem persists try downloading the latest version of the app manually.");
			delete_update_directory();
			return;
		}
	}
	catch (const std::exception& e) {
		error = e.what();

		// update status label
		try {
			get_label("home/update_status").text("Update file integrity check failed");
			update();
			close_update_status();
		}
		catch (const std::exception&) {}

		message("Update downloaded but file integrity check failed:\n" + error);
		delete_update_directory();
		return;
	}

	// save update location and update architecture
	const std::string update_architecture(architecture);
	if (!_settings.write_value("updates", "readytoinstall", fullpath, error) ||
		!_settings.write_value("updates", "architecture", update_architecture, error) ||
		!_settings.write_value("updates", "tempdirectory", _update_directory, error)) {

		// update status label
		try {
			get_label("home/update_status").text("Downloading update failed");	// to-do: improve
			update();
			close_update_status();
		}
		catch (const std::exception&) {}

		message("Update downloaded and verified but the following error occurred:\n" + error);
		delete_update_directory();
		return;
	}

	// update status label
	try {
		get_label("home/update_status")
			.text("v" + _update_info.version + " ready to be installed")
			.events().action = [this]() {
			if (prompt("Would you like to apply the update now?")) {
				_restart_now = true;
				close();
			}
		};
		update();
	}
	catch (const std::exception&) {}

	// file integrity confirmed ... install update
	if (prompt("Version " + _update_info.version + " is ready to be installed.\nWould you like to apply the update now?")) {
		_restart_now = true;
		close();
	}
}

bool main_form::installed() {
	// check if application is installed
	std::string error;
	leccore::registry reg(leccore::registry::scope::current_user);
	if (!reg.do_read("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + _install_guid_32 + "_is1",
		"InstallLocation", _install_location_32, error)) {
	}
	if (!reg.do_read("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + _install_guid_64 + "_is1",
		"InstallLocation", _install_location_64, error)) {
	}

	_installed = !_install_location_32.empty() || !_install_location_64.empty();

	auto portable_file_exists = []()->bool {
		try {
			std::filesystem::path path(".portable");
			return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
		}
		catch (const std::exception&) {
			return false;
		}
	};

	if (_installed) {
		// check if app is running from the install location
		try {
			const auto current_path = std::filesystem::current_path().string() + "\\";

			if (current_path != _install_location_32 &&
				current_path != _install_location_64) {
				if (portable_file_exists()) {
					_real_portable_mode = true;
					_installed = false;	// run in portable mode
				}
			}
		}
		catch (const std::exception&) {}
	}
	else {
		if (portable_file_exists())
			_real_portable_mode = true;
	}

	return _installed;
}

void main_form::create_update_status() {
	if (_update_details_displayed)
		return;

	_update_details_displayed = true;

	try {
		auto& home = get_page("home");
		auto& pc_details_pane_specs = get_pane("home/pc_details_pane");

		// add update status label
		auto& update_status = lecui::widgets::label::add(home, "update_status");
		update_status
			.text("Checking for updates")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect().height(caption_height).width(pc_details_pane_specs.rect().width())
			.place(pc_details_pane_specs.rect(), 50.f, 100.f);

		// reduce height of pc details pane to accommodate the new update pane
		pc_details_pane_specs.rect().bottom() = update_status.rect().top() - _margin;

		// update the ui
		update();
	}
	catch (const std::exception&) {
		// this shouldn't happen, seriously ... but added nonetheless for correctness
	}
}

void main_form::close_update_status() {
	// set timer for closing the update status
	_timer_man.add("update_status_timer", 3000, [this]() { on_close_update_status(); });
}

void main_form::on_close_update_status() {
	// stop close update status timer
	_timer_man.stop("update_status_timer");

	try {
		auto& home = get_page("home");
		auto& update_status_specs = get_label("home/update_status");
		auto& pc_details_pane_specs = get_pane("home/pc_details_pane");

		// restore size of pc details pane
		pc_details_pane_specs.rect().bottom() = update_status_specs.rect().bottom();

		// close update status label
		_widget_man.close("home/update_status");
		update();
	}
	catch (const std::exception&) {}

	_update_details_displayed = false;
}

std::string main_form::pc_details_text() {
	std::string text;
	text += "-------------------------------------------------------------------------------\n";
	text += "PC DETAILS\n";
	text += "-------------------------------------------------------------------------------\n\n";
	text += "Name:\t\t\t\t";
	text += _pc_details.name + "\n";
	text += "Manufacturer:\t\t\t";
	text += _pc_details.manufacturer + "\n";
	text += "Model:\t\t\t\t";
	text += _pc_details.model + "\n";
	text += "System type:\t\t\t";
	text += _pc_details.system_type + "\n";
	text += "BIOS Serial Number:\t\t";
	text += _pc_details.bios_serial_number + "\n";
	text += "Motherboard Serial Number:\t";
	text += _pc_details.motherboard_serial_number + "\n";
	text += "\n";
	return text;
}

std::string main_form::power_details_text() {
	std::string text;
	text += "-------------------------------------------------------------------------------\n";
	text += "POWER DETAILS\n";
	text += "-------------------------------------------------------------------------------\n\n";
	text += "Status:\t\t\t\t";
	text += std::string(_power.ac ? "On AC" : "On Battery") + ", " + _pc_info.to_string(_power.status) + "\n";
	text += "Level:\t\t\t\t" + (_power.level != -1 ?
		(std::to_string(_power.level) + "% ") : "Unknown ") + "overall power level\n";
	text += "Time remaining:\t\t\t" + (_power.lifetime_remaining.empty() ? std::string() : (_power.lifetime_remaining + " remaining")) + "\n";

	int battery_number = 0;
	for (const auto& battery : _power.batteries) {
		text += "\nBattery " + std::to_string(battery_number);
		text += "\n-----------\n";

		text += "Name:\t\t\t\t";
		text += battery.name + "\n";
		text += "Manufacturer:\t\t\t";
		text += battery.manufacturer + "\n";
		text += "Battery Health:\t\t\t";
		text += leccore::round_off::to_string(battery.health, 0) + "%\n";
		text += "Designed Capacity:\t\t";
		text += (_setting_milliunits ?
			std::to_string(battery.designed_capacity) + "mWh" :
			leccore::round_off::to_string(battery.designed_capacity / 1000.f, 1) + "Wh") + "\n";
		text += "Fully Charged Capacity:\t\t";
		text += (_setting_milliunits ?
			std::to_string(battery.fully_charged_capacity) + "mWh" :
			leccore::round_off::to_string(battery.fully_charged_capacity / 1000.f, 1) + "Wh") + "\n";
		text += "Current Capacity:\t\t";
		text += (_setting_milliunits ?
			std::to_string(battery.current_capacity) + "mWh" :
			leccore::round_off::to_string(battery.current_capacity / 1000.f, 1) + "Wh") + "\n";
		text += "Charge Level:\t\t\t";
		text += (leccore::round_off::to_string(battery.level, 1) + "%") + "\n";
		text += "Current Voltage:\t\t";
		text += (battery.current_voltage == -1 ? "Unknown" :
			_setting_milliunits ?
			std::to_string(battery.current_voltage) + "mV" :
			leccore::round_off::to_string(battery.current_voltage / 1000.f, 2) + "V") + "\n";
		text += "Charge Rate:\t\t\t";
		text += (_setting_milliunits ?
			std::to_string(battery.current_charge_rate) + "mW" :
			leccore::round_off::to_string(battery.current_charge_rate / 1000.f, 1) + "W") + "\n";
		text += "Status:\t\t\t\t";
		text += _pc_info.to_string(battery.status) + "\n";

		battery_number++;
	}

	text += "\n";

	return text;
}

std::string main_form::cpu_details_text() {
	std::string text;
	text += "-------------------------------------------------------------------------------\n";
	text += "CPU DETAILS\n";
	text += "-------------------------------------------------------------------------------\n";

	int cpu_number = 0;
	for (const auto& cpu : _cpus) {
		text += "\nCPU " + std::to_string(cpu_number);
		text += "\n-----------\n";

		text += "Name:\t\t\t\t";
		text += cpu.name + "\n";
		text += "Status:\t\t\t\t";
		text += cpu.status + "\n";
		text += "Base Speed:\t\t\t";
		text += leccore::round_off::to_string(cpu.base_speed, 2) + "GHz" + "\n";
		text += "Cores:\t\t\t\t";
		text += (std::to_string(cpu.cores) +
			std::string(cpu.cores == 1 ? " core" : " cores") + ", " +
			std::to_string(cpu.logical_processors) +
			std::string(cpu.cores == 1 ? " logical processor" : " logical processors")) + "\n";

		cpu_number++;
	}

	text += "\n";

	return text;
}

std::string main_form::graphics_details_text() {
	std::string text;
	text += "-------------------------------------------------------------------------------\n";
	text += "GRAPHICS DETAILS\n";
	text += "-------------------------------------------------------------------------------\n";

	int gpu_number = 0;
	for (const auto& gpu : _gpus) {
		text += "\nGPU " + std::to_string(gpu_number);
		text += "\n-----------\n";

		text += "Name:\t\t\t\t";
		text += gpu.name + "\n";
		text += "Status:\t\t\t\t";
		text += gpu.status + "\n";
		text += "Dedicated Memory:\t\t";
		text += leccore::format_size(gpu.dedicated_vram) + "\n";
		text += "Total Available:\t\t";
		text += leccore::format_size(gpu.total_graphics_memory) + "\n";

		gpu_number++;
	}

	int monitor_number = 0;
	for (const auto& monitor : _monitors) {
		text += "\nMONITOR " + std::to_string(monitor_number);
		text += "\n-----------\n";

		// get highest supported mode
		leccore::pc_info::video_mode highest_mode = {};

		for (auto& mode : monitor.supported_modes) {
			if (highest_mode.horizontal_resolution < mode.horizontal_resolution)
				highest_mode = mode;
		}

		text += "Name:\t\t\t\t";
		text += (monitor.manufacturer + monitor.product_code_id) + "\n";
		text += "Size:\t\t\t\t";
		text += (leccore::round_off::to_string(highest_mode.physical_size, 1) +
			" inches") + "\n";
		text += "Max. Refresh:\t\t\t";
		text += (leccore::round_off::to_string(highest_mode.refresh_rate, 1) + " Hz") + "\n";
		text += "Max. Pixel Clock:\t\t";
		text += (leccore::round_off::to_string((double(highest_mode.pixel_clock_rate) / (1000.0 * 1000.0)), 1) + " MHz") + "\n";
		text += "Max. Screen Resolution:\t\t";
		text += (std::to_string(highest_mode.horizontal_resolution) + "x" + std::to_string(highest_mode.vertical_resolution) + " (" + highest_mode.resolution_name + ")") + "\n";

		monitor_number++;
	}

	text += "\n";

	return text;
}

std::string main_form::ram_details_text() {
	std::string text;
	text += "-------------------------------------------------------------------------------\n";
	text += "RAM DETAILS\n";
	text += "-------------------------------------------------------------------------------\n\n";
	text += "Total Capacity:\t\t\t";
	text += leccore::format_size(_ram.size) + "\n";
	text += "Speed:\t\t\t\t";
	text += std::to_string(_ram.speed) + "MHz" + "\n";

	int ram_number = 0;
	for (const auto& ram : _ram.ram_chips) {
		text += "\nRAM " + std::to_string(ram_number);
		text += "\n-----------\n";

		text += "Part Number:\t\t\t";
		text += ram.part_number + "\n";
		text += "Manufacturer:\t\t\t";
		text += ram.manufacturer + "\n";
		text += "Status:\t\t\t\t";
		text += ram.status + "\n";
		text += "Type:\t\t\t\t";
		text += ram.type + "\n";
		text += "Form Factor:\t\t\t";
		text += ram.form_factor + "\n";
		text += "Capacity:\t\t\t";
		text += leccore::format_size(ram.capacity) + "\n";
		text += "Speed:\t\t\t\t";
		text += std::to_string(ram.speed) + "MHz" + "\n";

		ram_number++;
	}

	text += "\n";

	return text;
}

std::string main_form::drive_details_text() {
	std::string text;
	text += "-------------------------------------------------------------------------------\n";
	text += "DRIVE DETAILS\n";
	text += "-------------------------------------------------------------------------------\n";

	int drive_number = 0;
	for (const auto& drive : _drives) {
		text += "\nDrive " + std::to_string(drive_number);
		text += "\n-----------\n";

		text += "Model:\t\t\t\t";
		text += drive.model + "\n";
		text += "Status:\t\t\t\t";
		text += drive.status + "\n";
		text += "Storage Type:\t\t\t";
		text += drive.storage_type + "\n";
		text += "Bus Type:\t\t\t";
		text += drive.bus_type + "\n";
		text += "Serial Number:\t\t\t";
		text += drive.serial_number + "\n";
		text += "Capacity:\t\t\t";
		text += leccore::format_size(drive.size) + "\n";
		text += "Media Type:\t\t\t";
		text += drive.media_type + "\n";

		drive_number++;
	}

	text += "\n";

	return text;
}

main_form::main_form(const std::string& caption, bool restarted) :
	_cleanup_mode(restarted ? false : leccore::commandline_arguments::contains("/cleanup")),
	_update_mode(restarted ? false : leccore::commandline_arguments::contains("/update")),
	_recent_update_mode(restarted ? false : leccore::commandline_arguments::contains("/recentupdate")),
	_system_tray_mode(restarted ? false : leccore::commandline_arguments::contains("/systemtray")),
	_settings(installed() ? _reg_settings.base() : _ini_settings.base()),
	form(caption) {
	_installed = installed();

	if (!_installed)
		// don't allow system tray mode when running in portable mode
		_system_tray_mode = false;

	_reg_settings.set_registry_path("Software\\com.github.alecmus\\" + std::string(appname));
	_ini_settings.set_ini_path("");	// use app folder for ini settings

	if (_cleanup_mode || _update_mode || _recent_update_mode)
		force_instance();
}

main_form::~main_form() {}
