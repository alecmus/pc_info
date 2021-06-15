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

			// size and stuff
			ctrls_.resize(false);
			ctrls_.minimize(false);
			apprnc_.theme(setting_darktheme_parent_ ? lecui::themes::dark : lecui::themes::light);
			apprnc_.set_icons(ico_resource, ico_resource);
			dim_.size({ 300, 300 });

			return true;
		}

		bool on_layout(std::string& error) override {
			// add home page
			auto& home = page_man_.add("home");

			// add settings tab pane
			lecui::containers::tab_pane settings_pane(home, "settings");
			settings_pane().rect.left = margin_;
			settings_pane().rect.top = margin_;
			settings_pane().rect.width(home.size().width - 2.f * margin_);
			settings_pane().rect.height(home.size().height - 2.f * margin_);
			//settings_pane().color_border.alpha = 0;
			settings_pane().color_tabs_border.alpha = 0;
			//settings_pane().color_fill.alpha = 0;
			settings_pane().color_tabs.alpha = 0;
			
			// add appearance tab
			lecui::containers::tab appearance_tab(settings_pane, "Appearance");

			// add dark theme toggle button
			lecui::widgets::label darktheme_caption(appearance_tab.get());
			darktheme_caption().rect.width(appearance_tab.get().size().width);
			darktheme_caption().rect.height(20.f);
			darktheme_caption().text = "Dark theme";

			lecui::widgets::toggle darktheme(appearance_tab.get());
			darktheme().rect.width(darktheme_caption().rect.width());
			darktheme().rect.snap_to(darktheme_caption().rect, snap_type::bottom, 0.f);
			darktheme().text = "On";
			darktheme().text_off = "Off";
			darktheme().on = setting_darktheme_;
			darktheme().events().toggle = [&](bool on) { on_darktheme(on); };

			// add milliunits toggle button
			lecui::widgets::label milliunits_caption(appearance_tab.get());
			milliunits_caption().rect = darktheme_caption().rect;
			milliunits_caption().rect.snap_to(darktheme().rect, snap_type::bottom, 2.f * margin_);
			milliunits_caption().text = "Use milliunits";
			
			lecui::widgets::toggle milliunits(appearance_tab.get());
			milliunits().rect = darktheme().rect;
			milliunits().rect.snap_to(milliunits_caption().rect, snap_type::bottom, 0.f);
			milliunits().text = "Yes";
			milliunits().text_off = "No";
			milliunits().on = setting_milliunits_;
			milliunits().events().toggle = [&](bool on) { on_milliunits(on); };

			// add updates tab
			lecui::containers::tab updates_tab(settings_pane, "Updates");

			// add auto check updates toggle button
			lecui::widgets::label autocheck_updates_caption(updates_tab.get());
			autocheck_updates_caption().rect.width(updates_tab.get().size().width);
			autocheck_updates_caption().rect.height(20.f);
			autocheck_updates_caption().text = "Auto-check";

			lecui::widgets::toggle autocheck_updates(updates_tab.get());
			autocheck_updates().rect = darktheme().rect;
			autocheck_updates().rect.snap_to(autocheck_updates_caption().rect, snap_type::bottom, 0.f);
			autocheck_updates().text = "Yes";
			autocheck_updates().text_off = "No";
			autocheck_updates().on = setting_autocheck_updates_;
			autocheck_updates().events().toggle = [&](bool on) { on_autocheck_updates(on); };

			// add auto download updates toggle button
			lecui::widgets::label autodownload_updates_caption(updates_tab.get());
			autodownload_updates_caption().rect = autocheck_updates_caption().rect;
			autodownload_updates_caption().rect.snap_to(autocheck_updates().rect, snap_type::bottom, 2.f * margin_);
			autodownload_updates_caption().text = "Auto-download";

			lecui::widgets::toggle autodownload_updates(updates_tab.get(), "autodownload_updates");
			autodownload_updates().rect = autocheck_updates().rect;
			autodownload_updates().rect.snap_to(autodownload_updates_caption().rect, snap_type::bottom, 0.f);
			autodownload_updates().text = "Yes";
			autodownload_updates().text_off = "No";
			autodownload_updates().on = setting_autodownload_updates_;
			autodownload_updates().events().toggle = [&](bool on) { on_autodownload_updates(on); };

			settings_pane.select("Appearance");
			page_man_.show("home");
			return true;
		}

		void on_start() override {
			// disable autodownload_updates toggle button is autocheck_updates is off
			widget_man_.enable("home/settings/Updates/autodownload_updates", setting_autocheck_updates_);
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

	public:
		settings_form(const std::string& caption,
			form& parent,
			leccore::settings& settings,
			bool& setting_darktheme_parent,
			bool& setting_milliunits,
			bool& setting_autocheck_updates,
			bool& setting_autodownload_updates) :
			form(caption, parent),
			settings_(settings),
			setting_darktheme_parent_(setting_darktheme_parent),
			setting_milliunits_(setting_milliunits),
			setting_autocheck_updates_(setting_autocheck_updates),
			setting_autodownload_updates_(setting_autodownload_updates) {}

		bool restart_now() {
			return restart_now_;
		}
	};

	settings_form fm(std::string(appname) + " - Settings", *this, settings_,
		setting_darktheme_, setting_milliunits_,
		setting_autocheck_updates_, setting_autodownload_updates_);
	std::string error;
	if (!fm.show(error))
		message(error);

	if (fm.restart_now()) {
		restart_now_ = true;
		close();
	}
}
