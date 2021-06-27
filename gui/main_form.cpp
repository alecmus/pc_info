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

const float main_form::margin_ = 10.f;
const float main_form::title_font_size_ = 12.f;
const float main_form::highlight_font_size_ = 14.f;
const float main_form::detail_font_size_ = 10.f;
const float main_form::caption_font_size_ = 8.f;
const std::string main_form::sample_text_ = "<u><strong>Aq</strong></u>";
const std::string main_form::font_ = "Segoe UI";
const lecui::color main_form::caption_color_{ 100, 100, 100, 255 };
const lecui::color main_form::ok_color_{ 0, 150, 0, 255 };
const lecui::color main_form::not_ok_color_{ 200, 0, 0, 255 };
const unsigned long main_form::refresh_interval_ = 3000;

bool main_form::on_initialize(std::string& error) {
	if (!cleanup_mode_ && !update_mode_ && !system_tray_mode_) {
		// display splash screen
		if (get_dpi_scale() < 2.f)
			splash_.display(splash_image_128, false, error);
		else
			splash_.display(splash_image_256, false, error);
	}

	if (cleanup_mode_) {
		if (prompt("Would you like to delete the app settings?")) {
			// cleanup application settings
			if (!settings_.delete_recursive("", error))
				return false;

			if (installed_) {
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
		std::string value;
		if (settings_.read_value("updates", "readytoinstall", value, error)) {}

		// clear the registry entry
		if (!settings_.delete_value("updates", "readytoinstall", error)) {}

		if (!value.empty()) {
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

					if (installed_) {
#ifdef _WIN64
						target_directory = install_location_64_;
#else
						target_directory = install_location_32_;
#endif
					}
					else {
						if (real_portable_mode_) {
							try { target_directory = std::filesystem::current_path().string() + "\\"; }
							catch (const std::exception&) {}
						}
					}

					if (!target_directory.empty()) {
						if (settings_.write_value("updates", "rawfiles", unzipped_folder, error) &&
							settings_.write_value("updates", "target", target_directory, error)) {
							if (real_portable_mode_) {
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
		else
			if (update_mode_) {
				// get the location of the raw files
				if (settings_.read_value("updates", "rawfiles", value, error) && !value.empty()) {
					const std::string raw_files_directory(value);

					if (settings_.read_value("updates", "target", value, error)) {
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
				if (recent_update_mode_) {
					// check if the updates_rawfiles and updates_target settings are set, and eliminated them if so then notify user of successful update
					std::string updates_rawfiles;
					if (!settings_.read_value("updates", "rawfiles", updates_rawfiles, error) && !value.empty()) {}

					std::string updates_target;
					if (!settings_.read_value("updates", "target", updates_target, error)) {}

					if (!updates_rawfiles.empty() || !updates_target.empty()) {
						if (!settings_.delete_value("updates", "rawfiles", error)) {}
						if (!settings_.delete_value("updates", "target", error)) {}

						if (installed_) {
							// update inno setup version number
							leccore::registry reg(leccore::registry::scope::current_user);
#ifdef _WIN64
							if (!reg.do_write("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + install_guid_64_ + "_is1",
								"DisplayVersion", std::string(appversion), error)) {
							}
#else
							if (!reg.do_write("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + install_guid_32_ + "_is1",
								"DisplayVersion", std::string(appversion), error)) {
							}
#endif
						}

						// to-do: use a timer instead for all the calls below, for better user experience
						message("App updated successfully to version " + std::string(appversion));

						std::string updates_tempdirectory;
						if (!settings_.read_value("updates", "tempdirectory", updates_tempdirectory, error)) {}
						else {
							// delete updates temp directory
							if (!leccore::file::remove_directory(updates_tempdirectory, error)) {}
						}
						if (!settings_.delete_value("updates", "tempdirectory", error)) {}
					}
				}
	}

	// read application settings
	std::string value;
	if (!settings_.read_value("", "darktheme", value, error))
		return false;
	else
		// default to "off"
		setting_darktheme_ = value == "on";

	if (!settings_.read_value("", "milliunits", value, error))
		return false;
	else
		// default to "yes"
		setting_milliunits_ = value != "no";

	if (!settings_.read_value("updates", "autocheck", value, error))
		return false;
	else {
		// default to yes
		setting_autocheck_updates_ = value != "no";

		if (setting_autocheck_updates_) {
			if (!settings_.read_value("updates", "did_run_once", value, error))
				return false;
			else {
				if (value != "yes") {
					// do nothing ... for better first time impression
					if (!settings_.write_value("updates", "did_run_once", "yes", error)) {}
				}
				else {
					// start checking for updates
					check_update_.start();

					// start timer to keep progress of the update check
					timer_man_.add("update_check", 1500, [&]() { on_update_check(); });
				}
			}
		}
	}

	if (!settings_.read_value("updates", "autodownload", value, error))
		return false;
	else
		// default to yes
		setting_autodownload_updates_ = value != "no";

	if (!settings_.read_value("", "autostart", value, error))
		return false;
	else
		// default to no
		setting_autostart_ = value == "yes";

	if (setting_autostart_) {
		std::string command;
#ifdef _WIN64
		command = "\"" + install_location_64_ + "pc_info64.exe\"";
#else
		command = "\"" + install_location_32_ + "pc_info32.exe\"";
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
	ctrls_.allow_resize(false);
	apprnc_.theme(setting_darktheme_ ? lecui::themes::dark : lecui::themes::light);
	apprnc_.set_icons(ico_resource, ico_resource);
	dim_.set_size({ 1120, 570 });
	ctrls_.start_hidden(system_tray_mode_);

	// add form caption handler
	form::on_caption([this]() { about(); }, "View info about this app");

	// add form menu
	form_menu_.add("• • •", "Settings and more", {
		{ "Settings", [this]() { settings(); } },
		{ "Updates", [this]() { updates(); } },
		{ "About", [this]() { about(); } },
		{ "" },
		{ "Exit", [this]() { close(); } }
		}, error);

	// read pc, power, cpu, gpu, memory and drive info
	pc_info_.pc(pc_details_, error);
	pc_info_.power(power_, error);
	pc_info_.cpu(cpus_, error);
	pc_info_.gpu(gpus_, error);
	pc_info_.ram(ram_, error);
	pc_info_.drives(drives_, error);

	return true;
}

void main_form::updates() {
	if (check_update_.checking() || timer_man_.running("update_check"))
		return;

	if (download_update_.downloading() || timer_man_.running("update_download"))
		return;

	std::string error, value;
	if (!settings_.read_value("updates", "readytoinstall", value, error)) {}

	if (!value.empty()) {
		// file integrity confirmed ... install update
		if (prompt("An update has already been downloaded and is ready to be installed.\nWould you like to restart the app now so the update can be applied?")) {
			restart_now_ = true;
			close();
		}
		return;
	}

	message("We will check for updates in the background and notify you if any are found.");
	update_check_initiated_manually_ = true;

	// start checking for updates
	check_update_.start();

	// start timer to keep progress of the update check
	timer_man_.add("update_check", 1500, [&]() { on_update_check(); });
}

void main_form::on_start() {
	start_refresh_timer();

	if (installed_) {
		std::string error;
		if (!tray_icon_.add(ico_resource, std::string(appname) + " " +
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

	splash_.remove();
}

void main_form::start_refresh_timer() {
	timer_man_.add("refresh", refresh_interval_, [&]() { on_refresh(); });
}

void main_form::stop_refresh_timer() {
	timer_man_.stop("refresh");
}

bool main_form::on_layout(std::string& error) {
	// add home page
	auto& home = page_man_.add("home");

	// compute label heights
	const lecui::rect page_rect = { 0.f, home.size().width, 0.f, home.size().height };
	title_height = dim_.measure_label(sample_text_, font_, title_font_size_, true, false, page_rect).height();
	highlight_height = dim_.measure_label(sample_text_, font_, highlight_font_size_, true, false, page_rect).height();
	detail_height = dim_.measure_label(sample_text_, font_, detail_font_size_, true, false, page_rect).height();
	caption_height = dim_.measure_label(sample_text_, font_, caption_font_size_, true, false, page_rect).height();

	//////////////////////////////////////////////////
	// 1. Add pane for pc details
	lecui::containers::pane_builder pc_details_pane(home, "pc_details_pane");
	pc_details_pane().rect({ margin_, margin_ + 200.f, margin_, home.size().height - margin_ });

	// add pc details title
	lecui::widgets::label_builder pc_details_title(pc_details_pane.get());
	pc_details_title()
		.rect({ 0.f, pc_details_pane.get().size().width, 0.f, title_height })
		.font_size(title_font_size_)
		.text("<strong>PC DETAILS</strong>");

	// add pc name
	lecui::widgets::label_builder pc_name_caption(pc_details_pane.get());
	pc_name_caption()
		.text("Name")
		.color_text(caption_color_)
		.font_size(caption_font_size_)
		.rect(pc_details_title().rect())
		.rect().height(caption_height).snap_to(pc_details_title().rect(), snap_type::bottom, margin_);

	lecui::widgets::label_builder pc_name(pc_details_pane.get());
	pc_name()
		.text(pc_details_.name)
		.font_size(highlight_font_size_)
		.rect(pc_details_title().rect())
		.rect().height(highlight_height).snap_to(pc_name_caption().rect(), snap_type::bottom, 0.f);

	// add pc manufacturer
	lecui::widgets::label_builder manufacturer_caption(pc_details_pane.get());
	manufacturer_caption()
		.text("Manufacturer")
		.color_text(caption_color_)
		.font_size(caption_font_size_)
		.rect(pc_name_caption().rect())
		.rect().snap_to(pc_name().rect(), snap_type::bottom, margin_);

	lecui::widgets::label_builder manufacturer(pc_details_pane.get());
	manufacturer()
		.text(pc_details_.manufacturer)
		.font_size(detail_font_size_)
		.rect(manufacturer_caption().rect())
		.rect().height(detail_height).snap_to(manufacturer_caption().rect(), snap_type::bottom, 0.f);

	// add model
	lecui::widgets::label_builder model_caption(pc_details_pane.get());
	model_caption()
		.text("Model")
		.color_text(caption_color_)
		.font_size(caption_font_size_)
		.rect(pc_name_caption().rect())
		.rect().snap_to(manufacturer().rect(), snap_type::bottom, margin_);

	lecui::widgets::label_builder model(pc_details_pane.get());
	model()
		.text(pc_details_.model)
		.font_size(detail_font_size_)
		.rect(manufacturer().rect())
		.rect().snap_to(model_caption().rect(), snap_type::bottom, 0.f);

	// add system type
	lecui::widgets::label_builder type_caption(pc_details_pane.get());
	type_caption()
		.text("System type")
		.color_text(caption_color_)
		.font_size(caption_font_size_)
		.rect(pc_name_caption().rect())
		.rect().snap_to(model().rect(), snap_type::bottom, margin_);

	lecui::widgets::label_builder type(pc_details_pane.get());
	type()
		.text(pc_details_.system_type)
		.font_size(detail_font_size_)
		.rect(model().rect())
		.rect().snap_to(type_caption().rect(), snap_type::bottom, 0.f);

	// add bios serial number
	lecui::widgets::label_builder bios_sn_caption(pc_details_pane.get());
	bios_sn_caption()
		.text("BIOS Serial Number")
		.color_text(caption_color_)
		.font_size(caption_font_size_)
		.rect(pc_name_caption().rect())
		.rect().snap_to(type().rect(), snap_type::bottom, margin_);

	lecui::widgets::label_builder bios_sn(pc_details_pane.get());
	bios_sn()
		.text(pc_details_.bios_serial_number)
		.font_size(detail_font_size_)
		.rect(type().rect())
		.rect().snap_to(bios_sn_caption().rect(), snap_type::bottom, 0.f);

	// add board serial number
	lecui::widgets::label_builder board_sn_caption(pc_details_pane.get());
	board_sn_caption()
		.text("Motherboard Serial Number")
		.color_text(caption_color_)
		.font_size(caption_font_size_)
		.rect(pc_name_caption().rect())
		.rect().snap_to(bios_sn().rect(), snap_type::bottom, margin_);
	

	lecui::widgets::label_builder board_sn(pc_details_pane.get());
	board_sn()
		.text(pc_details_.motherboard_serial_number)
		.font_size(detail_font_size_)
		.rect(bios_sn().rect())
		.rect().snap_to(board_sn_caption().rect(), snap_type::bottom, 0.f);

	// add hardware summary details
	if (true) {
		lecui::widgets::label_builder cpu_summary(pc_details_pane.get());
		cpu_summary()
			.text(std::to_string(cpus_.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(cpus_.size() == 1 ? " CPU" : " CPUs") + "</span>")
			.font_size(highlight_font_size_)
			.rect(pc_name().rect())
			.rect().height(highlight_height).snap_to(board_sn().rect(), snap_type::bottom, 2 * margin_);
		
		lecui::widgets::label_builder gpu_summary(pc_details_pane.get());
		gpu_summary()
			.text(std::to_string(gpus_.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(gpus_.size() == 1 ? " GPU" : " GPUs") + "</span>")
			.font_size(highlight_font_size_)
			.rect(pc_name().rect())
			.rect().height(highlight_height).snap_to(cpu_summary().rect(), snap_type::bottom, 0.f);

		lecui::widgets::label_builder ram_summary(pc_details_pane.get());
		ram_summary()
			.text(std::to_string(ram_.ram_chips.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(ram_.ram_chips.size() == 1 ?
				" RAM chip" : " RAM chips") + "</span>")
			.font_size(highlight_font_size_)
			.rect(pc_name().rect())
			.rect().height(highlight_height).snap_to(gpu_summary().rect(), snap_type::bottom, 0.f);

		lecui::widgets::label_builder drive_summary(pc_details_pane.get(), "drive_summary");
		drive_summary()
			.text(std::to_string(drives_.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(drives_.size() == 1 ?
				" drive" : " drives") + "</span>")
			.font_size(highlight_font_size_)
			.rect(pc_name().rect())
			.rect().height(highlight_height).snap_to(ram_summary().rect(), snap_type::bottom, 0.f);

		lecui::widgets::label_builder battery_summary(pc_details_pane.get(), "battery_summary");
		battery_summary()
			.text(std::to_string(power_.batteries.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(power_.batteries.size() == 1 ?
				" battery" : " batteries") + "</span>")
			.font_size(highlight_font_size_)
			.rect(pc_name().rect())
			.rect().height(highlight_height).snap_to(drive_summary().rect(), snap_type::bottom, 0.f);
	}

	//////////////////////////////////////////////////
	// 2. Add pane for power details
	lecui::containers::pane_builder power_pane(home, "power_pane");
	power_pane()
		.rect(pc_details_pane().rect())
		.rect().width(270.f);
	power_pane().rect().snap_to(pc_details_pane().rect(), snap_type::right_top, margin_);

	// add pc details title
	lecui::widgets::label_builder power_details_title(power_pane.get());
	power_details_title()
		.text("<strong>POWER DETAILS</strong>")
		.font_size(title_font_size_)
		.rect({ 0.f, power_pane.get().size().width, 0.f, title_height });

	// add power status
	lecui::widgets::label_builder power_status_caption(power_pane.get());
	power_status_caption()
		.text("Status")
		.color_text(caption_color_)
		.font_size(caption_font_size_)
		.rect(power_details_title().rect())
		.rect().height(caption_height).snap_to(power_details_title().rect(), snap_type::bottom, margin_);

	lecui::widgets::label_builder power_status(power_pane.get(), "power_status");
	power_status()
		.text(std::string(power_.ac ? "On AC" : "On Battery") + (", <span style = 'font-size: 8.0pt;'>" + pc_info_.to_string(power_.status) + "</span>"))
		.font_size(highlight_font_size_)
		.rect(power_details_title().rect())
		.rect().height(highlight_height).snap_to(power_status_caption().rect(), snap_type::bottom, 0.f);

	// add power level
	lecui::widgets::label_builder level(power_pane.get(), "level");
	level().text((power_.level != -1 ?
		(std::to_string(power_.level) + "% ") : std::string("<em>Unknown</em> ")) + "<span style = 'font-size: 8.0pt;'>overall power level</span>")
		.font_size(detail_font_size_)
		.rect(power_status_caption().rect())
		.rect().height(detail_height).snap_to(power_status().rect(), snap_type::bottom, margin_);

	lecui::widgets::progress_bar_builder level_bar(power_pane.get(), "level_bar");
	level_bar()
		.percentage(static_cast<float>(power_.level))
		.rect().width(power_status().rect().width()).snap_to(level().rect(), snap_type::bottom, margin_ / 2.f);

	// add life remaining label
	lecui::widgets::label_builder life_remaining(power_pane.get(), "life_remaining");
	life_remaining()
		.text(power_.lifetime_remaining.empty() ? std::string() : (power_.lifetime_remaining + " remaining"))
		.color_text(caption_color_)
		.font_size(caption_font_size_)
		.rect(level_bar().rect())
		.rect().height(caption_height).snap_to(level_bar().rect(), snap_type::bottom, margin_ / 2.f);

	add_battery_pane(power_pane.get(), life_remaining().rect().bottom());

	//////////////////////////////////////////////////
	// 2. Add pane for cpu details
	lecui::containers::pane_builder cpu_pane(home);
	cpu_pane().rect()
		.left(power_pane().rect().right() + margin_).width(300.f)
		.top(margin_).height(240.f);

	// add cpu title
	lecui::widgets::label_builder cpu_title(cpu_pane.get());
	cpu_title()
		.text("<strong>CPU DETAILS</strong>")
		.font_size(title_font_size_)
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
	for (const auto& cpu : cpus_) {
		lecui::containers::tab_builder cpu_pane(cpu_tab_pane, "CPU " + std::to_string(cpu_number));

		// add cpu name
		lecui::widgets::label_builder cpu_name_caption(cpu_pane.get());
		cpu_name_caption()
			.text("Name")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect({ 0.f, cpu_pane.get().size().width, 0.f, caption_height });

		lecui::widgets::label_builder cpu_name(cpu_pane.get());
		cpu_name()
			.text(cpu.name)
			.font_size(detail_font_size_)
			.rect(cpu_name_caption().rect())
			.rect().height(detail_height).snap_to(cpu_name_caption().rect(), snap_type::bottom, 0.f);

		// add cpu status
		lecui::widgets::label_builder status_caption(cpu_pane.get());
		status_caption()
			.text("Status")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(cpu_name_caption().rect())
			.rect().snap_to(cpu_name().rect(), snap_type::bottom, margin_);

		lecui::widgets::label_builder status(cpu_pane.get());
		status()
			.text(cpu.status)
			.font_size(detail_font_size_)
			.rect(status_caption().rect())
			.rect().height(detail_height).snap_to(status_caption().rect(), snap_type::bottom, 0.f);
		if (cpu.status == "OK")
			status().color_text(ok_color_);

		// add base speed
		lecui::widgets::label_builder base_speed(cpu_pane.get());
		base_speed()
			.text(leccore::round_off::to_string(cpu.base_speed, 2) + "GHz <span style = 'font-size: 8.0pt;'>base speed</span>")
			.font_size(highlight_font_size_)
			.rect(status().rect())
			.rect().height(highlight_height).snap_to(status().rect(), snap_type::bottom, margin_);

		// add cpu cores
		lecui::widgets::label_builder cores(cpu_pane.get());
		cores()
			.font_size(highlight_font_size_)
			.text(std::to_string(cpu.cores) +
				"<span style = 'font-size: 8.0pt;'>" +
				std::string(cpu.cores == 1 ? " core" : " cores") + "</span>, " +
				std::to_string(cpu.logical_processors) +
				"<span style = 'font-size: 8.0pt;'>" +
				std::string(cpu.cores == 1 ? " logical processor" : " logical processors") + "</span>")
			.rect(base_speed().rect())
			.rect().height(highlight_height).snap_to(base_speed().rect(), snap_type::bottom, margin_);

		cpu_number++;
	}

	cpu_tab_pane.select("CPU 0");

	//////////////////////////////////////////////////
	// 3. Add pane for gpu details
	lecui::containers::pane_builder gpu_pane(home);
	gpu_pane().rect()
		.left(cpu_pane().rect().right() + margin_).width(300.f)
		.top(margin_).height(240.f);

	// add gpu title
	lecui::widgets::label_builder gpu_title(gpu_pane.get());
	gpu_title()
		.text("<strong>GPU DETAILS</strong>")
		.font_size(title_font_size_)
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
	for (const auto& gpu : gpus_) {
		lecui::containers::tab_builder gpu_pane(gpu_tab_pane,
			"GPU " + std::to_string(gpu_number));

		// add gpu name
		lecui::widgets::label_builder gpu_name_caption(gpu_pane.get());
		gpu_name_caption()
			.text("Name")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect({ 0.f, gpu_pane.get().size().width, 0.f, caption_height });

		lecui::widgets::label_builder gpu_name(gpu_pane.get());
		gpu_name()
			.text(gpu.name)
			.font_size(detail_font_size_)
			.rect(gpu_name_caption().rect())
			.rect().height(detail_height).snap_to(gpu_name_caption().rect(),
				snap_type::bottom, 0.f);

		// add gpu status
		lecui::widgets::label_builder status_caption(gpu_pane.get());
		status_caption()
			.text("Status")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(gpu_name_caption().rect())
			.rect().snap_to(gpu_name().rect(), snap_type::bottom, margin_);

		lecui::widgets::label_builder status(gpu_pane.get());
		status()
			.font_size(detail_font_size_)
			.rect(status_caption().rect())
			.rect().height(detail_height).snap_to(status_caption().rect(), snap_type::bottom, 0.f);
		status().text() = gpu.status;
		if (gpu.status == "OK")
			status().color_text(ok_color_);

		// add gpu resolution
		lecui::widgets::label_builder resolution_caption(gpu_pane.get());
		resolution_caption()
			.text("Resolution")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(gpu_name_caption().rect())
			.rect().snap_to(status().rect(), snap_type::bottom, margin_);

		lecui::widgets::label_builder resolution(gpu_pane.get());
		resolution()
			.text(std::to_string(gpu.horizontal_resolution) + "x" + std::to_string(gpu.vertical_resolution) + " (" + gpu.resolution_name + ")")
			.font_size(detail_font_size_)
			.rect(resolution_caption().rect())
			.rect().height(detail_height).snap_to(resolution_caption().rect(), snap_type::bottom, 0.f);

		// add refresh rate and memory
		lecui::widgets::label_builder additional(gpu_pane.get());
		additional().text(std::to_string(gpu.refresh_rate) + "Hz " +
			"<span style = 'font-size: 8.0pt;'>refresh rate</span>, " +
			leccore::format_size(gpu.ram) + " " +
			"<span style = 'font-size: 8.0pt;'>graphics memory</span>")
			.font_size(highlight_font_size_)
			.rect(resolution().rect())
			.rect().height(highlight_height).snap_to(resolution().rect(), snap_type::bottom, margin_);

		gpu_number++;
	}

	gpu_tab_pane.select("GPU 0");

	//////////////////////////////////////////////////
	// 4. Add pane for ram details
	lecui::containers::pane_builder ram_pane(home);
	ram_pane().rect()
		.left(cpu_pane().rect().left())
		.width(300.f)
		.top(cpu_pane().rect().bottom() + margin_)
		.height(270.f);

	// add ram title
	lecui::widgets::label_builder ram_title(ram_pane.get());
	ram_title()
		.text("<strong>RAM DETAILS</strong>")
		.font_size(title_font_size_)
		.rect({ 0.f, ram_pane.get().size().width, 0.f, title_height });

	// add ram summary
	lecui::widgets::label_builder ram_summary(ram_pane.get());
	ram_summary().text(leccore::format_size(ram_.size) + " " +
		"<span style = 'font-size: 8.0pt;'>capacity</span>, " +
		std::to_string(ram_.speed) + "MHz " +
		"<span style = 'font-size: 8.0pt;'>speed</span>")
		.font_size(highlight_font_size_)
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
	for (const auto& ram : ram_.ram_chips) {
		lecui::containers::tab_builder ram_pane(ram_tab_pane, "RAM " + std::to_string(ram_number));

		// add ram part number
		lecui::widgets::label_builder ram_part_number_caption(ram_pane.get());
		ram_part_number_caption()
			.text("Part Number")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect({ 0.f, ram_pane.get().size().width, 0.f, caption_height });

		lecui::widgets::label_builder ram_part_number(ram_pane.get());
		ram_part_number()
			.text(ram.part_number)
			.font_size(detail_font_size_)
			.rect(ram_part_number_caption().rect())
			.rect().height(detail_height).snap_to(ram_part_number_caption().rect(), snap_type::bottom, 0.f);

		// add ram manufacturer
		lecui::widgets::label_builder manufacturer_caption(ram_pane.get());
		manufacturer_caption()
			.text("Manufacturer")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(ram_part_number_caption().rect())
			.rect().width(ram_pane.get().size().width).snap_to(ram_part_number().rect(), snap_type::bottom, margin_);

		lecui::widgets::label_builder manufacturer(ram_pane.get());
		manufacturer()
			.text(ram.manufacturer)
			.font_size(detail_font_size_)
			.rect(manufacturer_caption().rect())
			.rect().height(detail_height).snap_to(manufacturer_caption().rect(), snap_type::bottom, 0.f);

		// add ram type
		lecui::widgets::label_builder type_caption(ram_pane.get());
		type_caption()
			.text("Type")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(ram_part_number_caption().rect())
			.rect().width(ram_pane.get().size().width / 2.f).snap_to(manufacturer().rect(), snap_type::bottom_left, margin_);

		lecui::widgets::label_builder type(ram_pane.get());
		type()
			.text(ram.type)
			.font_size(detail_font_size_)
			.rect(type_caption().rect())
			.rect().height(detail_height)
			.snap_to(type_caption().rect(), snap_type::bottom, 0.f);

		// add ram form factor
		lecui::widgets::label_builder form_factor_caption(ram_pane.get());
		form_factor_caption()
			.text("Form Factor")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(type_caption().rect())
			.rect().snap_to(type_caption().rect(), snap_type::right, 0.f);

		lecui::widgets::label_builder form_factor(ram_pane.get());
		form_factor()
			.text(ram.form_factor)
			.font_size(detail_font_size_)
			.rect(form_factor_caption().rect())
			.rect().height(detail_height).snap_to(form_factor_caption().rect(), snap_type::bottom, 0.f);

		// add capacity and speed
		lecui::widgets::label_builder additional(ram_pane.get());
		additional()
			.text(leccore::format_size(ram.capacity) + " " +
				"<span style = 'font-size: 8.0pt;'>capacity</span>, " +
				std::to_string(ram.speed) + "MHz " +
				"<span style = 'font-size: 8.0pt;'>speed</span>")
			.font_size(highlight_font_size_)
			.rect(ram_part_number().rect())
			.rect().height(highlight_height).snap_to(type().rect(), snap_type::bottom, margin_);

		ram_number++;
	}

	ram_tab_pane.select("RAM 0");

	//////////////////////////////////////////////////
	// 5. Add pane for drive details
	lecui::containers::pane_builder drive_pane(home, "drive_pane");
	drive_pane()
		.rect(ram_pane().rect())
		.rect().snap_to(ram_pane().rect(), snap_type::right, margin_);

	// add drive title
	lecui::widgets::label_builder drive_title(drive_pane.get(), "drive_title");
	drive_title().text("<strong>DRIVE DETAILS</strong>")
		.font_size(title_font_size_)
		.rect({ 0.f, drive_pane.get().size().width, 0.f, title_height });

	// add pane for drive details
	add_drive_details_pane(drive_pane.get(), drive_title().rect().bottom());

	page_man_.show("home");
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
	for (const auto& battery : power_.batteries) {
		lecui::containers::tab_builder battery_pane(battery_tab_pane, "Battery " + std::to_string(battery_number));

		// add battery name
		lecui::widgets::label_builder battery_name_caption(battery_pane.get());
		battery_name_caption()
			.text("Name")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect({ 0.f, battery_pane.get().size().width, 0.f, caption_height });

		lecui::widgets::label_builder battery_name(battery_pane.get());
		battery_name()
			.text(battery.name)
			.font_size(detail_font_size_)
			.rect(battery_name_caption().rect())
			.rect().height(detail_height).snap_to(battery_name_caption().rect(), snap_type::bottom, 0.f);

		// add battery manufacturer
		lecui::widgets::label_builder manufacturer_caption(battery_pane.get());
		manufacturer_caption()
			.text("Manufacturer")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(battery_name_caption().rect())
			.rect().width(battery_pane.get().size().width).snap_to(battery_name().rect(), snap_type::bottom, margin_);

		lecui::widgets::label_builder manufacturer(battery_pane.get());
		manufacturer()
			.text(battery.manufacturer)
			.font_size(detail_font_size_)
			.rect(manufacturer_caption().rect())
			.rect().height(detail_height).snap_to(manufacturer_caption().rect(), snap_type::bottom, 0.f);

		// add battery designed capacity
		lecui::widgets::label_builder designed_capacity_caption(battery_pane.get());
		designed_capacity_caption()
			.text("Designed Capacity")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(manufacturer_caption().rect())
			.rect().width(battery_pane.get().size().width / 2.f).snap_to(manufacturer().rect(), snap_type::bottom_left, margin_);

		lecui::widgets::label_builder designed_capacity(battery_pane.get(), "designed_capacity");
		designed_capacity().text(setting_milliunits_ ?
			std::to_string(battery.designed_capacity) + "mWh" :
			leccore::round_off::to_string(battery.designed_capacity / 1000.f, 1) + "Wh")
			.font_size(detail_font_size_)
			.rect(designed_capacity_caption().rect())
			.rect().height(detail_height).snap_to(designed_capacity_caption().rect(), snap_type::bottom, 0.f);

		// add battery fully charged capacity
		lecui::widgets::label_builder fully_charged_capacity_caption(battery_pane.get());
		fully_charged_capacity_caption()
			.text("Fully Charged Capacity")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(designed_capacity_caption().rect())
			.rect().snap_to(designed_capacity_caption().rect(), snap_type::right, 0.f);

		lecui::widgets::label_builder fully_charged_capacity(battery_pane.get(), "fully_charged_capacity");
		fully_charged_capacity()
			.text(setting_milliunits_ ?
				std::to_string(battery.fully_charged_capacity) + "mWh" :
				leccore::round_off::to_string(battery.fully_charged_capacity / 1000.f, 1) + "Wh")
			.font_size(detail_font_size_)
			.rect(fully_charged_capacity_caption().rect())
			.rect().height(detail_height).snap_to(fully_charged_capacity_caption().rect(), snap_type::bottom, 0.f);

		// add battery health
		lecui::widgets::progress_indicator_builder health(battery_pane.get(), "health");
		health()
			.percentage(static_cast<float>(battery.health))
			.rect().snap_to(designed_capacity().rect(), snap_type::bottom_left, margin_);

		lecui::widgets::label_builder health_caption(battery_pane.get());
		health_caption().text("BATTERY HEALTH").center_v(true).color_text(caption_color_).font_size(caption_font_size_)
			.rect(health().rect())
			.rect().right(battery_name().rect().right()).snap_to(health().rect(), snap_type::right, margin_);

		// add battery current capacity
		lecui::widgets::label_builder current_capacity_caption(battery_pane.get());
		current_capacity_caption()
			.text("Current Capacity")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect().width(battery_pane.get().size().width / 2.f).snap_to(health().rect(), snap_type::bottom_left, margin_);

		lecui::widgets::label_builder current_capacity(battery_pane.get(), "current_capacity");
		current_capacity()
			.text(setting_milliunits_ ?
				std::to_string(battery.current_capacity) + "mWh" :
				leccore::round_off::to_string(battery.current_capacity / 1000.f, 1) + "Wh")
			.font_size(detail_font_size_)
			.rect(current_capacity_caption().rect())
			.rect().height(detail_height).snap_to(current_capacity_caption().rect(), snap_type::bottom, 0.f);

		// add battery fully charged capacity
		lecui::widgets::label_builder charge_level_caption(battery_pane.get());
		charge_level_caption()
			.text("Charge Level")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(current_capacity_caption().rect())
			.rect().snap_to(current_capacity_caption().rect(), snap_type::right, 0.f);

		lecui::widgets::label_builder charge_level(battery_pane.get(), "charge_level");
		charge_level()
			.text(leccore::round_off::to_string(battery.level, 1) + "%")
			.font_size(detail_font_size_)
			.rect(charge_level_caption().rect())
			.rect().height(detail_height).snap_to(charge_level_caption().rect(), snap_type::bottom, 0.f);

		// add battery current voltage
		lecui::widgets::label_builder current_voltage_caption(battery_pane.get());
		current_voltage_caption()
			.text("Current Voltage")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(battery_name().rect())
			.rect().width(battery_pane.get().size().width / 2.f).snap_to(current_capacity().rect(), snap_type::bottom_left, margin_);

		lecui::widgets::label_builder current_voltage(battery_pane.get(), "current_voltage");
		current_voltage()
			.text(setting_milliunits_ ?
				std::to_string(battery.current_voltage) + "mV" :
				leccore::round_off::to_string(battery.current_voltage / 1000.f, 2) + "V")
			.font_size(detail_font_size_)
			.rect(current_voltage_caption().rect())
			.rect().height(detail_height)
			.snap_to(current_voltage_caption().rect(), snap_type::bottom, 0.f);
		
		// add battery current charge rate
		lecui::widgets::label_builder charge_rate_caption(battery_pane.get());
		charge_rate_caption()
			.text("Charge Rate")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(current_voltage_caption().rect())
			.rect().snap_to(current_voltage_caption().rect(), snap_type::right, 0.f);

		lecui::widgets::label_builder charge_rate(battery_pane.get(), "charge_rate");
		charge_rate()
			.text(setting_milliunits_ ?
				std::to_string(battery.current_charge_rate) + "mW" :
				leccore::round_off::to_string(battery.current_charge_rate / 1000.f, 1) + "W")
			.font_size(detail_font_size_)
			.rect(charge_rate_caption().rect())
			.rect().height(detail_height).snap_to(charge_rate_caption().rect(), snap_type::bottom, 0.f);

		// add battery status
		lecui::widgets::label_builder status_caption(battery_pane.get());
		status_caption()
			.text("Status")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(battery_name_caption().rect())
			.rect().width(battery_pane.get().size().width).snap_to(current_voltage().rect(), snap_type::bottom, margin_);

		lecui::widgets::label_builder status(battery_pane.get(), "status");
		status()
			.text(pc_info_.to_string(battery.status))
			.font_size(detail_font_size_)
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
	for (const auto& drive : drives_) {
		lecui::containers::tab_builder drive_pane(drive_tab_pane, "Drive " + std::to_string(drive_number));

		// add drive model
		lecui::widgets::label_builder drive_model_caption(drive_pane.get());
		drive_model_caption()
			.text("Model")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect({ 0.f, drive_pane.get().size().width, 0.f, caption_height });

		lecui::widgets::label_builder drive_model(drive_pane.get());
		drive_model()
			.text(drive.model)
			.font_size(detail_font_size_)
			.rect(drive_model_caption().rect())
			.rect().height(detail_height).snap_to(drive_model_caption().rect(), snap_type::bottom, 0.f);

		// add drive status
		lecui::widgets::label_builder status_caption(drive_pane.get());
		status_caption()
			.text("Status")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(drive_model_caption().rect())
			.rect().width(drive_pane.get().size().width / 3.f).snap_to(drive_model().rect(), snap_type::bottom_left, margin_);
		
		lecui::widgets::label_builder status(drive_pane.get(), "status");
		status()
			.text(drive.status)
			.font_size(detail_font_size_)
			.rect(status_caption().rect())
			.rect().height(detail_height).snap_to(status_caption().rect(), snap_type::bottom, 0.f);

		if (drive.status == "OK")
			status().color_text(ok_color_);

		// add storage type
		lecui::widgets::label_builder storage_type_caption(drive_pane.get());
		storage_type_caption()
			.text("Storage Type")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(status_caption().rect())
			.rect().snap_to(status_caption().rect(), snap_type::right, 0.f);

		lecui::widgets::label_builder storage_type(drive_pane.get());
		storage_type()
			.text(drive.storage_type)
			.font_size(detail_font_size_)
			.rect(status().rect())
			.rect().snap_to(status().rect(), snap_type::right, 0.f);

		// add bus type
		lecui::widgets::label_builder bus_type_caption(drive_pane.get());
		bus_type_caption()
			.text("Bus Type")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(storage_type_caption().rect())
			.rect().snap_to(storage_type_caption().rect(), snap_type::right, 0.f);

		lecui::widgets::label_builder bus_type(drive_pane.get());
		bus_type()
			.text(drive.bus_type)
			.font_size(detail_font_size_)
			.rect(storage_type().rect())
			.rect().snap_to(storage_type().rect(), snap_type::right, 0.f);

		// add drive serial number
		lecui::widgets::label_builder serial_number_caption(drive_pane.get());
		serial_number_caption()
			.text("Serial Number")
			.color_text(caption_color_)
			.font_size(caption_font_size_)
			.rect(drive_model_caption().rect())
			.rect().snap_to(status().rect(), snap_type::bottom_left, margin_);

		lecui::widgets::label_builder serial_number(drive_pane.get());
		serial_number()
			.text(drive.serial_number)
			.font_size(detail_font_size_)
			.rect(serial_number_caption().rect())
			.rect().height(detail_height).snap_to(serial_number_caption().rect(), snap_type::bottom, 0.f);

		// add capacity
		lecui::widgets::label_builder capacity(drive_pane.get());
		capacity()
			.text(leccore::format_size(drive.size) + " " + "<span style = 'font-size: 9.0pt;'>capacity</span>")
			.font_size(highlight_font_size_)
			.rect(serial_number().rect())
			.rect().height(highlight_height).snap_to(serial_number().rect(), snap_type::bottom, margin_);

		// add media type
		lecui::widgets::label_builder additional(drive_pane.get());
		additional()
			.text(drive.media_type)
			.font_size(caption_font_size_)
			.rect(capacity().rect())
			.rect().height(caption_height).snap_to(capacity().rect(), snap_type::bottom, 0.f);

		drive_number++;
	}

	drive_tab_pane.select("Drive 0");
}

void main_form::on_refresh() {
	stop_refresh_timer();
	bool refresh_ui = false;

	std::string error;
	std::vector<leccore::pc_info::drive_info> drives_old_ = drives_;
	if (!pc_info_.drives(drives_, error)) {}

	leccore::pc_info::power_info power_old_ = power_;
	if (!pc_info_.power(power_, error)) {}

	try {
		// refresh pc details
		if (drives_old_.size() != drives_.size()) {
			auto& drive_summary = lecui::widgets::label_builder::specs(*this, "home/pc_details_pane/drive_summary");
			drive_summary.text(std::to_string(drives_.size()) + "<span style = 'font-size: 8.0pt;'>" +
				std::string(drives_.size() == 1 ? " drive" : " drives") +
				"</span>");
		}

		if (power_old_.batteries.size() != power_.batteries.size()) {
			auto& battery_summary = lecui::widgets::label_builder::specs(*this, "home/pc_details_pane/battery_summary");
			battery_summary.text(std::to_string(power_.batteries.size()) + "<span style = 'font-size: 8.0pt;'>" +
				std::string(power_.batteries.size() == 1 ? " battery" : " batteries") +
				"</span>");
		}
	}
	catch (const std::exception) {}

	try {
		// refresh power details
		if (power_old_.ac != power_.ac) {
			auto& power_status = lecui::widgets::label_builder::specs(*this, "home/power_pane/power_status");
			power_status.text() = power_.ac ? "On AC" : "On Battery";
			power_status.text() += ", ";
			power_status.text() += ("<span style = 'font-size: 8.0pt;'>" +
				pc_info_.to_string(power_.status) + "</span>");
			refresh_ui = true;
		}

		if (power_old_.level != power_.level) {
			auto& level = lecui::widgets::label_builder::specs(*this, "home/power_pane/level");
			level.text((power_.level != -1 ?
				(std::to_string(power_.level) + "% ") : std::string("<em>Unknown</em> ")) +
				"<span style = 'font-size: 8.0pt;'>overall power level</span>");

			auto& level_bar = lecui::widgets::progress_bar_builder::specs(*this, "home/power_pane/level_bar");
			level_bar.percentage(static_cast<float>(power_.level));
			refresh_ui = true;
		}

		if (power_old_.lifetime_remaining != power_.lifetime_remaining) {
			auto& life_remaining = lecui::widgets::label_builder::specs(*this, "home/power_pane/life_remaining");
			life_remaining.text(power_.lifetime_remaining.empty() ? std::string() :
				(power_.lifetime_remaining + " remaining"));
			refresh_ui = true;
		}

		if (power_old_.batteries.size() != power_.batteries.size()) {
			// close old tab pane
			page_man_.close("home/power_pane/battery_tab_pane");

			auto& life_remaining = lecui::widgets::label_builder::specs(*this, "home/power_pane/life_remaining");
			auto& power_pane = lecui::containers::pane_builder::get(*this, "home/power_pane");

			// add battery pane
			add_battery_pane(power_pane, life_remaining.rect().bottom());

			refresh_ui = true;
		}
		else {
			for (size_t battery_number = 0; battery_number < power_.batteries.size(); battery_number++) {
				auto& battery_old = power_old_.batteries[battery_number];
				auto& battery = power_.batteries[battery_number];

				if (battery_old != battery ||
					setting_milliunits_old_ != setting_milliunits_) {
					if (battery_old.current_capacity != battery.current_capacity ||
						setting_milliunits_old_ != setting_milliunits_) {
						auto& current_capacity = lecui::widgets::label_builder::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/current_capacity");
						current_capacity.text(setting_milliunits_ ?
							std::to_string(battery.current_capacity) + "mWh" :
							leccore::round_off::to_string(battery.current_capacity / 1000.f, 1) + "Wh");
					}

					if (battery_old.level != battery.level) {
						auto& charge_level = lecui::widgets::label_builder::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/charge_level");
						charge_level.text(leccore::round_off::to_string(battery.level, 1) + "%");
					}

					if (battery_old.current_charge_rate != battery.current_charge_rate) {
						auto& charge_rate = lecui::widgets::label_builder::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/charge_rate");
						charge_rate.text(setting_milliunits_ ?
							std::to_string(battery.current_charge_rate) + "mW" :
							leccore::round_off::to_string(battery.current_charge_rate / 1000.f, 1) + "W");
					}

					if (battery_old.current_voltage != battery.current_voltage ||
						setting_milliunits_old_ != setting_milliunits_) {
						auto& current_voltage = lecui::widgets::label_builder::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/current_voltage");
						current_voltage.text(setting_milliunits_ ?
							std::to_string(battery.current_voltage) + "mV" :
							leccore::round_off::to_string(battery.current_voltage / 1000.f, 2) + "V");
					}

					if (battery_old.status != battery.status) {
						auto& status = lecui::widgets::label_builder::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/status");
						status.text(pc_info_.to_string(battery.status));
					}

					if (battery_old.designed_capacity != battery.designed_capacity ||
						setting_milliunits_old_ != setting_milliunits_) {
						auto& designed_capacity = lecui::widgets::label_builder::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/designed_capacity");
						designed_capacity.text(setting_milliunits_ ?
							std::to_string(battery.designed_capacity) + "mWh" :
							leccore::round_off::to_string(battery.designed_capacity / 1000.f, 1) + "Wh");
					}

					if (battery_old.fully_charged_capacity != battery.fully_charged_capacity ||
						setting_milliunits_old_ != setting_milliunits_) {
						auto& fully_charged_capacity = lecui::widgets::label_builder::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/fully_charged_capacity");
						fully_charged_capacity.text(setting_milliunits_ ?
							std::to_string(battery.fully_charged_capacity) + "mWh" :
							leccore::round_off::to_string(battery.fully_charged_capacity / 1000.f, 1) + "Wh");
					}

					if (battery_old.health != battery.health) {
						auto& health = lecui::widgets::progress_indicator_builder::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/health");
						health.percentage(static_cast<float>(battery.health));
					}

					refresh_ui = true;
				}
			}

			setting_milliunits_old_ = setting_milliunits_;
		}
	}
	catch (const std::exception) {}

	try {
		// to-do: refresh drive details
		if (drives_old_.size() != drives_.size()) {
			// close old tab pane
			page_man_.close("home/drive_pane/drive_tab_pane");

			auto& drive_title = lecui::widgets::label_builder::specs(*this, "home/drive_pane/drive_title");
			auto& drive_pane = lecui::containers::pane_builder::get(*this, "home/drive_pane");

			// add drive details pane
			add_drive_details_pane(drive_pane, drive_title.rect().bottom());

			refresh_ui = true;
		}
		else {
			for (size_t drive_number = 0; drive_number < drives_.size(); drive_number++) {
				auto& drive_old = drives_old_[drive_number];
				auto& drive = drives_[drive_number];

				if (drive_old.status != drive.status) {
					auto& status = lecui::widgets::label_builder::specs(*this, "home/drive_pane/drive_tab_pane/Drive " + std::to_string(drive_number) + "/status");
					status.text(drive.status);
					if (drive.status == "OK")
						status.color_text(ok_color_);
					else {
						status.color_text(not_ok_color_);
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
	if (check_update_.checking())
		return;

	// stop the update check timer
	timer_man_.stop("update_check");

	std::string error;
	if (!check_update_.result(update_info_, error)) {
		if (!setting_autocheck_updates_ || update_check_initiated_manually_)
			message("An error occurred while checking for updates:\n" + error);

		return;
	}
	
	// update found
	const std::string current_version(appversion);
	const int result = leccore::compare_versions(current_version, update_info_.version);
	if (result == -1) {
		// newer version available
		if (!setting_autodownload_updates_) {
			if (!prompt("<span style = 'font-size: 11.0pt;'>Update Available</span>\n\n"
				"Your version:\n" + current_version + "\n\n"
				"New version:\n<span style = 'color: rgb(0, 150, 0);'>" + update_info_.version + "</span>, " + update_info_.date + "\n\n"
				"<span style = 'font-size: 11.0pt;'>Description</span>\n" +
				update_info_.description + "\n\n"
				"Would you like to download the update now? (" +
				leccore::format_size(update_info_.size, 2) + ")\n\n"))
				return;
		}

		// create update download folder
		update_directory_ = leccore::user_folder::temp() + "\\" + leccore::hash_string::uuid();

		if (!leccore::file::create_directory(update_directory_, error))
			return;	// to-do: perhaps try again one or two more times? But then again, why would this method fail?

		// download update
		download_update_.start(update_info_.download_url, update_directory_);
		timer_man_.add("update_download", 1000, [&]() { on_update_download(); });
	}
	else {
		if (!setting_autocheck_updates_ || update_check_initiated_manually_)
			message("The latest version is already installed.");
	}
}

void main_form::on_close() {
	if (installed_)
		hide();
	else
		close();
}

void main_form::on_update_download() {
	if (download_update_.downloading())
		return;

	// stop the update download timer
	timer_man_.stop("update_download");

	auto delete_update_directory = [&]() {
		std::string error;
		if (!leccore::file::remove_directory(update_directory_, error)) {}
	};

	std::string error, fullpath;
	if (!download_update_.result(fullpath, error)) {
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
			return;
		}
	}

	leccore::hash_file::hash_results results;
	if (!hash.result(results, error)) {
		message("Update downloaded but file integrity check failed:\n" + error);
		delete_update_directory();
		return;
	}

	try {
		const auto result_hash = results.at(leccore::hash_file::algorithm::sha256);
		if (result_hash != update_info_.hash) {
			// update file possibly corrupted
			message("Update downloaded but files seem to be corrupt and so cannot be installed. "
				"If the problem persists try downloading the latest version of the app manually.");
			delete_update_directory();
			return;
		}
	}
	catch (const std::exception& e) {
		error = e.what();
		message("Update downloaded but file integrity check failed:\n" + error);
		delete_update_directory();
		return;
	}

	// save update location
	if (!settings_.write_value("updates", "readytoinstall", fullpath, error) ||
		!settings_.write_value("updates", "tempdirectory", update_directory_, error)) {
		message("Update downloaded and verified but the following error occurred:\n" + error);
		delete_update_directory();
		return;
	}

	// file integrity confirmed ... install update
	if (prompt("Version " + update_info_.version + " is ready to be installed.\nWould you like to restart the app now so the update can be applied?")) {
		restart_now_ = true;
		close();
	}
}

bool main_form::installed() {
	// check if application is installed
	std::string error;
	leccore::registry reg(leccore::registry::scope::current_user);
	if (!reg.do_read("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + install_guid_32_ + "_is1",
		"InstallLocation", install_location_32_, error)) {
	}
	if (!reg.do_read("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + install_guid_64_ + "_is1",
		"InstallLocation", install_location_64_, error)) {
	}

	installed_ = !install_location_32_.empty() || !install_location_64_.empty();

	auto portable_file_exists = []()->bool {
		try {
			std::filesystem::path path(".portable");
			return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
		}
		catch (const std::exception&) {
			return false;
		}
	};

	if (installed_) {
		// check if app is running from the install location
		try {
			const auto current_path = std::filesystem::current_path().string() + "\\";

			if (current_path != install_location_32_ &&
				current_path != install_location_64_) {
				if (portable_file_exists()) {
					real_portable_mode_ = true;
					installed_ = false;	// run in portable mode
				}
			}
		}
		catch (const std::exception&) {}
	}
	else {
		if (portable_file_exists())
			real_portable_mode_ = true;
	}

	return installed_;
}

main_form::main_form(const std::string& caption) :
	cleanup_mode_(leccore::commandline_arguments::contains("/cleanup")),
	update_mode_(leccore::commandline_arguments::contains("/update")),
	recent_update_mode_(leccore::commandline_arguments::contains("/recentupdate")),
	system_tray_mode_(leccore::commandline_arguments::contains("/systemtray")),
	settings_(installed() ? reg_settings_.base() : ini_settings_.base()),
	form(caption) {
	installed_ = installed();

	if (!installed_)
		// don't allow system tray mode when running in portable mode
		system_tray_mode_ = false;

	reg_settings_.set_registry_path("Software\\com.github.alecmus\\" + std::string(appname));
	ini_settings_.set_ini_path("");	// use app folder for ini settings

	if (cleanup_mode_ || update_mode_ || recent_update_mode_)
		force_instance();
}

main_form::~main_form() {}
