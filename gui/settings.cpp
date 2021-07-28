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
#include <liblec/lecui/widgets/toggle.h>

void main_form::settings() {
	restore();

	if (_settings_open)
		return;

	manage_async_access _a(_settings_open);

	class settings_form : public form {
		lecui::controls _ctrls{ *this };
		lecui::page_manager _page_man{ *this };
		lecui::widget_manager _widget_man{ *this };
		lecui::appearance _apprnc{ *this };
		lecui::dimensions _dim{ *this };
		leccore::settings& _settings;
		bool _restart_now = false;

		// use darktheme setting from parent for consistency before restart
		const bool& _setting_darktheme_parent;
		bool _setting_darktheme = false;
		bool& _setting_milliunits;
		bool& _setting_autocheck_updates;
		bool& _setting_autodownload_updates;
		bool& _setting_autostart;
		const std::string& _install_location_64;
		const std::string& _install_location_32;
		const bool& _installed;

		bool on_initialize(std::string& error) override {
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
			else
				// default to "yes"
				_setting_autocheck_updates = value != "no";

			if (!_settings.read_value("updates", "autodownload", value, error))
				return false;
			else
				// default to "yes"
				_setting_autodownload_updates = value != "no";

			if (!_settings.read_value("", "autostart", value, error))
				return false;
			else
				// default to no
				_setting_autostart = value == "yes";

			// size and stuff
			_ctrls
				.allow_resize(false)
				.allow_minimize(false);
			_apprnc
				.set_icons(ico_resource, ico_resource)
				.theme(_setting_darktheme_parent ? lecui::themes::dark : lecui::themes::light);
			_dim.set_size(lecui::size().width(300.f).height(270.f));

			return true;
		}

		bool on_layout(std::string& error) override {
			// add home page
			auto& home = _page_man.add("home");

			// add settings tab pane
			lecui::containers::tab_pane_builder settings_pane(home, "settings");
			settings_pane().rect()
				.left(_margin).top(_margin)
				.width(home.size().get_width() - 2.f * _margin).height(home.size().get_height() - 2.f * _margin);
			settings_pane().color_tabs_border().alpha(0);
			settings_pane().color_tabs().alpha(0);
			
			// add general tab
			lecui::containers::tab_builder general_tab(settings_pane, "General");

			// add dark theme toggle button
			lecui::widgets::label_builder darktheme_caption(general_tab.get());
			darktheme_caption().text("Dark theme");
			darktheme_caption().rect()
				.width(general_tab.get().size().get_width())
				.height(20.f);

			lecui::widgets::toggle_builder darktheme(general_tab.get());
			darktheme()
				.text("On").text_off("Off").tooltip("Change the UI theme").on(_setting_darktheme)
				.rect().width(darktheme_caption().rect().width()).snap_to(darktheme_caption().rect(), snap_type::bottom, 0.f);
			darktheme().events().toggle = [&](bool on) { on_darktheme(on); };

			// add milliunits toggle button
			lecui::widgets::label_builder milliunits_caption(general_tab.get());
			milliunits_caption()
				.text("Use milliunits")
				.rect(darktheme_caption().rect())
				.rect().snap_to(darktheme().rect(), snap_type::bottom, 2.f * _margin);
			
			lecui::widgets::toggle_builder milliunits(general_tab.get());
			milliunits()
				.text("Yes").text_off("No").tooltip("Change the UI unit display preference").on(_setting_milliunits)
				.rect(darktheme().rect())
				.rect().snap_to(milliunits_caption().rect(), snap_type::bottom, 0.f);
			milliunits().events().toggle = [&](bool on) { on_milliunits(on); };

			// add auto start with windows checkbox
			lecui::widgets::label_builder autostart_label(general_tab.get());
			autostart_label()
				.text("Start automatically with Windows")
				.rect(milliunits().rect())
				.rect().snap_to(milliunits().rect(), snap_type::bottom, 2.f * _margin);

			lecui::widgets::toggle_builder autostart(general_tab.get(), "autostart");
			autostart().text("Yes").text_off("No")
				.tooltip("Select whether to automatically start the app when the user signs into Windows").on(_setting_autostart)
				.rect(milliunits().rect())
				.rect().snap_to(autostart_label().rect(), snap_type::bottom, 0.f);
			autostart().events().toggle = [&](bool on) { on_autostart(on); };

			// add updates tab
			lecui::containers::tab_builder updates_tab(settings_pane, "Updates");

			// add auto check updates toggle button
			lecui::widgets::label_builder autocheck_updates_caption(updates_tab.get());
			autocheck_updates_caption()
				.text("Auto-check")
				.rect()
				.width(updates_tab.get().size().get_width())
				.height(20.f);

			lecui::widgets::toggle_builder autocheck_updates(updates_tab.get());
			autocheck_updates()
				.text("Yes").text_off("No").tooltip("Select whether to automatically check for updates").on(_setting_autocheck_updates)
				.rect(darktheme().rect())
				.rect().snap_to(autocheck_updates_caption().rect(), snap_type::bottom, 0.f);
			autocheck_updates().events().toggle = [&](bool on) { on_autocheck_updates(on); };

			// add auto download updates toggle button
			lecui::widgets::label_builder autodownload_updates_caption(updates_tab.get());
			autodownload_updates_caption()
				.text("Auto-download")
				.rect(autocheck_updates_caption().rect())
				.rect().snap_to(autocheck_updates().rect(), snap_type::bottom, 2.f * _margin);

			lecui::widgets::toggle_builder autodownload_updates(updates_tab.get(), "autodownload_updates");
			autodownload_updates()
				.text("Yes").text_off("No").tooltip("Select whether to automatically download updates").on(_setting_autodownload_updates)
				.rect(autocheck_updates().rect())
				.rect().snap_to(autodownload_updates_caption().rect(), snap_type::bottom, 0.f);
			
			autodownload_updates().events().toggle = [&](bool on) { on_autodownload_updates(on); };

			settings_pane.select("General");
			_page_man.show("home");
			return true;
		}

		void on_start() override {
			// disable autodownload_updates toggle button is autocheck_updates is off
			_widget_man.enable("home/settings/Updates/autodownload_updates", _setting_autocheck_updates);
			_widget_man.enable("home/settings/General/autostart", _installed);
		}

		void on_darktheme(bool on) {
			std::string error;
			if (!_settings.write_value("", "darktheme", on ? "on" : "off", error)) {
				message("Error saving dark theme setting: " + error);
				// to-do: set toggle button to saved setting (or default if unreadable)
			}
			else {
				_setting_darktheme = on;

				if (_setting_darktheme != _setting_darktheme_parent) {
					if (prompt("Would you like to restart the app now for the changes to take effect?")) {
						_restart_now = true;
						close();
					}
				}
			}
		}

		void on_milliunits(bool on) {
			std::string error;
			if (!_settings.write_value("", "milliunits", on ? "yes" : "no", error)) {
				message("Error saving milliunits setting: " + error);
				// to-do: set toggle button to saved setting (or default if unreadable)
			}
			else
				_setting_milliunits = on;
		}

		void on_autocheck_updates(bool on) {
			std::string error;
			if (!_settings.write_value("updates", "autocheck", on ? "yes" : "no", error)) {
				message("Error saving auto-check for updates setting: " + error);
				// to-do: set toggle button to saved setting (or default if unreadable)
			}
			else
				_setting_autocheck_updates = on;

			// disable autodownload_updates toggle button is autocheck_updates is off
			_widget_man.enable("home/settings/Updates/autodownload_updates", _setting_autocheck_updates);
			update();
		}

		void on_autodownload_updates(bool on) {
			std::string error;
			if (!_settings.write_value("updates", "autodownload", on ? "yes" : "no", error)) {
				message("Error saving auto-download updates setting: " + error);
				// to-do: set toggle button to saved setting (or default if unreadable)
			}
			else
				_setting_autodownload_updates = on;
		}

		void on_autostart(bool on) {
			std::string error;
			if (!_settings.write_value("", "autostart", on ? "yes" : "no", error)) {
				message("Error saving auto-start setting: " + error);
				// to-do: set toggle button to saved setting (or default if unreadable)
			}
			else
				_setting_autostart = on;

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
		}

	public:
		settings_form(const std::string& caption,
			form& parent,
			leccore::settings& settings,
			bool& setting_darktheme_parent,
			bool& setting_milliunits,
			bool& setting_autocheck_updates,
			bool& setting_autodownload_updates,
			bool& setting_autostart,
			const std::string& install_location_64,
			const std::string& install_location_32,
			const bool& installed) :
			form(caption, parent),
			_settings(settings),
			_setting_darktheme_parent(setting_darktheme_parent),
			_setting_milliunits(setting_milliunits),
			_setting_autocheck_updates(setting_autocheck_updates),
			_setting_autodownload_updates(setting_autodownload_updates),
			_setting_autostart(setting_autostart),
			_install_location_64(install_location_64),
			_install_location_32(install_location_32),
			_installed(installed) {}

		bool restart_now() {
			return _restart_now;
		}
	};

	settings_form fm(std::string(appname) + " - Settings", *this, _settings,
		_setting_darktheme, _setting_milliunits,
		_setting_autocheck_updates, _setting_autodownload_updates,
		_setting_autostart, _install_location_64, _install_location_32, _installed);
	std::string error;
	if (!fm.show(error))
		message(error);

	if (fm.restart_now()) {
		_restart_now = true;
		close();
	}
}
