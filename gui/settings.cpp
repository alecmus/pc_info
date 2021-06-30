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

	if (settings_open_)
		return;

	manage_async_access a_(settings_open_);

	class settings_form : public form {
		lecui::controls ctrls_{ *this };
		lecui::page_management page_man_{ *this };
		lecui::widget_management widget_man_{ *this };
		lecui::appearance apprnc_{ *this };
		lecui::dimensions dim_{ *this };
		leccore::settings& settings_;
		bool restart_now_ = false;

		// use darktheme setting from parent for consistency before restart
		const bool& setting_darktheme_parent_;
		bool setting_darktheme_ = false;
		bool& setting_milliunits_;
		bool& setting_autocheck_updates_;
		bool& setting_autodownload_updates_;
		bool& setting_autostart_;
		const std::string& install_location_64_;
		const std::string& install_location_32_;
		const bool& installed_;

		bool on_initialize(std::string& error) override {
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
			else
				// default to "yes"
				setting_autocheck_updates_ = value != "no";

			if (!settings_.read_value("updates", "autodownload", value, error))
				return false;
			else
				// default to "yes"
				setting_autodownload_updates_ = value != "no";

			if (!settings_.read_value("", "autostart", value, error))
				return false;
			else
				// default to no
				setting_autostart_ = value == "yes";

			// size and stuff
			ctrls_
				.allow_resize(false)
				.allow_minimize(false);
			apprnc_
				.set_icons(ico_resource, ico_resource)
				.theme(setting_darktheme_parent_ ? lecui::themes::dark : lecui::themes::light);
			dim_.set_size({ 300, 270 });

			return true;
		}

		bool on_layout(std::string& error) override {
			// add home page
			auto& home = page_man_.add("home");

			// add settings tab pane
			lecui::containers::tab_pane_builder settings_pane(home, "settings");
			settings_pane().rect()
				.left(margin_).top(margin_)
				.width(home.size().width - 2.f * margin_).height(home.size().height - 2.f * margin_);
			settings_pane().color_tabs_border().alpha(0);
			settings_pane().color_tabs().alpha(0);
			
			// add general tab
			lecui::containers::tab_builder general_tab(settings_pane, "General");

			// add dark theme toggle button
			lecui::widgets::label_builder darktheme_caption(general_tab.get());
			darktheme_caption().text("Dark theme");
			darktheme_caption().rect()
				.width(general_tab.get().size().width)
				.height(20.f);

			lecui::widgets::toggle_builder darktheme(general_tab.get());
			darktheme()
				.text("On").text_off("Off").tooltip("Change the UI theme").on(setting_darktheme_)
				.rect().width(darktheme_caption().rect().width()).snap_to(darktheme_caption().rect(), snap_type::bottom, 0.f);
			darktheme().events().toggle = [&](bool on) { on_darktheme(on); };

			// add milliunits toggle button
			lecui::widgets::label_builder milliunits_caption(general_tab.get());
			milliunits_caption()
				.text("Use milliunits")
				.rect(darktheme_caption().rect())
				.rect().snap_to(darktheme().rect(), snap_type::bottom, 2.f * margin_);
			
			lecui::widgets::toggle_builder milliunits(general_tab.get());
			milliunits()
				.text("Yes").text_off("No").tooltip("Change the UI unit display preference").on(setting_milliunits_)
				.rect(darktheme().rect())
				.rect().snap_to(milliunits_caption().rect(), snap_type::bottom, 0.f);
			milliunits().events().toggle = [&](bool on) { on_milliunits(on); };

			// add auto start with windows checkbox
			lecui::widgets::label_builder autostart_label(general_tab.get());
			autostart_label()
				.text("Start automatically with Windows")
				.rect(milliunits().rect())
				.rect().snap_to(milliunits().rect(), snap_type::bottom, 2.f * margin_);

			lecui::widgets::toggle_builder autostart(general_tab.get(), "autostart");
			autostart().text("Yes").text_off("No")
				.tooltip("Select whether to automatically start the app when the user signs into Windows").on(setting_autostart_)
				.rect(milliunits().rect())
				.rect().snap_to(autostart_label().rect(), snap_type::bottom, 0.f);
			autostart().events().toggle = [&](bool on) { on_autostart(on); };

			// add updates tab
			lecui::containers::tab_builder updates_tab(settings_pane, "Updates");

			// add auto check updates toggle button
			lecui::widgets::label_builder autocheck_updates_caption(updates_tab.get());
			autocheck_updates_caption()
				.text("Auto-check")
				.rect().width(updates_tab.get().size().width).height(20.f);

			lecui::widgets::toggle_builder autocheck_updates(updates_tab.get());
			autocheck_updates()
				.text("Yes").text_off("No").tooltip("Select whether to automatically check for updates").on(setting_autocheck_updates_)
				.rect(darktheme().rect())
				.rect().snap_to(autocheck_updates_caption().rect(), snap_type::bottom, 0.f);
			autocheck_updates().events().toggle = [&](bool on) { on_autocheck_updates(on); };

			// add auto download updates toggle button
			lecui::widgets::label_builder autodownload_updates_caption(updates_tab.get());
			autodownload_updates_caption()
				.text("Auto-download")
				.rect(autocheck_updates_caption().rect())
				.rect().snap_to(autocheck_updates().rect(), snap_type::bottom, 2.f * margin_);

			lecui::widgets::toggle_builder autodownload_updates(updates_tab.get(), "autodownload_updates");
			autodownload_updates()
				.text("Yes").text_off("No").tooltip("Select whether to automatically download updates").on(setting_autodownload_updates_)
				.rect(autocheck_updates().rect())
				.rect().snap_to(autodownload_updates_caption().rect(), snap_type::bottom, 0.f);
			
			autodownload_updates().events().toggle = [&](bool on) { on_autodownload_updates(on); };

			settings_pane.select("General");
			page_man_.show("home");
			return true;
		}

		void on_start() override {
			// disable autodownload_updates toggle button is autocheck_updates is off
			widget_man_.enable("home/settings/Updates/autodownload_updates", setting_autocheck_updates_);
			widget_man_.enable("home/settings/General/autostart", installed_);
		}

		void on_darktheme(bool on) {
			std::string error;
			if (!settings_.write_value("", "darktheme", on ? "on" : "off", error)) {
				message("Error saving dark theme setting: " + error);
				// to-do: set toggle button to saved setting (or default if unreadable)
			}
			else {
				setting_darktheme_ = on;

				if (setting_darktheme_ != setting_darktheme_parent_) {
					if (prompt("Would you like to restart the app now for the changes to take effect?")) {
						restart_now_ = true;
						close();
					}
				}
			}
		}

		void on_milliunits(bool on) {
			std::string error;
			if (!settings_.write_value("", "milliunits", on ? "yes" : "no", error)) {
				message("Error saving milliunits setting: " + error);
				// to-do: set toggle button to saved setting (or default if unreadable)
			}
			else
				setting_milliunits_ = on;
		}

		void on_autocheck_updates(bool on) {
			std::string error;
			if (!settings_.write_value("updates", "autocheck", on ? "yes" : "no", error)) {
				message("Error saving auto-check for updates setting: " + error);
				// to-do: set toggle button to saved setting (or default if unreadable)
			}
			else
				setting_autocheck_updates_ = on;

			// disable autodownload_updates toggle button is autocheck_updates is off
			widget_man_.enable("home/settings/Updates/autodownload_updates", setting_autocheck_updates_);
			update();
		}

		void on_autodownload_updates(bool on) {
			std::string error;
			if (!settings_.write_value("updates", "autodownload", on ? "yes" : "no", error)) {
				message("Error saving auto-download updates setting: " + error);
				// to-do: set toggle button to saved setting (or default if unreadable)
			}
			else
				setting_autodownload_updates_ = on;
		}

		void on_autostart(bool on) {
			std::string error;
			if (!settings_.write_value("", "autostart", on ? "yes" : "no", error)) {
				message("Error saving auto-start setting: " + error);
				// to-do: set toggle button to saved setting (or default if unreadable)
			}
			else
				setting_autostart_ = on;

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
			settings_(settings),
			setting_darktheme_parent_(setting_darktheme_parent),
			setting_milliunits_(setting_milliunits),
			setting_autocheck_updates_(setting_autocheck_updates),
			setting_autodownload_updates_(setting_autodownload_updates),
			setting_autostart_(setting_autostart),
			install_location_64_(install_location_64),
			install_location_32_(install_location_32),
			installed_(installed) {}

		bool restart_now() {
			return restart_now_;
		}
	};

	settings_form fm(std::string(appname) + " - Settings", *this, settings_,
		setting_darktheme_, setting_milliunits_,
		setting_autocheck_updates_, setting_autodownload_updates_,
		setting_autostart_, install_location_64_, install_location_32_, installed_);
	std::string error;
	if (!fm.show(error))
		message(error);

	if (fm.restart_now()) {
		restart_now_ = true;
		close();
	}
}
