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
#include <liblec/leccore/settings.h>

void main_form::settings() {
	class settings_form : public form {
		lecui::controls ctrls_{ *this };
		lecui::page_management page_man_{ *this };
		lecui::appearance apprnc_{ *this };
		lecui::dimensions dim_{ *this };
		leccore::settings& settings_;
		bool restart_now_ = false;

		// use darktheme setting from parent for consistency before restart
		const bool& setting_darktheme_parent_;
		bool setting_darktheme_ = false;
		bool& setting_milliunits_;

		bool on_initialize(std::string& error) override {
			// read application settings
			std::string value;
			if (!settings_.read_value("", "darktheme", value, error))
				return false;
			else
				setting_darktheme_ = value == "on";		// default to "off"

			if (!settings_.read_value("", "milliunits", value, error))
				return false;
			else
				setting_milliunits_ = value != "no";	// default to "yes"

			// size and stuff
			ctrls_.resize(false);
			ctrls_.minimize(false);
			apprnc_.theme(setting_darktheme_parent_ ? lecui::themes::dark : lecui::themes::light);
			apprnc_.set_icons(ico_resource, ico_resource);
			dim_.size({ 300, 250 });

			return true;
		}

		bool on_layout(std::string& error) override {
			// add home page
			auto& home = page_man_.add("home");

			// add dark theme toggle button
			lecui::widgets::label darktheme_caption(home);
			darktheme_caption().rect.left = margin_;
			darktheme_caption().rect.top = margin_;
			darktheme_caption().rect.width(home.size().width - 2 * margin_);
			darktheme_caption().rect.height(20.f);
			darktheme_caption().text = "Dark theme";

			lecui::widgets::toggle darktheme(home);
			darktheme().rect.width(darktheme_caption().rect.width());
			darktheme().rect.snap_to(darktheme_caption().rect, snap_type::bottom, 0.f);
			darktheme().text = "On";
			darktheme().text_off = "Off";
			darktheme().on = setting_darktheme_;
			darktheme().events().toggle = [&](bool on) { on_darktheme(on); };

			// add milliunits toggle button
			lecui::widgets::label milliunits_caption(home);
			milliunits_caption().rect = darktheme_caption().rect;
			milliunits_caption().rect.snap_to(darktheme().rect, snap_type::bottom, margin_);
			milliunits_caption().text = "Use milliunits";
			
			lecui::widgets::toggle milliunits(home);
			milliunits().rect = darktheme().rect;
			milliunits().rect.snap_to(milliunits_caption().rect, snap_type::bottom, 0.f);
			milliunits().text = "Yes";
			milliunits().text_off = "No";
			milliunits().on = setting_milliunits_;
			milliunits().events().toggle = [&](bool on) { on_milliunits(on); };

			page_man_.show("home");
			return true;
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

	public:
		settings_form(const std::string& caption,
			form& parent,
			leccore::settings& settings,
			bool& setting_darktheme_parent,
			bool& setting_milliunits) :
			form(caption, parent),
			settings_(settings),
			setting_darktheme_parent_(setting_darktheme_parent),
			setting_milliunits_(setting_milliunits) {}

		bool restart_now() {
			return restart_now_;
		}
	};

	// settings objects
	leccore::registry_settings reg_settings_(leccore::registry::scope::current_user);
	reg_settings_.set_registry_path("Software\\com.github.alecmus\\" + std::string(appname));

	leccore::ini_settings ini_settings_("config.cfg");
	ini_settings_.set_ini_path("");	// use current folder

	leccore::settings* p_settings_ = &ini_settings_;
	if (installed_)
		p_settings_ = &reg_settings_;

	settings_form fm(std::string(appname) + " - Settings", *this, *p_settings_,
		setting_darktheme_, setting_milliunits_);
	std::string error;
	if (!fm.show(error))
		message(error);

	if (fm.restart_now()) {
		restart_now_ = true;
		close();
	}
}
