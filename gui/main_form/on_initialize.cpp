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
#include "../../create_process.h"
#include <filesystem>
#include <liblec/leccore/zip.h>
#include <liblec/leccore/file.h>

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

	// read pc, power, cpu, gpu, memory and drive info
	_pc_info.pc(_pc_details, error);
	_pc_info.power(_power, error);
	_pc_info.cpu(_cpus, error);
	_pc_info.gpu(_gpus, error);
	_pc_info.ram(_ram, error);
	_pc_info.drives(_drives, error);

	// size and stuff
	_ctrls
		.allow_resize(false)
		.start_hidden(_system_tray_mode);
	_apprnc
		.set_icons(ico_resource, ico_resource)
		.theme(_setting_darktheme ? lecui::themes::dark : lecui::themes::light);

	float form_width = 1120.f;

	if (_power.batteries.empty())
		form_width -= (270.f + _margin);

	_dim.set_size(lecui::size().width(form_width).height(570.f));

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

	return true;
}
