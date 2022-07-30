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

// lecui
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/line.h>
#include <liblec/lecui/widgets/image_view.h>

// leccore
#include <liblec/leccore/system.h>

void main_form::about() {
	if (minimized())
		restore();
	else
		show();

	if (_about_open)
		return;

	manage_async_access _a(_about_open);

	class about_form : public form {
		lecui::controls _ctrls{ *this };
		lecui::page_manager _page_man{ *this };
		lecui::widget_manager _widget_man{ *this };
		lecui::appearance _apprnc{ *this };
		lecui::dimensions _dim{ *this };

		const bool& _setting_darktheme_parent;

		bool on_initialize(std::string& error) {
			// size and stuff
			_ctrls
				.allow_resize(false)
				.allow_minimize(false);

			_apprnc
				.main_icon(ico_resource)
				.mini_icon(ico_resource)
				.caption_icon(get_dpi_scale() < 2.f ? icon_png_32 : icon_png_64)
				.theme(_setting_darktheme_parent ? lecui::themes::dark : lecui::themes::light);
			_dim.set_size(lecui::size().width(400.f).height(430.f));

			return true;
		}

		bool on_layout(std::string& error) {
			// add home page
			auto& home = _page_man.add("home");

			const auto right = home.size().get_width();
			const auto bottom = home.size().get_height();

			// add page title
			auto& title = lecui::widgets::label::add(home);
			title
				.text("<strong>ABOUT THIS APP</strong>")
				.rect(lecui::rect()
					.left(_margin)
					.right(right - _margin)
					.top(_margin)
					.height(25.f))
				.on_resize(lecui::resize_params()
					.width_rate(100.f));

			const auto width = title.rect().width();

			// add app icon
			auto& app_icon = lecui::widgets::image_view::add(home);
			app_icon
				.png_resource(get_dpi_scale() < 2.f ? icon_png_64 : icon_png_128)
				.rect()
				.width(64.f)
				.height(64.f)
				.snap_to(title.rect(), snap_type::bottom_left, 0.f);

			// add app version label
			auto& app_version = lecui::widgets::label::add(home);
			app_version
				.text("<span style = 'font-size: 9.0pt;'>" +
					std::string(appname) + " " + std::string(appversion) + " (" + std::string(architecture) + "), " + std::string(appdate) +
					"</span>")
				.on_resize(lecui::resize_params()
					.width_rate(100.f))
				.rect().width(width).height(20.f).snap_to(app_icon.rect(), snap_type::bottom_left, _margin);

			// add copyright information
			auto& copyright = lecui::widgets::label::add(home);
			copyright
				.text("<span style = 'font-size: 8.0pt;'>© 2021 Alec Musasa</span>")
				.rect().width(width).snap_to(app_version.rect(), snap_type::bottom, 0.f);

			// add more information
			auto& more_info_caption = lecui::widgets::label::add(home);
			more_info_caption
				.text("<strong>For more info</strong>")
				.rect().width(width).snap_to(copyright.rect(), snap_type::bottom, _margin);

			auto& github_link = lecui::widgets::label::add(home);
			github_link
				.text("Visit <span style = 'color: rgb(85, 155, 215);'>https://alecmus.github.io/apps/pc_info</span>")
				.rect().width(width).snap_to(more_info_caption.rect(), snap_type::bottom, 0.f);
			github_link
				.events().action = [this]() {
				std::string error;
				if (!leccore::shell::open("https://alecmus.github.io/apps/pc_info", error))
					message(error);
			};

			// add libraries used
			auto& libraries_used_caption = lecui::widgets::label::add(home);
			libraries_used_caption
				.text("<strong>Libraries used</strong>")
				.rect().width(width).snap_to(github_link.rect(), snap_type::bottom, _margin);

			auto& leccore_version = lecui::widgets::label::add(home);
			leccore_version
				.text(leccore::version())
				.rect().width(width).snap_to(libraries_used_caption.rect(), snap_type::bottom, 0.f);
			leccore_version
				.events().action = [this]() {
				std::string error;
				if (!leccore::shell::open("https://github.com/alecmus/leccore", error))
					message(error);
			};

			auto& lecui_version = lecui::widgets::label::add(home);
			lecui_version
				.text(lecui::version())
				.rect().width(width).snap_to(leccore_version.rect(), snap_type::bottom, 0.f);
			lecui_version
				.events().action = [this]() {
				std::string error;
				if (!leccore::shell::open("https://github.com/alecmus/lecui", error))
					message(error);
			};

			// add additional credits
			auto& addition_credits_caption = lecui::widgets::label::add(home);
			addition_credits_caption
				.text("<strong>Additional credits</strong>")
				.rect().width(width).snap_to(lecui_version.rect(), snap_type::bottom, _margin);

			auto& dinosoft_labs = lecui::widgets::label::add(home);
			dinosoft_labs
				.text("Main icon designed by DinosoftLabs from https://www.flaticon.com")
				.rect().width(width).snap_to(addition_credits_caption.rect(), snap_type::bottom, 0.f);
			dinosoft_labs
				.events().action = [this]() {
				std::string error;
				if (!leccore::shell::open("https://www.flaticon.com/authors/dinosoftlabs", error))
					message(error);
			};

			auto& ongicon = lecui::widgets::label::add(home);
			ongicon
				.text("Copy icons created by Ongicon from https://www.flaticon.com")
				.rect().width(width).snap_to(dinosoft_labs.rect(), snap_type::bottom, 0.f);
			ongicon
				.events().action = [this]() {
				std::string error;
				if (!leccore::shell::open("https://www.flaticon.com/free-icons/copy", error))
					message(error);
			};

			// add line
			auto& license_line = lecui::widgets::line::add(home);
			license_line
				.thickness(0.25f)
				.rect(lecui::rect(ongicon.rect()));
			license_line.rect().top(license_line.rect().bottom());
			license_line.rect().top() += 2.f * _margin;
			license_line.rect().bottom() += 2.f * _margin;

			license_line
				.points(
					{
						lecui::point().x(0.f).y(0.f),
						lecui::point().x(license_line.rect().width()).y(0.f)
					});

			// add license information
			auto& license_notice = lecui::widgets::label::add(home);
			license_notice
				.text("This app is free software released under the MIT License.")
				.rect().width(width).snap_to(license_line.rect(), snap_type::bottom, _margin);

			_page_man.show("home");
			return true;
		}

	public:
		about_form(const std::string& caption,
			form& parent,
			bool& setting_darktheme_parent) :
			form(caption, parent),
			_setting_darktheme_parent(setting_darktheme_parent) {
			// initialize event
			events().initialize = [this](std::string& error) {
				return on_initialize(error);
			};

			// layout event
			events().layout = [this](std::string& error) {
				return on_layout(error);
			};
		}
	};

	about_form fm(std::string(appname), *this, _setting_darktheme);
	std::string error;
	if (!fm.create(error))
		message(error);
}
