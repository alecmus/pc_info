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

#include "../gui.h"
#include "../create_process.h"
#include <liblec/leccore/system.h>
#include <liblec/leccore/app_version_info.h>
#include <liblec/leccore/hash.h>
#include <liblec/leccore/file.h>
#include <liblec/leccore/system.h>
#include <liblec/leccore/zip.h>
#include <filesystem>

#ifdef _WIN64
#define architecture	"64bit"
#else
#define architecture	"32bit"
#endif

const float main_form::_margin = 10.f;
const float main_form::_title_font_size = 12.f;
const float main_form::_highlight_font_size = 14.f;
const float main_form::_detail_font_size = 10.f;
const float main_form::_caption_font_size = 8.f;
const std::string main_form::_sample_text = "<u><strong>Aq</strong></u>";
const std::string main_form::_font = "Segoe UI";
const lecui::color main_form::_caption_color{ 100, 100, 100, 255 };
const lecui::color main_form::_ok_color{ 0, 150, 0, 255 };
const lecui::color main_form::_not_ok_color{ 200, 0, 0, 255 };
const unsigned long main_form::_refresh_interval = 3000;

bool main_form::on_initialize(std::string& error) {
	if (!_cleanup_mode && !_update_mode && !_system_tray_mode) {
		// display splash screen
		if (get_dpi_scale() < 2.f)
			_splash.display(splash_image_128, false, error);
		else
			_splash.display(splash_image_256, false, error);
	}

	if (_cleanup_mode) {
		if (prompt("Would you like to delete the app settings?")) {
			// cleanup application settings
			if (!_settings.delete_recursive("", error))
				return false;

			if (_installed) {
				// cleanup company settings (will delete the company subkey if no
				// other apps have placed subkeys under it
				leccore::registry reg(leccore::registry::scope::current_user);
				if (!reg.do_delete("Software\\com.github.alecmus\\", error)) {}
			}
		}

		close();
		return true;
	}
	else {
		// check if there is an update ready to be installed
		std::string value, update_architecture;
		if (_settings.read_value("updates", "readytoinstall", value, error)) {}
		if (_settings.read_value("updates", "architecture", update_architecture, error)) {}

		// clear the registry entries
		if (!_settings.delete_value("updates", "readytoinstall", error)) {}
		if (!_settings.delete_value("updates", "architecture", error)) {}

		if (!value.empty()) {
			// check if update architecture matches this app's architecture
			const std::string app_architecture(architecture);

			if (app_architecture == update_architecture ||
				update_architecture.empty()	// failsafe
				) {
				try {
					const std::string fullpath(value);
					std::filesystem::path file_path(fullpath);

					const std::string directory = file_path.parent_path().string();
					const std::string filename = file_path.filename().string();

					// assume the zip file extracts to a directory with the same name
					std::string unzipped_folder;
					const auto idx = filename.find(".zip");

					if (idx != std::string::npos)
						unzipped_folder = directory + "\\" + filename.substr(0, idx);

					// unzip the file into the same directory as the zip file
					leccore::unzip unzip;
					unzip.start(fullpath, directory);

					while (unzip.unzipping()) {
						if (!keep_alive()) {
							// to-do: implement stopping mechanism
							//unzip.stop()
							close();
							return true;
						}
					}

					leccore::unzip::unzip_log log;
					if (unzip.result(log, error) && std::filesystem::exists(unzipped_folder)) {
						// get target directory
						std::string target_directory;

						if (_installed) {
#ifdef _WIN64
							target_directory = _install_location_64;
#else
							target_directory = _install_location_32;
#endif
						}
						else {
							if (_real_portable_mode) {
								try { target_directory = std::filesystem::current_path().string() + "\\"; }
								catch (const std::exception&) {}
							}
						}

						if (!target_directory.empty()) {
							if (_settings.write_value("updates", "rawfiles", unzipped_folder, error) &&
								_settings.write_value("updates", "target", target_directory, error)) {
								if (_real_portable_mode) {
									try {
										// copy the .config file to the unzipped folder
										std::filesystem::path p("pc_info.ini");
										const std::string dest_file = unzipped_folder + "\\" + p.filename().string();
										std::filesystem::copy_file(p, dest_file, std::filesystem::copy_options::overwrite_existing);
									}
									catch (const std::exception&) {}
								}

								// run downloaded app from the unzipped folder
#ifdef _WIN64
								const std::string new_exe_fullpath = unzipped_folder + "\\pc_info64.exe";
#else
								const std::string new_exe_fullpath = unzipped_folder + "\\pc_info32.exe";
#endif
								if (create_process(new_exe_fullpath, { "/update" }, error)) {
									close();
									return true;
								}
							}
						}

						// continue app execution normally
					}
					else {
						// delete the update folder ... there many be something wrong with the update file
						if (!leccore::file::remove_directory(directory, error)) {}

						// continue app execution normally
					}
				}
				catch (const std::exception&) {
					// continue app execution normally
				}
			}
			else {
				// system architecture did not match ... continue app execution normally
			}
		}
		else
			if (_update_mode) {
				// get the location of the raw files
				if (_settings.read_value("updates", "rawfiles", value, error) && !value.empty()) {
					const std::string raw_files_directory(value);

					if (_settings.read_value("updates", "target", value, error)) {
						std::string target(value);
						if (!target.empty()) {
							try {
								// overrwrite the files in target with the files in raw_files_directory
								for (const auto& path : std::filesystem::directory_iterator(raw_files_directory)) {
									std::filesystem::path p(path);
									const std::string dest_file = target + p.filename().string();
									std::filesystem::copy_file(path, dest_file, std::filesystem::copy_options::overwrite_existing);
								}

								// files copied successfully, now execute the app in the target directory
#ifdef _WIN64
								const std::string updated_exe_fullpath = target + "\\pc_info64.exe";
#else
								const std::string updated_exe_fullpath = target + "\\pc_info32.exe";
#endif
								if (create_process(updated_exe_fullpath, { "/recentupdate" }, error)) {}

								close();
								return true;
							}
							catch (const std::exception& e) {
								error = e.what();

								// exit
								close();
								return true;
							}
						}
					}
				}
				else {
					close();
					return true;
				}
			}
			else
				if (_recent_update_mode) {
					// check if the updates_rawfiles and updates_target settings are set, and eliminated them if so then notify user of successful update
					std::string updates_rawfiles;
					if (!_settings.read_value("updates", "rawfiles", updates_rawfiles, error) && !value.empty()) {}

					std::string updates_target;
					if (!_settings.read_value("updates", "target", updates_target, error)) {}

					if (!updates_rawfiles.empty() || !updates_target.empty()) {
						if (!_settings.delete_value("updates", "rawfiles", error)) {}
						if (!_settings.delete_value("updates", "target", error)) {}

						if (_installed) {
							// update inno setup version number
							leccore::registry reg(leccore::registry::scope::current_user);
#ifdef _WIN64
							if (!reg.do_write("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + _install_guid_64 + "_is1",
								"DisplayVersion", std::string(appversion), error)) {
							}
#else
							if (!reg.do_write("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + _install_guid_32 + "_is1",
								"DisplayVersion", std::string(appversion), error)) {
							}
#endif
						}

						// to-do: use a timer instead for all the calls below, for better user experience
						message("App updated successfully to version " + std::string(appversion));

						std::string updates_tempdirectory;
						if (!_settings.read_value("updates", "tempdirectory", updates_tempdirectory, error)) {}
						else {
							// delete updates temp directory
							if (!leccore::file::remove_directory(updates_tempdirectory, error)) {}
						}
						if (!_settings.delete_value("updates", "tempdirectory", error)) {}
					}
				}
	}

	// read application settings
	std::string value;
	if (!_settings.read_value("", "darktheme", value, error))
		return false;
	else
		// default to "off"
		_setting_darktheme = value == "on";

	if (!_settings.read_value("", "milliunits", value, error))
		return false;
	else
		// default to "yes"
		_setting_milliunits = value != "no";

	if (!_settings.read_value("updates", "autocheck", value, error))
		return false;
	else {
		// default to yes
		_setting_autocheck_updates = value != "no";

		if (_setting_autocheck_updates) {
			if (!_settings.read_value("updates", "did_run_once", value, error))
				return false;
			else {
				if (value != "yes") {
					// do nothing ... for better first time impression
					if (!_settings.write_value("updates", "did_run_once", "yes", error)) {}
				}
				else {
					// schedule checking for updates (5 minutes if in system tray mode, and 5 seconds otherwise)
					_timer_man.add("start_update_check", _system_tray_mode ? 5 * 60 * 1000 : 5 * 1000, [this]() {
						// stop the start update check timer
						_timer_man.stop("start_update_check");

						// create update status
						create_update_status();

						// start checking for updates
						_check_update.start();

						// start timer to keep progress of the update check (every 1.5 seconds)
						_timer_man.add("update_check", 1500, [&]() { on_update_check(); });
						});
				}
			}
		}
	}

	if (!_settings.read_value("updates", "autodownload", value, error))
		return false;
	else
		// default to yes
		_setting_autodownload_updates = value != "no";

	if (!_settings.read_value("", "autostart", value, error))
		return false;
	else
		// default to no
		_setting_autostart = value == "yes";

	if (_setting_autostart) {
		std::string command;
#ifdef _WIN64
		command = "\"" + _install_location_64 + "pc_info64.exe\"";
#else
		command = "\"" + _install_location_32 + "pc_info32.exe\"";
#endif
		command += " /systemtray";

		leccore::registry reg(leccore::registry::scope::current_user);
		if (!reg.do_write("Software\\Microsoft\\Windows\\CurrentVersion\\Run", "pc_info", command, error)) {}
	}
	else {
		leccore::registry reg(leccore::registry::scope::current_user);
		if (!reg.do_delete("Software\\Microsoft\\Windows\\CurrentVersion\\Run", "pc_info", error)) {}
	}

	// size and stuff
	_ctrls
		.allow_resize(false)
		.start_hidden(_system_tray_mode);
	_apprnc
		.set_icons(ico_resource, ico_resource)
		.theme(_setting_darktheme ? lecui::themes::dark : lecui::themes::light);
	_dim.set_size({ 1120.f, 570.f });

	// add form caption handler
	form::on_caption([this]() { about(); }, "View info about this app");

	// add form menu
	_form_menu.add("• • •", "Settings and more", {
		{ "Settings", [this]() { settings(); } },
		{ "Updates", [this]() { updates(); } },
		{ "About", [this]() { about(); } },
		{ "" },
		{ "Exit", [this]() { close(); } }
		}, error);

	// read pc, power, cpu, gpu, memory and drive info
	_pc_info.pc(_pc_details, error);
	_pc_info.power(_power, error);
	_pc_info.cpu(_cpus, error);
	_pc_info.gpu(_gpus, error);
	_pc_info.ram(_ram, error);
	_pc_info.drives(_drives, error);

	return true;
}

void main_form::updates() {
	if (_check_update.checking() || _timer_man.running("update_check"))
		return;

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

void main_form::on_start() {
	start_refresh_timer();

	if (_installed) {
		std::string error;
		if (!_tray_icon.add(ico_resource, std::string(appname) + " " +
			std::string(appversion) + " (" + std::string(architecture) + ")",
			{
			{ "<strong>Show PC Info</strong>", [this]() { restore(); } },
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

bool main_form::on_layout(std::string& error) {
	// add home page
	auto& home = _page_man.add("home");

	// compute label heights
	const lecui::rect page_rect = { 0.f, home.size().width, 0.f, home.size().height };
	title_height = _dim.measure_label(_sample_text, _font, _title_font_size, true, false, page_rect).height();
	highlight_height = _dim.measure_label(_sample_text, _font, _highlight_font_size, true, false, page_rect).height();
	detail_height = _dim.measure_label(_sample_text, _font, _detail_font_size, true, false, page_rect).height();
	caption_height = _dim.measure_label(_sample_text, _font, _caption_font_size, true, false, page_rect).height();

	//////////////////////////////////////////////////
	// 1. Add pane for pc details
	lecui::containers::pane_builder pc_details_pane(home, "pc_details_pane");
	pc_details_pane().rect({ _margin, _margin + 200.f, _margin, home.size().height - _margin });

	// add pc details title
	lecui::widgets::label_builder pc_details_title(pc_details_pane.get());
	pc_details_title()
		.rect({ 0.f, pc_details_pane.get().size().width, 0.f, title_height })
		.font_size(_title_font_size)
		.text("<strong>PC DETAILS</strong>");

	// add pc name
	lecui::widgets::label_builder pc_name_caption(pc_details_pane.get());
	pc_name_caption()
		.text("Name")
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(pc_details_title().rect())
		.rect().height(caption_height).snap_to(pc_details_title().rect(), snap_type::bottom, _margin);

	lecui::widgets::label_builder pc_name(pc_details_pane.get());
	pc_name()
		.text(_pc_details.name)
		.font_size(_highlight_font_size)
		.rect(pc_details_title().rect())
		.rect().height(highlight_height).snap_to(pc_name_caption().rect(), snap_type::bottom, 0.f);

	// add pc manufacturer
	lecui::widgets::label_builder manufacturer_caption(pc_details_pane.get());
	manufacturer_caption()
		.text("Manufacturer")
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(pc_name_caption().rect())
		.rect().snap_to(pc_name().rect(), snap_type::bottom, _margin);

	lecui::widgets::label_builder manufacturer(pc_details_pane.get());
	manufacturer()
		.text(_pc_details.manufacturer)
		.font_size(_detail_font_size)
		.rect(manufacturer_caption().rect())
		.rect().height(detail_height).snap_to(manufacturer_caption().rect(), snap_type::bottom, 0.f);

	// add model
	lecui::widgets::label_builder model_caption(pc_details_pane.get());
	model_caption()
		.text("Model")
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(pc_name_caption().rect())
		.rect().snap_to(manufacturer().rect(), snap_type::bottom, _margin);

	lecui::widgets::label_builder model(pc_details_pane.get());
	model()
		.text(_pc_details.model)
		.font_size(_detail_font_size)
		.rect(manufacturer().rect())
		.rect().snap_to(model_caption().rect(), snap_type::bottom, 0.f);

	// add system type
	lecui::widgets::label_builder type_caption(pc_details_pane.get());
	type_caption()
		.text("System type")
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(pc_name_caption().rect())
		.rect().snap_to(model().rect(), snap_type::bottom, _margin);

	lecui::widgets::label_builder type(pc_details_pane.get());
	type()
		.text(_pc_details.system_type)
		.font_size(_detail_font_size)
		.rect(model().rect())
		.rect().snap_to(type_caption().rect(), snap_type::bottom, 0.f);

	// add bios serial number
	lecui::widgets::label_builder bios_sn_caption(pc_details_pane.get());
	bios_sn_caption()
		.text("BIOS Serial Number")
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(pc_name_caption().rect())
		.rect().snap_to(type().rect(), snap_type::bottom, _margin);

	lecui::widgets::label_builder bios_sn(pc_details_pane.get());
	bios_sn()
		.text(_pc_details.bios_serial_number)
		.font_size(_detail_font_size)
		.rect(type().rect())
		.rect().snap_to(bios_sn_caption().rect(), snap_type::bottom, 0.f);

	// add board serial number
	lecui::widgets::label_builder board_sn_caption(pc_details_pane.get());
	board_sn_caption()
		.text("Motherboard Serial Number")
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(pc_name_caption().rect())
		.rect().snap_to(bios_sn().rect(), snap_type::bottom, _margin);
	

	lecui::widgets::label_builder board_sn(pc_details_pane.get());
	board_sn()
		.text(_pc_details.motherboard_serial_number)
		.font_size(_detail_font_size)
		.rect(bios_sn().rect())
		.rect().snap_to(board_sn_caption().rect(), snap_type::bottom, 0.f);

	// add hardware summary details
	if (true) {
		lecui::widgets::label_builder cpu_summary(pc_details_pane.get());
		cpu_summary()
			.text(std::to_string(_cpus.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(_cpus.size() == 1 ? " CPU" : " CPUs") + "</span>")
			.font_size(_highlight_font_size)
			.rect(pc_name().rect())
			.rect().height(highlight_height).snap_to(board_sn().rect(), snap_type::bottom, 2 * _margin);
		
		lecui::widgets::label_builder gpu_summary(pc_details_pane.get());
		gpu_summary()
			.text(std::to_string(_gpus.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(_gpus.size() == 1 ? " GPU" : " GPUs") + "</span>")
			.font_size(_highlight_font_size)
			.rect(pc_name().rect())
			.rect().height(highlight_height).snap_to(cpu_summary().rect(), snap_type::bottom, 0.f);

		lecui::widgets::label_builder ram_summary(pc_details_pane.get());
		ram_summary()
			.text(std::to_string(_ram.ram_chips.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(_ram.ram_chips.size() == 1 ?
				" RAM chip" : " RAM chips") + "</span>")
			.font_size(_highlight_font_size)
			.rect(pc_name().rect())
			.rect().height(highlight_height).snap_to(gpu_summary().rect(), snap_type::bottom, 0.f);

		lecui::widgets::label_builder drive_summary(pc_details_pane.get(), "drive_summary");
		drive_summary()
			.text(std::to_string(_drives.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(_drives.size() == 1 ?
				" drive" : " drives") + "</span>")
			.font_size(_highlight_font_size)
			.rect(pc_name().rect())
			.rect().height(highlight_height).snap_to(ram_summary().rect(), snap_type::bottom, 0.f);

		lecui::widgets::label_builder battery_summary(pc_details_pane.get(), "battery_summary");
		battery_summary()
			.text(std::to_string(_power.batteries.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(_power.batteries.size() == 1 ?
				" battery" : " batteries") + "</span>")
			.font_size(_highlight_font_size)
			.rect(pc_name().rect())
			.rect().height(highlight_height).snap_to(drive_summary().rect(), snap_type::bottom, 0.f);
	}

	//////////////////////////////////////////////////
	// 2. Add pane for power details
	lecui::containers::pane_builder power_pane(home, "power_pane");
	power_pane()
		.rect(pc_details_pane().rect())
		.rect().width(270.f);
	power_pane().rect().snap_to(pc_details_pane().rect(), snap_type::right_top, _margin);

	// add pc details title
	lecui::widgets::label_builder power_details_title(power_pane.get());
	power_details_title()
		.text("<strong>POWER DETAILS</strong>")
		.font_size(_title_font_size)
		.rect({ 0.f, power_pane.get().size().width, 0.f, title_height });

	// add power status
	lecui::widgets::label_builder power_status_caption(power_pane.get());
	power_status_caption()
		.text("Status")
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(power_details_title().rect())
		.rect().height(caption_height).snap_to(power_details_title().rect(), snap_type::bottom, _margin);

	lecui::widgets::label_builder power_status(power_pane.get(), "power_status");
	power_status()
		.text(std::string(_power.ac ? "On AC" : "On Battery") + (", <span style = 'font-size: 8.0pt;'>" + _pc_info.to_string(_power.status) + "</span>"))
		.font_size(_highlight_font_size)
		.rect(power_details_title().rect())
		.rect().height(highlight_height).snap_to(power_status_caption().rect(), snap_type::bottom, 0.f);

	// add power level
	lecui::widgets::label_builder level(power_pane.get(), "level");
	level().text((_power.level != -1 ?
		(std::to_string(_power.level) + "% ") : std::string("<em>Unknown</em> ")) + "<span style = 'font-size: 8.0pt;'>overall power level</span>")
		.font_size(_detail_font_size)
		.rect(power_status_caption().rect())
		.rect().height(detail_height).snap_to(power_status().rect(), snap_type::bottom, _margin);

	lecui::widgets::progress_bar_builder level_bar(power_pane.get(), "level_bar");
	level_bar()
		.percentage(static_cast<float>(_power.level))
		.rect().width(power_status().rect().width()).snap_to(level().rect(), snap_type::bottom, _margin / 2.f);

	// add life remaining label
	lecui::widgets::label_builder life_remaining(power_pane.get(), "life_remaining");
	life_remaining()
		.text(_power.lifetime_remaining.empty() ? std::string() : (_power.lifetime_remaining + " remaining"))
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(level_bar().rect())
		.rect().height(caption_height).snap_to(level_bar().rect(), snap_type::bottom, _margin / 2.f);

	add_battery_pane(power_pane.get(), life_remaining().rect().bottom());

	//////////////////////////////////////////////////
	// 2. Add pane for cpu details
	lecui::containers::pane_builder cpu_pane(home);
	cpu_pane().rect()
		.left(power_pane().rect().right() + _margin).width(300.f)
		.top(_margin).height(240.f);

	// add cpu title
	lecui::widgets::label_builder cpu_title(cpu_pane.get());
	cpu_title()
		.text("<strong>CPU DETAILS</strong>")
		.font_size(_title_font_size)
		.rect({ 0.f, cpu_pane.get().size().width, 0.f, title_height });

	lecui::containers::tab_pane_builder cpu_tab_pane(cpu_pane.get());
	cpu_tab_pane().tab_side(lecui::containers::tab_pane_specs::side::top);
	cpu_tab_pane().rect()
		.left(0.f)
		.right(cpu_pane.get().size().width)
		.top(cpu_title().rect().bottom())
		.bottom(cpu_pane.get().size().height);
	cpu_tab_pane().color_tabs().alpha(0);
	cpu_tab_pane().color_tabs_border().alpha(0);

	// add as many tab panes as there are cpus
	int cpu_number = 0;
	for (const auto& cpu : _cpus) {
		lecui::containers::tab_builder cpu_pane(cpu_tab_pane, "CPU " + std::to_string(cpu_number));

		// add cpu name
		lecui::widgets::label_builder cpu_name_caption(cpu_pane.get());
		cpu_name_caption()
			.text("Name")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect({ 0.f, cpu_pane.get().size().width, 0.f, caption_height });

		lecui::widgets::label_builder cpu_name(cpu_pane.get());
		cpu_name()
			.text(cpu.name)
			.font_size(_detail_font_size)
			.rect(cpu_name_caption().rect())
			.rect().height(detail_height).snap_to(cpu_name_caption().rect(), snap_type::bottom, 0.f);

		// add cpu status
		lecui::widgets::label_builder status_caption(cpu_pane.get());
		status_caption()
			.text("Status")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(cpu_name_caption().rect())
			.rect().snap_to(cpu_name().rect(), snap_type::bottom, _margin);

		lecui::widgets::label_builder status(cpu_pane.get());
		status()
			.text(cpu.status)
			.font_size(_detail_font_size)
			.rect(status_caption().rect())
			.rect().height(detail_height).snap_to(status_caption().rect(), snap_type::bottom, 0.f);
		if (cpu.status == "OK")
			status().color_text(_ok_color);

		// add base speed
		lecui::widgets::label_builder base_speed(cpu_pane.get());
		base_speed()
			.text(leccore::round_off::to_string(cpu.base_speed, 2) + "GHz <span style = 'font-size: 8.0pt;'>base speed</span>")
			.font_size(_highlight_font_size)
			.rect(status().rect())
			.rect().height(highlight_height).snap_to(status().rect(), snap_type::bottom, _margin);

		// add cpu cores
		lecui::widgets::label_builder cores(cpu_pane.get());
		cores()
			.font_size(_highlight_font_size)
			.text(std::to_string(cpu.cores) +
				"<span style = 'font-size: 8.0pt;'>" +
				std::string(cpu.cores == 1 ? " core" : " cores") + "</span>, " +
				std::to_string(cpu.logical_processors) +
				"<span style = 'font-size: 8.0pt;'>" +
				std::string(cpu.cores == 1 ? " logical processor" : " logical processors") + "</span>")
			.rect(base_speed().rect())
			.rect().height(highlight_height).snap_to(base_speed().rect(), snap_type::bottom, _margin);

		cpu_number++;
	}

	cpu_tab_pane.select("CPU 0");

	//////////////////////////////////////////////////
	// 3. Add pane for gpu details
	lecui::containers::pane_builder gpu_pane(home);
	gpu_pane().rect()
		.left(cpu_pane().rect().right() + _margin).width(300.f)
		.top(_margin).height(240.f);

	// add gpu title
	lecui::widgets::label_builder gpu_title(gpu_pane.get());
	gpu_title()
		.text("<strong>GPU DETAILS</strong>")
		.font_size(_title_font_size)
		.rect({ 0.f, gpu_pane.get().size().width, 0.f, title_height });

	lecui::containers::tab_pane_builder gpu_tab_pane(gpu_pane.get());
	gpu_tab_pane().tab_side(lecui::containers::tab_pane_specs::side::top);
	gpu_tab_pane().rect()
		.left(0.f)
		.right(gpu_pane.get().size().width)
		.top(gpu_title().rect().bottom())
		.bottom(gpu_pane.get().size().height);
	gpu_tab_pane().color_tabs().alpha(0);
	gpu_tab_pane().color_tabs_border().alpha(0);

	// add as many tab panes as there are gpus
	int gpu_number = 0;
	for (const auto& gpu : _gpus) {
		lecui::containers::tab_builder gpu_pane(gpu_tab_pane,
			"GPU " + std::to_string(gpu_number));

		// add gpu name
		lecui::widgets::label_builder gpu_name_caption(gpu_pane.get());
		gpu_name_caption()
			.text("Name")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect({ 0.f, gpu_pane.get().size().width, 0.f, caption_height });

		lecui::widgets::label_builder gpu_name(gpu_pane.get());
		gpu_name()
			.text(gpu.name)
			.font_size(_detail_font_size)
			.rect(gpu_name_caption().rect())
			.rect().height(detail_height).snap_to(gpu_name_caption().rect(),
				snap_type::bottom, 0.f);

		// add gpu status
		lecui::widgets::label_builder status_caption(gpu_pane.get());
		status_caption()
			.text("Status")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(gpu_name_caption().rect())
			.rect().width(gpu_pane.get().size().width / 4.f).snap_to(gpu_name().rect(), snap_type::bottom_left, _margin);

		lecui::widgets::label_builder status(gpu_pane.get());
		status()
			.font_size(_detail_font_size)
			.rect(status_caption().rect())
			.rect().height(detail_height).snap_to(status_caption().rect(), snap_type::bottom, 0.f);
		status().text() = gpu.status;
		if (gpu.status == "OK")
			status().color_text(_ok_color);

		// add gpu resolution
		lecui::widgets::label_builder resolution_caption(gpu_pane.get());
		resolution_caption()
			.text("Resolution")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(status_caption().rect())
			.rect().right(gpu_name_caption().rect().right()).snap_to(status_caption().rect(), snap_type::right, _margin);

		lecui::widgets::label_builder resolution(gpu_pane.get());
		resolution()
			.text(std::to_string(gpu.horizontal_resolution) + "x" + std::to_string(gpu.vertical_resolution) + " (" + gpu.resolution_name + ")")
			.font_size(_detail_font_size)
			.rect(resolution_caption().rect())
			.rect().height(detail_height).snap_to(resolution_caption().rect(), snap_type::bottom, 0.f);

		// add dedicated video memory
		lecui::widgets::label_builder dedicated_ram(gpu_pane.get());
		dedicated_ram()
			.text(leccore::format_size(gpu.dedicated_vram) + "<span style = 'font-size: 8.0pt;'> dedicated video memory</span>")
			.font_size(_highlight_font_size)
			.rect(status().rect())
			.rect().width(gpu_name_caption().rect().width()).height(highlight_height).snap_to(status().rect(), snap_type::bottom_left, _margin);

		// add refresh rate and memory
		lecui::widgets::label_builder additional(gpu_pane.get());
		additional().text(std::to_string(gpu.refresh_rate) + "Hz " +
			"<span style = 'font-size: 8.0pt;'>refresh rate</span>, " +
			leccore::format_size(gpu.total_graphics_memory) + " " +
			"<span style = 'font-size: 8.0pt;'>graphics memory</span>")
			.font_size(_highlight_font_size)
			.rect(status().rect())
			.rect().width(dedicated_ram().rect().width()).height(highlight_height).snap_to(dedicated_ram().rect(), snap_type::bottom_left, _margin);

		gpu_number++;
	}

	gpu_tab_pane.select("GPU 0");

	//////////////////////////////////////////////////
	// 4. Add pane for ram details
	lecui::containers::pane_builder ram_pane(home);
	ram_pane().rect()
		.left(cpu_pane().rect().left())
		.width(300.f)
		.top(cpu_pane().rect().bottom() + _margin)
		.height(270.f);

	// add ram title
	lecui::widgets::label_builder ram_title(ram_pane.get());
	ram_title()
		.text("<strong>RAM DETAILS</strong>")
		.font_size(_title_font_size)
		.rect({ 0.f, ram_pane.get().size().width, 0.f, title_height });

	// add ram summary
	lecui::widgets::label_builder ram_summary(ram_pane.get());
	ram_summary().text(leccore::format_size(_ram.size) + " " +
		"<span style = 'font-size: 8.0pt;'>capacity</span>, " +
		std::to_string(_ram.speed) + "MHz " +
		"<span style = 'font-size: 8.0pt;'>speed</span>")
		.font_size(_highlight_font_size)
		.rect(ram_title().rect())
		.rect().snap_to(ram_title().rect(), snap_type::bottom, 0.f).height(highlight_height);

	lecui::containers::tab_pane_builder ram_tab_pane(ram_pane.get());
	ram_tab_pane().tab_side(lecui::containers::tab_pane_specs::side::top);
	ram_tab_pane().rect()
		.left(0.f)
		.right(ram_pane.get().size().width)
		.top(ram_summary().rect().bottom())
		.bottom(ram_pane.get().size().height);
	ram_tab_pane().color_tabs().alpha(0);
	ram_tab_pane().color_tabs_border().alpha(0);

	// add as many tab panes as there are rams
	int ram_number = 0;
	for (const auto& ram : _ram.ram_chips) {
		lecui::containers::tab_builder ram_pane(ram_tab_pane, "RAM " + std::to_string(ram_number));

		// add ram part number
		lecui::widgets::label_builder ram_part_number_caption(ram_pane.get());
		ram_part_number_caption()
			.text("Part Number")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect({ 0.f, ram_pane.get().size().width, 0.f, caption_height });

		lecui::widgets::label_builder ram_part_number(ram_pane.get());
		ram_part_number()
			.text(ram.part_number)
			.font_size(_detail_font_size)
			.rect(ram_part_number_caption().rect())
			.rect().height(detail_height).snap_to(ram_part_number_caption().rect(), snap_type::bottom, 0.f);

		// add ram manufacturer
		lecui::widgets::label_builder manufacturer_caption(ram_pane.get());
		manufacturer_caption()
			.text("Manufacturer")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(ram_part_number_caption().rect())
			.rect().width(ram_pane.get().size().width).snap_to(ram_part_number().rect(), snap_type::bottom, _margin);

		lecui::widgets::label_builder manufacturer(ram_pane.get());
		manufacturer()
			.text(ram.manufacturer)
			.font_size(_detail_font_size)
			.rect(manufacturer_caption().rect())
			.rect().height(detail_height).snap_to(manufacturer_caption().rect(), snap_type::bottom, 0.f);

		// add ram type
		lecui::widgets::label_builder type_caption(ram_pane.get());
		type_caption()
			.text("Type")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(ram_part_number_caption().rect())
			.rect().width(ram_pane.get().size().width / 2.f).snap_to(manufacturer().rect(), snap_type::bottom_left, _margin);

		lecui::widgets::label_builder type(ram_pane.get());
		type()
			.text(ram.type)
			.font_size(_detail_font_size)
			.rect(type_caption().rect())
			.rect().height(detail_height)
			.snap_to(type_caption().rect(), snap_type::bottom, 0.f);

		// add ram form factor
		lecui::widgets::label_builder form_factor_caption(ram_pane.get());
		form_factor_caption()
			.text("Form Factor")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(type_caption().rect())
			.rect().snap_to(type_caption().rect(), snap_type::right, 0.f);

		lecui::widgets::label_builder form_factor(ram_pane.get());
		form_factor()
			.text(ram.form_factor)
			.font_size(_detail_font_size)
			.rect(form_factor_caption().rect())
			.rect().height(detail_height).snap_to(form_factor_caption().rect(), snap_type::bottom, 0.f);

		// add capacity and speed
		lecui::widgets::label_builder additional(ram_pane.get());
		additional()
			.text(leccore::format_size(ram.capacity) + " " +
				"<span style = 'font-size: 8.0pt;'>capacity</span>, " +
				std::to_string(ram.speed) + "MHz " +
				"<span style = 'font-size: 8.0pt;'>speed</span>")
			.font_size(_highlight_font_size)
			.rect(ram_part_number().rect())
			.rect().height(highlight_height).snap_to(type().rect(), snap_type::bottom, _margin);

		ram_number++;
	}

	ram_tab_pane.select("RAM 0");

	//////////////////////////////////////////////////
	// 5. Add pane for drive details
	lecui::containers::pane_builder drive_pane(home, "drive_pane");
	drive_pane()
		.rect(ram_pane().rect())
		.rect().snap_to(ram_pane().rect(), snap_type::right, _margin);

	// add drive title
	lecui::widgets::label_builder drive_title(drive_pane.get(), "drive_title");
	drive_title().text("<strong>DRIVE DETAILS</strong>")
		.font_size(_title_font_size)
		.rect({ 0.f, drive_pane.get().size().width, 0.f, title_height });

	// add pane for drive details
	add_drive_details_pane(drive_pane.get(), drive_title().rect().bottom());

	_page_man.show("home");
	return true;
}

void main_form::add_battery_pane(lecui::containers::page& power_pane, const float top) {
	// add pane for battery details
	lecui::containers::tab_pane_builder battery_tab_pane(power_pane, "battery_tab_pane");
	battery_tab_pane()
		.tab_side(lecui::containers::tab_pane_specs::side::top)
		.rect({ 0.f, power_pane.size().width, top, power_pane.size().height });
	battery_tab_pane().color_tabs().alpha(0);
	battery_tab_pane().color_tabs_border().alpha(0);

	// add as many tab panes as there are batteries
	int battery_number = 0;
	for (const auto& battery : _power.batteries) {
		lecui::containers::tab_builder battery_pane(battery_tab_pane, "Battery " + std::to_string(battery_number));

		// add battery name
		lecui::widgets::label_builder battery_name_caption(battery_pane.get());
		battery_name_caption()
			.text("Name")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect({ 0.f, battery_pane.get().size().width, 0.f, caption_height });

		lecui::widgets::label_builder battery_name(battery_pane.get());
		battery_name()
			.text(battery.name)
			.font_size(_detail_font_size)
			.rect(battery_name_caption().rect())
			.rect().height(detail_height).snap_to(battery_name_caption().rect(), snap_type::bottom, 0.f);

		// add battery manufacturer
		lecui::widgets::label_builder manufacturer_caption(battery_pane.get());
		manufacturer_caption()
			.text("Manufacturer")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(battery_name_caption().rect())
			.rect().width(battery_pane.get().size().width).snap_to(battery_name().rect(), snap_type::bottom, _margin);

		lecui::widgets::label_builder manufacturer(battery_pane.get());
		manufacturer()
			.text(battery.manufacturer)
			.font_size(_detail_font_size)
			.rect(manufacturer_caption().rect())
			.rect().height(detail_height).snap_to(manufacturer_caption().rect(), snap_type::bottom, 0.f);

		// add battery designed capacity
		lecui::widgets::label_builder designed_capacity_caption(battery_pane.get());
		designed_capacity_caption()
			.text("Designed Capacity")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(manufacturer_caption().rect())
			.rect().width(battery_pane.get().size().width / 2.f).snap_to(manufacturer().rect(), snap_type::bottom_left, _margin);

		lecui::widgets::label_builder designed_capacity(battery_pane.get(), "designed_capacity");
		designed_capacity().text(_setting_milliunits ?
			std::to_string(battery.designed_capacity) + "mWh" :
			leccore::round_off::to_string(battery.designed_capacity / 1000.f, 1) + "Wh")
			.font_size(_detail_font_size)
			.rect(designed_capacity_caption().rect())
			.rect().height(detail_height).snap_to(designed_capacity_caption().rect(), snap_type::bottom, 0.f);

		// add battery fully charged capacity
		lecui::widgets::label_builder fully_charged_capacity_caption(battery_pane.get());
		fully_charged_capacity_caption()
			.text("Fully Charged Capacity")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(designed_capacity_caption().rect())
			.rect().snap_to(designed_capacity_caption().rect(), snap_type::right, 0.f);

		lecui::widgets::label_builder fully_charged_capacity(battery_pane.get(), "fully_charged_capacity");
		fully_charged_capacity()
			.text(_setting_milliunits ?
				std::to_string(battery.fully_charged_capacity) + "mWh" :
				leccore::round_off::to_string(battery.fully_charged_capacity / 1000.f, 1) + "Wh")
			.font_size(_detail_font_size)
			.rect(fully_charged_capacity_caption().rect())
			.rect().height(detail_height).snap_to(fully_charged_capacity_caption().rect(), snap_type::bottom, 0.f);

		// add battery health
		lecui::widgets::progress_indicator_builder health(battery_pane.get(), "health");
		health()
			.percentage(static_cast<float>(battery.health))
			.rect().snap_to(designed_capacity().rect(), snap_type::bottom_left, _margin);

		lecui::widgets::label_builder health_caption(battery_pane.get());
		health_caption().text("BATTERY HEALTH").center_v(true).color_text(_caption_color).font_size(_caption_font_size)
			.rect(health().rect())
			.rect().right(battery_name().rect().right()).snap_to(health().rect(), snap_type::right, _margin);

		// add battery current capacity
		lecui::widgets::label_builder current_capacity_caption(battery_pane.get());
		current_capacity_caption()
			.text("Current Capacity")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect().width(battery_pane.get().size().width / 2.f).snap_to(health().rect(), snap_type::bottom_left, _margin);

		lecui::widgets::label_builder current_capacity(battery_pane.get(), "current_capacity");
		current_capacity()
			.text(_setting_milliunits ?
				std::to_string(battery.current_capacity) + "mWh" :
				leccore::round_off::to_string(battery.current_capacity / 1000.f, 1) + "Wh")
			.font_size(_detail_font_size)
			.rect(current_capacity_caption().rect())
			.rect().height(detail_height).snap_to(current_capacity_caption().rect(), snap_type::bottom, 0.f);

		// add battery fully charged capacity
		lecui::widgets::label_builder charge_level_caption(battery_pane.get());
		charge_level_caption()
			.text("Charge Level")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(current_capacity_caption().rect())
			.rect().snap_to(current_capacity_caption().rect(), snap_type::right, 0.f);

		lecui::widgets::label_builder charge_level(battery_pane.get(), "charge_level");
		charge_level()
			.text(leccore::round_off::to_string(battery.level, 1) + "%")
			.font_size(_detail_font_size)
			.rect(charge_level_caption().rect())
			.rect().height(detail_height).snap_to(charge_level_caption().rect(), snap_type::bottom, 0.f);

		// add battery current voltage
		lecui::widgets::label_builder current_voltage_caption(battery_pane.get());
		current_voltage_caption()
			.text("Current Voltage")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(battery_name().rect())
			.rect().width(battery_pane.get().size().width / 2.f).snap_to(current_capacity().rect(), snap_type::bottom_left, _margin);

		lecui::widgets::label_builder current_voltage(battery_pane.get(), "current_voltage");
		current_voltage()
			.text(_setting_milliunits ?
				std::to_string(battery.current_voltage) + "mV" :
				leccore::round_off::to_string(battery.current_voltage / 1000.f, 2) + "V")
			.font_size(_detail_font_size)
			.rect(current_voltage_caption().rect())
			.rect().height(detail_height)
			.snap_to(current_voltage_caption().rect(), snap_type::bottom, 0.f);
		
		// add battery current charge rate
		lecui::widgets::label_builder charge_rate_caption(battery_pane.get());
		charge_rate_caption()
			.text("Charge Rate")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(current_voltage_caption().rect())
			.rect().snap_to(current_voltage_caption().rect(), snap_type::right, 0.f);

		lecui::widgets::label_builder charge_rate(battery_pane.get(), "charge_rate");
		charge_rate()
			.text(_setting_milliunits ?
				std::to_string(battery.current_charge_rate) + "mW" :
				leccore::round_off::to_string(battery.current_charge_rate / 1000.f, 1) + "W")
			.font_size(_detail_font_size)
			.rect(charge_rate_caption().rect())
			.rect().height(detail_height).snap_to(charge_rate_caption().rect(), snap_type::bottom, 0.f);

		// add battery status
		lecui::widgets::label_builder status_caption(battery_pane.get());
		status_caption()
			.text("Status")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(battery_name_caption().rect())
			.rect().width(battery_pane.get().size().width).snap_to(current_voltage().rect(), snap_type::bottom, _margin);

		lecui::widgets::label_builder status(battery_pane.get(), "status");
		status()
			.text(_pc_info.to_string(battery.status))
			.font_size(_detail_font_size)
			.rect(status_caption().rect())
			.rect().height(detail_height).snap_to(status_caption().rect(), snap_type::bottom, 0.f);

		battery_number++;
	}

	battery_tab_pane.select("Battery 0");
}

void main_form::add_drive_details_pane(lecui::containers::page& drive_pane, const float top) {
	lecui::containers::tab_pane_builder drive_tab_pane(drive_pane, "drive_tab_pane");
	drive_tab_pane()
		.rect({ 0.f, drive_pane.size().width, top, drive_pane.size().height })
		.tab_side(lecui::containers::tab_pane_specs::side::top);
	drive_tab_pane().color_tabs().alpha(0);
	drive_tab_pane().color_tabs_border().alpha(0);

	// add as many tab panes as there are drives
	int drive_number = 0;
	for (const auto& drive : _drives) {
		lecui::containers::tab_builder drive_pane(drive_tab_pane, "Drive " + std::to_string(drive_number));

		// add drive model
		lecui::widgets::label_builder drive_model_caption(drive_pane.get());
		drive_model_caption()
			.text("Model")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect({ 0.f, drive_pane.get().size().width, 0.f, caption_height });

		lecui::widgets::label_builder drive_model(drive_pane.get());
		drive_model()
			.text(drive.model)
			.font_size(_detail_font_size)
			.rect(drive_model_caption().rect())
			.rect().height(detail_height).snap_to(drive_model_caption().rect(), snap_type::bottom, 0.f);

		// add drive status
		lecui::widgets::label_builder status_caption(drive_pane.get());
		status_caption()
			.text("Status")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(drive_model_caption().rect())
			.rect().width(drive_pane.get().size().width / 3.f).snap_to(drive_model().rect(), snap_type::bottom_left, _margin);
		
		lecui::widgets::label_builder status(drive_pane.get(), "status");
		status()
			.text(drive.status)
			.font_size(_detail_font_size)
			.rect(status_caption().rect())
			.rect().height(detail_height).snap_to(status_caption().rect(), snap_type::bottom, 0.f);

		if (drive.status == "OK")
			status().color_text(_ok_color);

		// add storage type
		lecui::widgets::label_builder storage_type_caption(drive_pane.get());
		storage_type_caption()
			.text("Storage Type")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(status_caption().rect())
			.rect().snap_to(status_caption().rect(), snap_type::right, 0.f);

		lecui::widgets::label_builder storage_type(drive_pane.get());
		storage_type()
			.text(drive.storage_type)
			.font_size(_detail_font_size)
			.rect(status().rect())
			.rect().snap_to(status().rect(), snap_type::right, 0.f);

		// add bus type
		lecui::widgets::label_builder bus_type_caption(drive_pane.get());
		bus_type_caption()
			.text("Bus Type")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(storage_type_caption().rect())
			.rect().snap_to(storage_type_caption().rect(), snap_type::right, 0.f);

		lecui::widgets::label_builder bus_type(drive_pane.get());
		bus_type()
			.text(drive.bus_type)
			.font_size(_detail_font_size)
			.rect(storage_type().rect())
			.rect().snap_to(storage_type().rect(), snap_type::right, 0.f);

		// add drive serial number
		lecui::widgets::label_builder serial_number_caption(drive_pane.get());
		serial_number_caption()
			.text("Serial Number")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(drive_model_caption().rect())
			.rect().snap_to(status().rect(), snap_type::bottom_left, _margin);

		lecui::widgets::label_builder serial_number(drive_pane.get());
		serial_number()
			.text(drive.serial_number)
			.font_size(_detail_font_size)
			.rect(serial_number_caption().rect())
			.rect().height(detail_height).snap_to(serial_number_caption().rect(), snap_type::bottom, 0.f);

		// add capacity
		lecui::widgets::label_builder capacity(drive_pane.get());
		capacity()
			.text(leccore::format_size(drive.size) + " " + "<span style = 'font-size: 9.0pt;'>capacity</span>")
			.font_size(_highlight_font_size)
			.rect(serial_number().rect())
			.rect().height(highlight_height).snap_to(serial_number().rect(), snap_type::bottom, _margin);

		// add media type
		lecui::widgets::label_builder additional(drive_pane.get());
		additional()
			.text(drive.media_type)
			.font_size(_caption_font_size)
			.rect(capacity().rect())
			.rect().height(caption_height).snap_to(capacity().rect(), snap_type::bottom, 0.f);

		drive_number++;
	}

	drive_tab_pane.select("Drive 0");
}

void main_form::on_refresh() {
	if (!visible())
		return;

	stop_refresh_timer();
	bool refresh_ui = false;

	std::string error;
	std::vector<leccore::pc_info::drive_info> _drives_old = _drives;
	if (!_pc_info.drives(_drives, error)) {}

	leccore::pc_info::power_info _power_old = _power;
	if (!_pc_info.power(_power, error)) {}

	try {
		// refresh pc details
		if (_drives_old.size() != _drives.size()) {
			auto& drive_summary = get_label_specs("home/pc_details_pane/drive_summary");
			drive_summary.text(std::to_string(_drives.size()) + "<span style = 'font-size: 8.0pt;'>" +
				std::string(_drives.size() == 1 ? " drive" : " drives") +
				"</span>");
		}

		if (_power_old.batteries.size() != _power.batteries.size()) {
			auto& battery_summary = get_label_specs("home/pc_details_pane/battery_summary");
			battery_summary.text(std::to_string(_power.batteries.size()) + "<span style = 'font-size: 8.0pt;'>" +
				std::string(_power.batteries.size() == 1 ? " battery" : " batteries") +
				"</span>");
		}
	}
	catch (const std::exception) {}

	try {
		// refresh power details
		if (_power_old.ac != _power.ac) {
			auto& power_status = get_label_specs("home/power_pane/power_status");
			power_status.text() = _power.ac ? "On AC" : "On Battery";
			power_status.text() += ", ";
			power_status.text() += ("<span style = 'font-size: 8.0pt;'>" +
				_pc_info.to_string(_power.status) + "</span>");
			refresh_ui = true;
		}

		if (_power_old.level != _power.level) {
			auto& level = get_label_specs("home/power_pane/level");
			level.text((_power.level != -1 ?
				(std::to_string(_power.level) + "% ") : std::string("<em>Unknown</em> ")) +
				"<span style = 'font-size: 8.0pt;'>overall power level</span>");

			auto& level_bar = get_progress_bar_specs("home/power_pane/level_bar");
			level_bar.percentage(static_cast<float>(_power.level));
			refresh_ui = true;
		}

		if (_power_old.lifetime_remaining != _power.lifetime_remaining) {
			auto& life_remaining = get_label_specs("home/power_pane/life_remaining");
			life_remaining.text(_power.lifetime_remaining.empty() ? std::string() :
				(_power.lifetime_remaining + " remaining"));
			refresh_ui = true;
		}

		if (_power_old.batteries.size() != _power.batteries.size()) {
			// close old tab pane
			_page_man.close("home/power_pane/battery_tab_pane");

			auto& life_remaining = get_label_specs("home/power_pane/life_remaining");
			auto& power_pane = get_pane_page("home/power_pane");

			// add battery pane
			add_battery_pane(power_pane, life_remaining.rect().bottom());

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
						auto& current_capacity = get_label_specs("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/current_capacity");
						current_capacity.text(_setting_milliunits ?
							std::to_string(battery.current_capacity) + "mWh" :
							leccore::round_off::to_string(battery.current_capacity / 1000.f, 1) + "Wh");
					}

					if (battery_old.level != battery.level) {
						auto& charge_level = get_label_specs("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/charge_level");
						charge_level.text(leccore::round_off::to_string(battery.level, 1) + "%");
					}

					if (battery_old.current_charge_rate != battery.current_charge_rate) {
						auto& charge_rate = get_label_specs("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/charge_rate");
						charge_rate.text(_setting_milliunits ?
							std::to_string(battery.current_charge_rate) + "mW" :
							leccore::round_off::to_string(battery.current_charge_rate / 1000.f, 1) + "W");
					}

					if (battery_old.current_voltage != battery.current_voltage ||
						_setting_milliunits_old != _setting_milliunits) {
						auto& current_voltage = get_label_specs("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/current_voltage");
						current_voltage.text(_setting_milliunits ?
							std::to_string(battery.current_voltage) + "mV" :
							leccore::round_off::to_string(battery.current_voltage / 1000.f, 2) + "V");
					}

					if (battery_old.status != battery.status) {
						auto& status = get_label_specs("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/status");
						status.text(_pc_info.to_string(battery.status));
					}

					if (battery_old.designed_capacity != battery.designed_capacity ||
						_setting_milliunits_old != _setting_milliunits) {
						auto& designed_capacity = get_label_specs("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/designed_capacity");
						designed_capacity.text(_setting_milliunits ?
							std::to_string(battery.designed_capacity) + "mWh" :
							leccore::round_off::to_string(battery.designed_capacity / 1000.f, 1) + "Wh");
					}

					if (battery_old.fully_charged_capacity != battery.fully_charged_capacity ||
						_setting_milliunits_old != _setting_milliunits) {
						auto& fully_charged_capacity = get_label_specs("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/fully_charged_capacity");
						fully_charged_capacity.text(_setting_milliunits ?
							std::to_string(battery.fully_charged_capacity) + "mWh" :
							leccore::round_off::to_string(battery.fully_charged_capacity / 1000.f, 1) + "Wh");
					}

					if (battery_old.health != battery.health) {
						auto& health = get_progress_indicator_specs("home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/health");
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
		// to-do: refresh drive details
		if (_drives_old.size() != _drives.size()) {
			// close old tab pane
			_page_man.close("home/drive_pane/drive_tab_pane");

			auto& drive_title = get_label_specs("home/drive_pane/drive_title");
			auto& drive_pane = get_pane_page("home/drive_pane");

			// add drive details pane
			add_drive_details_pane(drive_pane, drive_title.rect().bottom());

			refresh_ui = true;
		}
		else {
			for (size_t drive_number = 0; drive_number < _drives.size(); drive_number++) {
				auto& drive_old = _drives_old[drive_number];
				auto& drive = _drives[drive_number];

				if (drive_old.status != drive.status) {
					auto& status = get_label_specs("home/drive_pane/drive_tab_pane/Drive " + std::to_string(drive_number) + "/status");
					status.text(drive.status);
					if (drive.status == "OK")
						status.color_text(_ok_color);
					else {
						status.color_text(_not_ok_color);
						// to-do: handle more cases
					}

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
		auto& text = get_label_specs("home/update_status").text();
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
			get_label_specs("home/update_status").text("Error while checking for updates");
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
			get_label_specs("home/update_status").text("Update available: " + _update_info.version);
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
			get_label_specs("home/update_status").text("Downloading update ...");
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
			get_label_specs("home/update_status").text("Latest version is already installed");
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
			auto& text = get_label_specs("home/update_status").text();
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
			get_label_specs("home/update_status").text("Downloading update failed");
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
			get_label_specs("home/update_status").text("Update file integrity check failed");
			update();
			close_update_status();
		}
		catch (const std::exception&) {}

		message("Update downloaded but file integrity check failed:\n" + error);
		delete_update_directory();
		return;
	}

	try {
		const auto result_hash = results.at(leccore::hash_file::algorithm::sha256);
		if (result_hash != _update_info.hash) {
			// update status label
			try {
				get_label_specs("home/update_status").text("Update files seem to be corrupt");
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
			get_label_specs("home/update_status").text("Update file integrity check failed");
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
			get_label_specs("home/update_status").text("Downloading update failed");	// to-do: improve
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
		get_label_specs("home/update_status")
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
		auto& pc_details_pane_specs = get_pane_specs("home/pc_details_pane");

		// add update status label
		lecui::widgets::label_builder update_status(home, "update_status");
		update_status()
			.text("Checking for updates")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect().height(caption_height).width(pc_details_pane_specs.rect().width())
			.place(pc_details_pane_specs.rect(), 50.f, 100.f);

		// reduce height of pc details pane to accommodate the new update pane
		pc_details_pane_specs.rect().bottom() = update_status().rect().top() - _margin;

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
		auto& update_status_specs = get_label_specs("home/update_status");
		auto& pc_details_pane_specs = get_pane_specs("home/pc_details_pane");

		// restore size of pc details pane
		pc_details_pane_specs.rect().bottom() = update_status_specs.rect().bottom();

		// close update status label
		_widget_man.close("home/update_status");
		update();
	}
	catch (const std::exception&) {}

	_update_details_displayed = false;
}

main_form::main_form(const std::string& caption) :
	_cleanup_mode(leccore::commandline_arguments::contains("/cleanup")),
	_update_mode(leccore::commandline_arguments::contains("/update")),
	_recent_update_mode(leccore::commandline_arguments::contains("/recentupdate")),
	_system_tray_mode(leccore::commandline_arguments::contains("/systemtray")),
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
