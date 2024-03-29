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
#include <liblec/lecui/containers/page.h>
#include <liblec/lecui/containers/pane.h>
#include <liblec/lecui/containers/tab_pane.h>

#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/progress_bar.h>
#include <liblec/lecui/widgets/progress_indicator.h>
#include <liblec/lecui/widgets/line.h>
#include <liblec/lecui/widgets/image_view.h>
#include <liblec/lecui/widgets/rectangle.h>

// leccore
#include <liblec/leccore/system.h>

bool main_form::on_layout(std::string& error) {
	// add home page
	auto& home = _page_man.add("home");

	// compute label heights
	const lecui::rect page_rect = { 0.f, home.size().get_width(), 0.f, home.size().get_height() };
	title_height = _dim.measure_label(_sample_text, _font, _title_font_size, lecui::text_alignment::center, lecui::paragraph_alignment::top, page_rect).height();
	highlight_height = _dim.measure_label(_sample_text, _font, _highlight_font_size, lecui::text_alignment::center, lecui::paragraph_alignment::top, page_rect).height();
	detail_height = _dim.measure_label(_sample_text, _font, _detail_font_size, lecui::text_alignment::center, lecui::paragraph_alignment::top, page_rect).height();
	caption_height = _dim.measure_label(_sample_text, _font, _caption_font_size, lecui::text_alignment::center, lecui::paragraph_alignment::top, page_rect).height();

	// 1. Add pc details
	add_pc_details_pane();

	// 2. Add power details
	if (!_power.batteries.empty()) {
		add_power_pane();
		add_battery_pane();
	}

	// 3. Add cpu details
	add_cpu_pane();
	add_cpu_tab_pane();

	// 4. Add gpu details
	add_graphics_pane();
	add_gpu_tab_pane();

	// 5. Add monitor details
	add_monitor_tab_pane();

	// 6. Add ram details
	add_ram_pane();
	add_ram_tab_pane();

	// 7. Add drive details
	add_drive_pane();
	add_drive_tab_pane();

	_page_man.show("home");
	return true;
}

void main_form::add_pc_details_pane() {
	auto& home = get_page("home");

	auto& pc_details_pane = lecui::containers::pane::add(home, "pc_details_pane");
	pc_details_pane.rect(lecui::rect()
		.left(_margin)
		.right(_margin + 200.f)
		.top(_margin)
		.bottom(home.size().get_height() - _margin));

	pc_details_pane.events().mouse_enter = [&]() {
		try {
			auto& copy = get_image_view("home/pc_details_pane/copy");
			copy.opacity(50.f);
		}
		catch (const std::exception&) {}
	};

	pc_details_pane.events().mouse_leave = [&]() {
		try {
			auto& copy = get_image_view("home/pc_details_pane/copy");
			copy.opacity(0.f);
		}
		catch (const std::exception&) {}
	};

	// add pc details title
	auto& pc_details_title = lecui::widgets::label::add(pc_details_pane);
	pc_details_title
		.rect({ 0.f, pc_details_pane.size().get_width(), 0.f, title_height })
		.font_size(_title_font_size)
		.text("<strong>PC DETAILS</strong>");

	// add pc name
	auto& pc_name_caption = lecui::widgets::label::add(pc_details_pane);
	pc_name_caption
		.text("Name")
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(pc_details_title.rect())
		.rect().height(caption_height).snap_to(pc_details_title.rect(), snap_type::bottom, _margin);

	auto& pc_name = lecui::widgets::label::add(pc_details_pane);
	pc_name
		.text(_pc_details.name)
		.font_size(_highlight_font_size)
		.rect(pc_details_title.rect())
		.rect().height(highlight_height).snap_to(pc_name_caption.rect(), snap_type::bottom, 0.f);

	// add pc manufacturer
	auto& manufacturer_caption = lecui::widgets::label::add(pc_details_pane);
	manufacturer_caption
		.text("Manufacturer")
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(pc_name_caption.rect())
		.rect().snap_to(pc_name.rect(), snap_type::bottom, _margin);

	auto& manufacturer = lecui::widgets::label::add(pc_details_pane);
	manufacturer
		.text(_pc_details.manufacturer)
		.font_size(_detail_font_size)
		.rect(manufacturer_caption.rect())
		.rect().height(detail_height).snap_to(manufacturer_caption.rect(), snap_type::bottom, 0.f);

	// add model
	auto& model_caption = lecui::widgets::label::add(pc_details_pane);
	model_caption
		.text("Model")
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(pc_name_caption.rect())
		.rect().snap_to(manufacturer.rect(), snap_type::bottom, _margin);

	auto& model = lecui::widgets::label::add(pc_details_pane);
	model
		.text(_pc_details.model)
		.font_size(_detail_font_size)
		.rect(manufacturer.rect())
		.rect().snap_to(model_caption.rect(), snap_type::bottom, 0.f);

	// add system type
	auto& type_caption = lecui::widgets::label::add(pc_details_pane);
	type_caption
		.text("System type")
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(pc_name_caption.rect())
		.rect().snap_to(model.rect(), snap_type::bottom, _margin);

	auto& type = lecui::widgets::label::add(pc_details_pane);
	type
		.text(_pc_details.system_type)
		.font_size(_detail_font_size)
		.rect(model.rect())
		.rect().snap_to(type_caption.rect(), snap_type::bottom, 0.f);

	// add bios serial number
	auto& bios_sn_caption = lecui::widgets::label::add(pc_details_pane);
	bios_sn_caption
		.text("BIOS Serial Number")
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(pc_name_caption.rect())
		.rect().snap_to(type.rect(), snap_type::bottom, _margin);

	auto& bios_sn = lecui::widgets::label::add(pc_details_pane);
	bios_sn
		.text(_pc_details.bios_serial_number)
		.font_size(_detail_font_size)
		.rect(type.rect())
		.rect().snap_to(bios_sn_caption.rect(), snap_type::bottom, 0.f);

	// add board serial number
	auto& board_sn_caption = lecui::widgets::label::add(pc_details_pane);
	board_sn_caption
		.text("Motherboard Serial Number")
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(pc_name_caption.rect())
		.rect().snap_to(bios_sn.rect(), snap_type::bottom, _margin);

	auto& board_sn = lecui::widgets::label::add(pc_details_pane);
	board_sn
		.text(_pc_details.motherboard_serial_number)
		.font_size(_detail_font_size)
		.rect(bios_sn.rect())
		.rect().snap_to(board_sn_caption.rect(), snap_type::bottom, 0.f);

	// add hardware summary details
	auto& cpu_summary = lecui::widgets::label::add(pc_details_pane);
	cpu_summary
		.text(std::to_string(_cpus.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(_cpus.size() == 1 ? " CPU" : " CPUs") + "</span>")
		.font_size(_highlight_font_size)
		.rect(pc_name.rect())
		.rect().height(highlight_height).snap_to(board_sn.rect(), snap_type::bottom, 2 * _margin);

	auto& gpu_summary = lecui::widgets::label::add(pc_details_pane);
	gpu_summary
		.text(std::to_string(_gpus.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(_gpus.size() == 1 ? " GPU" : " GPUs") + "</span>")
		.font_size(_highlight_font_size)
		.rect(pc_name.rect())
		.rect().height(highlight_height).snap_to(cpu_summary.rect(), snap_type::bottom, 0.f);

	auto& monitor_summary = lecui::widgets::label::add(pc_details_pane, "monitor_summary");
	monitor_summary
		.text(std::to_string(_monitors.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(_monitors.size() == 1 ? " monitor" : " monitors") + "</span>")
		.font_size(_highlight_font_size)
		.rect(pc_name.rect())
		.rect().height(highlight_height).snap_to(gpu_summary.rect(), snap_type::bottom, 0.f);

	auto& ram_summary = lecui::widgets::label::add(pc_details_pane);
	ram_summary
		.text(std::to_string(_ram.ram_chips.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(_ram.ram_chips.size() == 1 ?
			" RAM chip" : " RAM chips") + "</span>")
		.font_size(_highlight_font_size)
		.rect(pc_name.rect())
		.rect().height(highlight_height).snap_to(monitor_summary.rect(), snap_type::bottom, 0.f);

	auto& drive_summary = lecui::widgets::label::add(pc_details_pane, "drive_summary");
	drive_summary
		.text(std::to_string(_drives.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(_drives.size() == 1 ?
			" drive" : " drives") + "</span>")
		.font_size(_highlight_font_size)
		.rect(pc_name.rect())
		.rect().height(highlight_height).snap_to(ram_summary.rect(), snap_type::bottom, 0.f);

	auto& battery_summary = lecui::widgets::label::add(pc_details_pane, "battery_summary");
	battery_summary
		.text(std::to_string(_power.batteries.size()) + "<span style = 'font-size: 8.0pt;'>" + std::string(_power.batteries.size() == 1 ?
			" battery" : " batteries") + "</span>")
		.font_size(_highlight_font_size)
		.rect(pc_name.rect())
		.rect().height(highlight_height).snap_to(drive_summary.rect(), snap_type::bottom, 0.f);

	// add copy details icon
	auto& copy = lecui::widgets::image_view::add(pc_details_pane, "copy");
	copy
		.png_resource(get_dpi_scale() < 2.f ? png_copy_32 : png_copy_64)
		.tooltip("Copy PC Details")
		.rect()
		.left(pc_details_pane.size().get_width() - 24.f).width(24.f)
		.height(24.f);

	copy
		.opacity(0.f)	// invisible by default
		.color_hot().alpha(0);

	copy
		.color_selected().alpha(0);

	copy.events().mouse_enter = [&]() {
		try {
			auto& copy = get_image_view("home/pc_details_pane/copy");
			copy.opacity(100.f);
		}
		catch (const std::exception&) {}
	};

	copy.events().mouse_leave = [&]() {
		auto& copy = get_image_view("home/pc_details_pane/copy");
		copy.opacity(50.f);
	};

	copy.events().action = [&]() {
		std::string error;
		if (!leccore::clipboard::set_text(pc_details_text(), error))
			message(error);
		else
			message("PC details copied to the clipboard.");
	};
}

void main_form::add_power_pane() {
	auto& home = get_page("home");
	auto& pc_details_pane = get_pane("home/pc_details_pane");

	auto& power_pane = lecui::containers::pane::add(home, "power_pane");
	power_pane
		.rect(pc_details_pane.rect())
		.rect().width(270.f).height(550.f);
	power_pane.rect().snap_to(pc_details_pane.rect(), snap_type::right_top, _margin);

	power_pane.events().mouse_enter = [&]() {
		try {
			auto& copy = get_image_view("home/power_pane/copy");
			copy.opacity(50.f);
		}
		catch (const std::exception&) {}
	};

	power_pane.events().mouse_leave = [&]() {
		try {
			auto& copy = get_image_view("home/power_pane/copy");
			copy.opacity(0.f);
		}
		catch (const std::exception&) {}
	};

	// add pc details title
	auto& power_details_title = lecui::widgets::label::add(power_pane);
	power_details_title
		.text("<strong>POWER DETAILS</strong>")
		.font_size(_title_font_size)
		.rect({ 0.f, power_pane.size().get_width(), 0.f, title_height });

	// add power status
	auto& power_status_caption = lecui::widgets::label::add(power_pane);
	power_status_caption
		.text("Status")
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(power_details_title.rect())
		.rect().height(caption_height).snap_to(power_details_title.rect(), snap_type::bottom, _margin);

	auto& power_status = lecui::widgets::label::add(power_pane, "power_status");
	power_status
		.text(std::string(_power.ac ? "On AC" : "On Battery") + (", <span style = 'font-size: 8.0pt;'>" + _pc_info.to_string(_power.status) + "</span>"))
		.font_size(_highlight_font_size)
		.rect(power_details_title.rect())
		.rect().height(highlight_height).snap_to(power_status_caption.rect(), snap_type::bottom, 0.f);

	// add power level
	auto& level = lecui::widgets::label::add(power_pane, "level");
	level.text((_power.level != -1 ?
		(std::to_string(_power.level) + "% ") : std::string("<em>Unknown</em> ")) + "<span style = 'font-size: 8.0pt;'>overall power level</span>")
		.font_size(_detail_font_size)
		.rect(power_status_caption.rect())
		.rect().height(detail_height).snap_to(power_status.rect(), snap_type::bottom, _margin);

	auto& level_bar = lecui::widgets::progress_bar::add(power_pane, "level_bar");
	level_bar
		.percentage(static_cast<float>(_power.level))
		.rect().width(power_status.rect().width()).snap_to(level.rect(), snap_type::bottom, _margin / 2.f);

	// add life remaining label
	auto& life_remaining = lecui::widgets::label::add(power_pane, "life_remaining");
	life_remaining
		.text(_power.lifetime_remaining.empty() ? std::string() : (_power.lifetime_remaining + " remaining"))
		.color_text(_caption_color)
		.font_size(_caption_font_size)
		.rect(level_bar.rect())
		.rect().height(caption_height).snap_to(level_bar.rect(), snap_type::bottom, _margin / 2.f);

	// add copy details icon
	auto& copy = lecui::widgets::image_view::add(power_pane, "copy");
	copy
		.png_resource(get_dpi_scale() < 2.f ? png_copy_32 : png_copy_64)
		.tooltip("Copy Power Details")
		.rect()
		.left(power_pane.size().get_width() - 24.f).width(24.f)
		.height(24.f);

	copy
		.opacity(0.f)	// invisible by default
		.color_hot().alpha(0);

	copy
		.color_selected().alpha(0);

	copy.events().mouse_enter = [&]() {
		try {
			auto& copy = get_image_view("home/power_pane/copy");
			copy.opacity(100.f);
		}
		catch (const std::exception&) {}
	};

	copy.events().mouse_leave = [&]() {
		auto& copy = get_image_view("home/power_pane/copy");
		copy.opacity(50.f);
	};

	copy.events().action = [&]() {
		std::string error;
		if (!leccore::clipboard::set_text(power_details_text(), error))
			message(error);
		else
			message("Power details copied to the clipboard.");
	};
}

void main_form::add_battery_pane() {
	auto& power_pane = get_pane("home/power_pane");
	auto& life_remaining = get_label("home/power_pane/life_remaining");

	// add pane for battery details
	auto& battery_tab_pane = lecui::containers::tab_pane::add(power_pane, "battery_tab_pane");
	battery_tab_pane
		.tab_side(lecui::containers::tab_pane::side::top)
		.rect({ 0.f, power_pane.size().get_width(), life_remaining.rect().bottom(), power_pane.size().get_height() });
	battery_tab_pane.color_tabs().alpha(0);
	battery_tab_pane.color_tabs_border().alpha(0);

	// add as many tab panes as there are batteries
	int battery_number = 0;
	for (const auto& battery : _power.batteries) {
		auto& battery_pane = lecui::containers::tab::add(battery_tab_pane, "Battery " + std::to_string(battery_number));

		// add battery name
		auto& battery_name_caption = lecui::widgets::label::add(battery_pane);
		battery_name_caption
			.text("Name")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect({ 0.f, battery_pane.size().get_width(), 0.f, caption_height });

		auto& battery_name = lecui::widgets::label::add(battery_pane);
		battery_name
			.text(battery.name)
			.font_size(_detail_font_size)
			.rect(battery_name_caption.rect())
			.rect().height(detail_height).snap_to(battery_name_caption.rect(), snap_type::bottom, 0.f);

		// add battery manufacturer
		auto& manufacturer_caption = lecui::widgets::label::add(battery_pane);
		manufacturer_caption
			.text("Manufacturer")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(battery_name_caption.rect())
			.rect().width(battery_pane.size().get_width()).snap_to(battery_name.rect(), snap_type::bottom, _margin);

		auto& manufacturer = lecui::widgets::label::add(battery_pane);
		manufacturer
			.text(battery.manufacturer)
			.font_size(_detail_font_size)
			.rect(manufacturer_caption.rect())
			.rect().height(detail_height).snap_to(manufacturer_caption.rect(), snap_type::bottom, 0.f);

		// add seperator 1
		auto& seperator_1 = lecui::widgets::line::add(battery_pane);
		seperator_1
			.rect(manufacturer_caption.rect())
			.rect().height(1.f).snap_to(manufacturer.rect(), snap_type::bottom, 1.f * _margin);
		seperator_1
			.points({ { 0.f, 0.f }, { seperator_1.rect().width(), 0.f } })
			.thickness(0.25f);

		// add battery health
		auto& health_caption = lecui::widgets::label::add(battery_pane);
		health_caption
			.text("BATTERY HEALTH")
			.alignment(lecui::text_alignment::center)
			.paragraph_alignment(lecui::paragraph_alignment::middle)
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(battery_name_caption.rect())
			.rect().snap_to(seperator_1.rect(), snap_type::bottom, _margin);

		auto& health = lecui::widgets::progress_indicator::add(battery_pane, "health");
		health
			.percentage(static_cast<float>(battery.health))
			.rect().snap_to(health_caption.rect(), snap_type::bottom, _margin);

		lecui::rect ref = seperator_1.rect();
		ref.snap_to(health.rect(), snap_type::bottom, 0.f);

		// add battery designed capacity
		auto& designed_capacity_caption = lecui::widgets::label::add(battery_pane);
		designed_capacity_caption
			.text("Designed Capacity")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(manufacturer_caption.rect())
			.rect().width(battery_pane.size().get_width() / 2.f).snap_to(ref, snap_type::bottom_left, _margin);

		auto& designed_capacity = lecui::widgets::label::add(battery_pane, "designed_capacity");
		designed_capacity.text(_setting_milliunits ?
			std::to_string(battery.designed_capacity) + "mWh" :
			leccore::round_off::to_string(battery.designed_capacity / 1000.f, 1) + "Wh")
			.font_size(_detail_font_size)
			.rect(designed_capacity_caption.rect())
			.rect().height(detail_height).snap_to(designed_capacity_caption.rect(), snap_type::bottom, 0.f);

		// add battery fully charged capacity
		auto& fully_charged_capacity_caption = lecui::widgets::label::add(battery_pane);
		fully_charged_capacity_caption
			.text("Fully Charged Capacity")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(designed_capacity_caption.rect())
			.rect().snap_to(designed_capacity_caption.rect(), snap_type::right, 0.f);

		auto& fully_charged_capacity = lecui::widgets::label::add(battery_pane, "fully_charged_capacity");
		fully_charged_capacity
			.text(_setting_milliunits ?
				std::to_string(battery.fully_charged_capacity) + "mWh" :
				leccore::round_off::to_string(battery.fully_charged_capacity / 1000.f, 1) + "Wh")
			.font_size(_detail_font_size)
			.rect(fully_charged_capacity_caption.rect())
			.rect().height(detail_height).snap_to(fully_charged_capacity_caption.rect(), snap_type::bottom, 0.f);

		// add seperator 2
		auto& seperator_2 = lecui::widgets::line::add(battery_pane);
		seperator_2
			.rect(manufacturer_caption.rect())
			.rect().height(1.f).snap_to(designed_capacity.rect(), snap_type::bottom_left, _margin);
		seperator_2
			.points({ { 0.f, 0.f }, { seperator_2.rect().width(), 0.f } })
			.thickness(0.25f);

		// add battery current capacity
		auto& current_capacity_caption = lecui::widgets::label::add(battery_pane);
		current_capacity_caption
			.text("Current Capacity")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect()
			.width(battery_pane.size().get_width() / 2.f)
			.height(caption_height)
			.snap_to(seperator_2.rect(), snap_type::bottom_left, 1.f * _margin);

		auto& current_capacity = lecui::widgets::label::add(battery_pane, "current_capacity");
		current_capacity
			.text(battery.current_capacity == -1 ? "Unknown" :
				_setting_milliunits ?
				std::to_string(battery.current_capacity) + "mWh" :
				leccore::round_off::to_string(battery.current_capacity / 1000.f, 1) + "Wh")
			.font_size(_detail_font_size)
			.rect(current_capacity_caption.rect())
			.rect().height(detail_height).snap_to(current_capacity_caption.rect(), snap_type::bottom, 0.f);

		// add battery fully charged capacity
		auto& charge_level_caption = lecui::widgets::label::add(battery_pane);
		charge_level_caption
			.text("Charge Level")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(current_capacity_caption.rect())
			.rect().snap_to(current_capacity_caption.rect(), snap_type::right, 0.f);

		auto& charge_level = lecui::widgets::label::add(battery_pane, "charge_level");
		charge_level
			.text(leccore::round_off::to_string(battery.level, 1) + "%")
			.font_size(_detail_font_size)
			.rect(charge_level_caption.rect())
			.rect().height(detail_height).snap_to(charge_level_caption.rect(), snap_type::bottom, 0.f);

		// add battery current voltage
		auto& current_voltage_caption = lecui::widgets::label::add(battery_pane);
		current_voltage_caption
			.text("Current Voltage")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(battery_name_caption.rect())
			.rect().width(battery_pane.size().get_width() / 2.f).snap_to(current_capacity.rect(), snap_type::bottom_left, _margin);

		auto& current_voltage = lecui::widgets::label::add(battery_pane, "current_voltage");
		current_voltage
			.text(battery.current_voltage == -1 ? "Unknown" :
				_setting_milliunits ?
				std::to_string(battery.current_voltage) + "mV" :
				leccore::round_off::to_string(battery.current_voltage / 1000.f, 2) + "V")
			.font_size(_detail_font_size)
			.rect(current_voltage_caption.rect())
			.rect().height(detail_height)
			.snap_to(current_voltage_caption.rect(), snap_type::bottom, 0.f);

		// add battery current charge rate
		auto& charge_rate_caption = lecui::widgets::label::add(battery_pane);
		charge_rate_caption
			.text("Charge Rate")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(current_voltage_caption.rect())
			.rect().snap_to(current_voltage_caption.rect(), snap_type::right, 0.f);

		auto& charge_rate = lecui::widgets::label::add(battery_pane, "charge_rate");
		charge_rate
			.text(_setting_milliunits ?
				std::to_string(battery.current_charge_rate) + "mW" :
				leccore::round_off::to_string(battery.current_charge_rate / 1000.f, 1) + "W")
			.font_size(_detail_font_size)
			.rect(charge_rate_caption.rect())
			.rect().height(detail_height).snap_to(charge_rate_caption.rect(), snap_type::bottom, 0.f);

		// add battery status
		auto& status_caption = lecui::widgets::label::add(battery_pane);
		status_caption
			.text("Status")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(battery_name_caption.rect())
			.rect().width(battery_pane.size().get_width()).snap_to(current_voltage.rect(), snap_type::bottom, _margin);

		auto& status = lecui::widgets::label::add(battery_pane, "status");
		status
			.text(_pc_info.to_string(battery.status))
			.font_size(_detail_font_size)
			.rect(status_caption.rect())
			.rect().height(detail_height).snap_to(status_caption.rect(), snap_type::bottom, 0.f);

		battery_number++;
	}

	battery_tab_pane.selected("Battery 0");
}

void main_form::add_cpu_pane() {
	auto& home = get_page("home");
	auto& pc_details_pane = get_pane("home/pc_details_pane");

	float right = pc_details_pane.rect().right();

	try {
		auto& power_pane = get_pane("home/power_pane");
		right = power_pane.rect().right();
	}
	catch (const std::exception&) {
		// power pane doesn't exist
	}

	auto& cpu_pane = lecui::containers::pane::add(home, "cpu_pane");
	cpu_pane.rect()
		.left(right + _margin).width(300.f)
		.top(_margin).height(200.f);

	cpu_pane.events().mouse_enter = [&]() {
		try {
			auto& copy = get_image_view("home/cpu_pane/copy");
			copy.opacity(50.f);
		}
		catch (const std::exception&) {}
	};

	cpu_pane.events().mouse_leave = [&]() {
		try {
			auto& copy = get_image_view("home/cpu_pane/copy");
			copy.opacity(0.f);
		}
		catch (const std::exception&) {}
	};

	// add cpu title
	auto& cpu_title = lecui::widgets::label::add(cpu_pane, "cpu_title");
	cpu_title
		.text("<strong>CPU DETAILS</strong>")
		.font_size(_title_font_size)
		.rect({ 0.f, cpu_pane.size().get_width(), 0.f, title_height });

	// add copy details icon
	auto& copy = lecui::widgets::image_view::add(cpu_pane, "copy");
	copy
		.png_resource(get_dpi_scale() < 2.f ? png_copy_32 : png_copy_64)
		.tooltip("Copy CPU Details")
		.rect()
		.left(cpu_pane.size().get_width() - 24.f).width(24.f)
		.height(24.f);

	copy
		.opacity(0.f)	// invisible by default
		.color_hot().alpha(0);

	copy
		.color_selected().alpha(0);

	copy.events().mouse_enter = [&]() {
		try {
			auto& copy = get_image_view("home/cpu_pane/copy");
			copy.opacity(100.f);
		}
		catch (const std::exception&) {}
	};

	copy.events().mouse_leave = [&]() {
		try {
			auto& copy = get_image_view("home/cpu_pane/copy");
			copy.opacity(50.f);
		}
		catch (const std::exception&) {}
	};

	copy.events().action = [&]() {
		std::string error;
		if (!leccore::clipboard::set_text(cpu_details_text(), error))
			message(error);
		else
			message("CPU details copied to the clipboard.");
	};
}

void main_form::add_cpu_tab_pane() {
	auto& cpu_pane = get_pane("home/cpu_pane");
	auto& cpu_title = get_label("home/cpu_pane/cpu_title");

	auto& cpu_tab_pane = lecui::containers::tab_pane::add(cpu_pane);
	cpu_tab_pane.tab_side(lecui::containers::tab_pane::side::top);
	cpu_tab_pane.rect()
		.left(0.f)
		.right(cpu_pane.size().get_width())
		.top(cpu_title.rect().bottom())
		.bottom(cpu_pane.size().get_height());
	cpu_tab_pane.color_tabs().alpha(0);
	cpu_tab_pane.color_tabs_border().alpha(0);

	// add as many tab panes as there are cpus
	int cpu_number = 0;
	for (const auto& cpu : _cpus) {
		auto& cpu_pane = lecui::containers::tab::add(cpu_tab_pane, "CPU " + std::to_string(cpu_number));

		// add cpu name
		auto& cpu_name_caption = lecui::widgets::label::add(cpu_pane);
		cpu_name_caption
			.text("Name")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect({ 0.f, cpu_pane.size().get_width(), 0.f, caption_height });

		auto& cpu_name = lecui::widgets::label::add(cpu_pane);
		cpu_name
			.text(cpu.name)
			.font_size(_detail_font_size)
			.rect(cpu_name_caption.rect())
			.rect().height(detail_height).snap_to(cpu_name_caption.rect(), snap_type::bottom, 0.f);

		// add cpu status
		auto& status_caption = lecui::widgets::label::add(cpu_pane);
		status_caption
			.text("Status")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(cpu_name_caption.rect())
			.rect().width(cpu_pane.size().get_width() / 4.f).snap_to(cpu_name.rect(), snap_type::bottom_left, _margin);

		auto& status = lecui::widgets::label::add(cpu_pane);
		status
			.text(cpu.status)
			.font_size(_detail_font_size)
			.rect(status_caption.rect())
			.rect().height(detail_height).snap_to(status_caption.rect(), snap_type::bottom, 0.f);
		
		if (cpu.status == "OK")
			status.color_text(_ok_color);

		// add base speed
		auto& base_speed = lecui::widgets::label::add(cpu_pane);
		base_speed
			.text(leccore::round_off::to_string(cpu.base_speed, 2) + "GHz <span style = 'font-size: 8.0pt;'>base speed</span>")
			.font_size(_highlight_font_size)
			.rect(status.rect())
			.rect().width(3.f * cpu_pane.size().get_width() / 4.f).height(highlight_height).snap_to(status.rect(), snap_type::right_bottom, 0.f);

		// add cpu cores
		auto& cores = lecui::widgets::label::add(cpu_pane);
		cores
			.font_size(_highlight_font_size)
			.text(std::to_string(cpu.cores) +
				"<span style = 'font-size: 8.0pt;'>" +
				std::string(cpu.cores == 1 ? " core" : " cores") + "</span>, " +
				std::to_string(cpu.logical_processors) +
				"<span style = 'font-size: 8.0pt;'>" +
				std::string(cpu.cores == 1 ? " logical processor" : " logical processors") + "</span>")
			.rect(cpu_name_caption.rect())
			.rect().height(highlight_height).snap_to(status.rect(), snap_type::bottom_right, _margin);

		cpu_number++;
	}

	cpu_tab_pane.selected("CPU 0");
}

void main_form::add_graphics_pane() {
	auto& home = get_page("home");
	auto& cpu_pane = get_pane("home/cpu_pane");

	auto& graphics_pane = lecui::containers::pane::add(home, "graphics_pane");
	graphics_pane.rect()
		.left(cpu_pane.rect().left())
		.width(300.f)
		.top(cpu_pane.rect().bottom() + _margin)
		.height(340.f);

	graphics_pane.events().mouse_enter = [&]() {
		try {
			auto& copy = get_image_view("home/graphics_pane/copy");
			copy.opacity(50.f);
		}
		catch (const std::exception&) {}
	};

	graphics_pane.events().mouse_leave = [&]() {
		try {
			auto& copy = get_image_view("home/graphics_pane/copy");
			copy.opacity(0.f);
		}
		catch (const std::exception&) {}
	};

	// add graphics title
	auto& graphics_title = lecui::widgets::label::add(graphics_pane, "graphics_title");
	graphics_title
		.text("<strong>GRAPHICS DETAILS</strong>")
		.font_size(_title_font_size)
		.rect({ 0.f, graphics_pane.size().get_width(), 0.f, title_height });

	// add copy details icon
	auto& copy = lecui::widgets::image_view::add(graphics_pane, "copy");
	copy
		.png_resource(get_dpi_scale() < 2.f ? png_copy_32 : png_copy_64)
		.tooltip("Copy Graphics Details")
		.rect()
		.left(graphics_pane.size().get_width() - 24.f).width(24.f)
		.height(24.f);

	copy
		.opacity(0.f)	// invisible by default
		.color_hot().alpha(0);

	copy
		.color_selected().alpha(0);

	copy.events().mouse_enter = [&]() {
		try {
			auto& copy = get_image_view("home/graphics_pane/copy");
			copy.opacity(100.f);
		}
		catch (const std::exception&) {}
	};

	copy.events().mouse_leave = [&]() {
		auto& copy = get_image_view("home/graphics_pane/copy");
		copy.opacity(50.f);
	};

	copy.events().action = [&]() {
		std::string error;
		if (!leccore::clipboard::set_text(graphics_details_text(), error))
			message(error);
		else
			message("Graphics details copied to the clipboard.");
	};
}

void main_form::add_gpu_tab_pane() {
	auto& graphics_pane = get_pane("home/graphics_pane");
	auto& graphics_title = get_label("home/graphics_pane/graphics_title");

	auto& gpu_tab_pane = lecui::containers::tab_pane::add(graphics_pane, "gpu_tab_pane");
	gpu_tab_pane.tab_side(lecui::containers::tab_pane::side::top);
	gpu_tab_pane.rect()
		.left(0.f)
		.right(graphics_pane.size().get_width())
		.top(graphics_title.rect().bottom())
		.height(125.f);
	gpu_tab_pane.color_tabs().alpha(0);
	gpu_tab_pane.color_tabs_border().alpha(0);

	// add as many tabs as there are gpus
	int gpu_number = 0;
	for (const auto& gpu : _gpus) {
		auto& gpu_pane = lecui::containers::tab::add(gpu_tab_pane,
			"GPU " + std::to_string(gpu_number));

		if (gpu.name != _microsoft_basic_display_adapter_name) {
			// add gpu name
			auto& gpu_name_caption = lecui::widgets::label::add(gpu_pane);
			gpu_name_caption
				.text("Name")
				.color_text(_caption_color)
				.font_size(_caption_font_size)
				.rect({ 0.f, gpu_pane.size().get_width(), 0.f, caption_height });

			auto& gpu_name = lecui::widgets::label::add(gpu_pane);
			gpu_name
				.text(gpu.name)
				.font_size(_detail_font_size)
				.rect(gpu_name_caption.rect())
				.rect().height(detail_height).snap_to(gpu_name_caption.rect(),
					snap_type::bottom, 0.f);

			// add gpu status
			auto& status_caption = lecui::widgets::label::add(gpu_pane);
			status_caption
				.text("Status")
				.color_text(_caption_color)
				.font_size(_caption_font_size)
				.rect(gpu_name_caption.rect())
				.rect().width(gpu_pane.size().get_width() / 5.f).snap_to(gpu_name.rect(), snap_type::bottom_left, _margin);

			auto& status = lecui::widgets::label::add(gpu_pane, "status");
			status
				.text(gpu.status)
				.font_size(_detail_font_size)
				.rect(status_caption.rect())
				.rect().height(detail_height).snap_to(status_caption.rect(), snap_type::bottom, 0.f);

			if (gpu.status == "OK")
				status.color_text(_ok_color);

			// add dedicated video memory
			auto& dedicated_ram_caption = lecui::widgets::label::add(gpu_pane);
			dedicated_ram_caption
				.text("Dedicated Memory")
				.color_text(_caption_color)
				.font_size(_caption_font_size)
				.rect(status_caption.rect())
				.rect().width(2.f * status_caption.rect().width()).snap_to(status_caption.rect(), snap_type::right, 0.f);

			auto& dedicated_ram = lecui::widgets::label::add(gpu_pane);
			dedicated_ram
				.text(leccore::format_size(gpu.dedicated_vram))
				.font_size(_detail_font_size)
				.rect(dedicated_ram_caption.rect())
				.rect().height(status.rect().height()).width(2.f * status_caption.rect().width()).snap_to(status.rect(), snap_type::right, 0.f);

			// add total available graphics memory
			auto& total_graphics_memory_caption = lecui::widgets::label::add(gpu_pane);
			total_graphics_memory_caption
				.text("Total Available")
				.color_text(_caption_color)
				.font_size(_caption_font_size)
				.rect(dedicated_ram_caption.rect())
				.rect().snap_to(dedicated_ram_caption.rect(), snap_type::right, 0.f);

			auto& total_graphics_memory = lecui::widgets::label::add(gpu_pane);
			total_graphics_memory
				.text(leccore::format_size(gpu.total_graphics_memory))
				.font_size(_detail_font_size)
				.rect(dedicated_ram.rect())
				.rect().snap_to(dedicated_ram.rect(), snap_type::right, 0.f);
		}
		else {
			// add notice
			auto& notice = lecui::widgets::label::add(gpu_pane);
			notice
				.text("Graphics driver not installed")
				.color_text(_caption_color)
				.font_size(_caption_font_size)
				.rect({ 0.f, gpu_pane.size().get_width(), 0.f, caption_height });
		}

		gpu_number++;
	}

	gpu_tab_pane.selected("GPU 0");
}

void main_form::add_monitor_tab_pane() {
	auto& graphics_pane = get_pane("home/graphics_pane");
	auto& gpu_tab_pane = get_tab_pane("home/graphics_pane/gpu_tab_pane");

	auto& monitor_tab_pane = lecui::containers::tab_pane::add(graphics_pane, "monitor_tab_pane");
	monitor_tab_pane.tab_side(lecui::containers::tab_pane::side::top);
	monitor_tab_pane.rect()
		.left(0.f)
		.right(graphics_pane.size().get_width())
		.top(gpu_tab_pane.rect().bottom() + _margin)
		.bottom(graphics_pane.size().get_height());
	monitor_tab_pane.color_tabs().alpha(0);
	monitor_tab_pane.color_tabs_border().alpha(0);

	// add as many tab panes as there are monitors
	int monitor_number = 0;
	for (const auto& monitor : _monitors) {
		auto& monitor_pane = lecui::containers::tab::add(monitor_tab_pane, "Monitor " + std::to_string(monitor_number));

		// add monitor name
		auto& monitor_name_caption = lecui::widgets::label::add(monitor_pane);
		monitor_name_caption
			.text("Name")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect({ 0.f, monitor_pane.size().get_width(), 0.f, caption_height });

		auto& monitor_name = lecui::widgets::label::add(monitor_pane);
		monitor_name
			.text(monitor.manufacturer + monitor.product_code_id)
			.font_size(_detail_font_size)
			.rect(monitor_name_caption.rect())
			.rect().height(detail_height).snap_to(monitor_name_caption.rect(), snap_type::bottom, 0.f);

		// get highest supported mode
		leccore::pc_info::video_mode highest_mode = {};

		for (auto& mode : monitor.supported_modes) {
			if (highest_mode.horizontal_resolution < mode.horizontal_resolution)
				highest_mode = mode;
		}

		// add screen size
		auto& size_caption = lecui::widgets::label::add(monitor_pane);
		size_caption
			.text("Size")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(monitor_name_caption.rect())
			.rect().width(monitor_pane.size().get_width() / 3.f).snap_to(monitor_name.rect(), snap_type::bottom_left, _margin);

		auto& size = lecui::widgets::label::add(monitor_pane);
		size
			.font_size(_detail_font_size)
			.rect(size_caption.rect())
			.rect().height(detail_height).snap_to(size_caption.rect(), snap_type::bottom, 0.f);
		size.text() = (leccore::round_off::to_string(highest_mode.physical_size, 1) +
			" <span style = 'font-size: 8.0pt;'>inches</span>");

		// add highest refresh rate
		auto& highest_refresh_rate_caption = lecui::widgets::label::add(monitor_pane);
		highest_refresh_rate_caption
			.text("Max. Refresh")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(size_caption.rect())
			.rect().snap_to(size_caption.rect(), snap_type::right, 0.f);

		auto& highest_refresh_rate = lecui::widgets::label::add(monitor_pane);
		highest_refresh_rate
			.text(leccore::round_off::to_string(highest_mode.refresh_rate, 1) + " Hz")
			.font_size(_detail_font_size)
			.rect(highest_refresh_rate_caption.rect())
			.rect().height(detail_height).snap_to(highest_refresh_rate_caption.rect(), snap_type::bottom, 0.f);

		// add highest pixel clock rate
		auto& highest_pixel_clock_rate_caption = lecui::widgets::label::add(monitor_pane);
		highest_pixel_clock_rate_caption
			.text("Max. Pixel Clock")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(highest_refresh_rate_caption.rect())
			.rect().snap_to(highest_refresh_rate_caption.rect(), snap_type::right, 0.f);

		auto& highest_pixel_clock_rate = lecui::widgets::label::add(monitor_pane);
		highest_pixel_clock_rate
			.text(leccore::round_off::to_string((double(highest_mode.pixel_clock_rate) / (1000.0 * 1000.0)), 1) + " MHz")
			.font_size(_detail_font_size)
			.rect(highest_pixel_clock_rate_caption.rect())
			.rect().height(detail_height).snap_to(highest_pixel_clock_rate_caption.rect(), snap_type::bottom, 0.f);

		// add highest resolution
		auto& highest_resolution_caption = lecui::widgets::label::add(monitor_pane);
		highest_resolution_caption
			.text("Max. Screen Resolution")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(monitor_name_caption.rect())
			.rect().snap_to(size.rect(), snap_type::bottom_left, _margin);

		auto& highest_resolution = lecui::widgets::label::add(monitor_pane);
		highest_resolution
			.text(std::to_string(highest_mode.horizontal_resolution) + "x" + std::to_string(highest_mode.vertical_resolution) + " (" + highest_mode.resolution_name + ")")
			.font_size(_detail_font_size)
			.rect(highest_resolution_caption.rect())
			.rect().height(detail_height).snap_to(highest_resolution_caption.rect(), snap_type::bottom, 0.f);

		monitor_number++;
	}

	monitor_tab_pane.selected("Monitor 0");
}

void main_form::add_ram_pane() {
	auto& home = get_page("home");
	auto& cpu_pane = get_pane("home/cpu_pane");

	auto& ram_pane = lecui::containers::pane::add(home, "ram_pane");
	ram_pane.rect()
		.left(cpu_pane.rect().right() + _margin).width(300.f)
		.top(_margin).height(285.f);

	ram_pane.events().mouse_enter = [&]() {
		try {
			auto& copy = get_image_view("home/ram_pane/copy");
			copy.opacity(50.f);
		}
		catch (const std::exception&) {}
	};

	ram_pane.events().mouse_leave = [&]() {
		try {
			auto& copy = get_image_view("home/ram_pane/copy");
			copy.opacity(0.f);
		}
		catch (const std::exception&) {}
	};

	// add ram title
	auto& ram_title = lecui::widgets::label::add(ram_pane);
	ram_title
		.text("<strong>RAM DETAILS</strong>")
		.font_size(_title_font_size)
		.rect({ 0.f, ram_pane.size().get_width(), 0.f, title_height });

	// add ram summary
	auto& ram_summary = lecui::widgets::label::add(ram_pane, "ram_summary");
	ram_summary.text(leccore::format_size(_ram.size) + " " +
		"<span style = 'font-size: 8.0pt;'>total capacity</span>, " +
		std::to_string(_ram.speed) + "MHz " +
		"<span style = 'font-size: 8.0pt;'>speed</span>")
		.font_size(_highlight_font_size)
		.rect(ram_title.rect())
		.rect().snap_to(ram_title.rect(), snap_type::bottom, _margin).height(highlight_height);

	// add copy details icon
	auto& copy = lecui::widgets::image_view::add(ram_pane, "copy");
	copy
		.png_resource(get_dpi_scale() < 2.f ? png_copy_32 : png_copy_64)
		.tooltip("Copy RAM Details")
		.rect()
		.left(ram_pane.size().get_width() - 24.f).width(24.f)
		.height(24.f);

	copy
		.opacity(0.f)	// invisible by default
		.color_hot().alpha(0);

	copy
		.color_selected().alpha(0);

	copy.events().mouse_enter = [&]() {
		try {
			auto& copy = get_image_view("home/ram_pane/copy");
			copy.opacity(100.f);
		}
		catch (const std::exception&) {}
	};

	copy.events().mouse_leave = [&]() {
		auto& copy = get_image_view("home/ram_pane/copy");
		copy.opacity(50.f);
	};

	copy.events().action = [&]() {
		std::string error;
		if (!leccore::clipboard::set_text(ram_details_text(), error))
			message(error);
		else
			message("RAM details copied to the clipboard.");
	};
}

void main_form::add_ram_tab_pane() {
	auto& ram_pane = get_pane("home/ram_pane");
	auto& ram_summary = get_label("home/ram_pane/ram_summary");

	auto& ram_tab_pane = lecui::containers::tab_pane::add(ram_pane);
	ram_tab_pane.tab_side(lecui::containers::tab_pane::side::top);
	ram_tab_pane.rect()
		.left(0.f)
		.right(ram_pane.size().get_width())
		.top(ram_summary.rect().bottom() + _margin)
		.bottom(ram_pane.size().get_height());
	ram_tab_pane.color_tabs().alpha(0);
	ram_tab_pane.color_tabs_border().alpha(0);

	// add as many tab panes as there are rams
	int ram_number = 0;
	for (const auto& ram : _ram.ram_chips) {
		auto& ram_pane = lecui::containers::tab::add(ram_tab_pane, "RAM " + std::to_string(ram_number));

		// add ram part number
		auto& ram_part_number_caption = lecui::widgets::label::add(ram_pane);
		ram_part_number_caption
			.text("Part Number")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect({ 0.f, ram_pane.size().get_width(), 0.f, caption_height });

		auto& ram_part_number = lecui::widgets::label::add(ram_pane);
		ram_part_number
			.text(ram.part_number)
			.font_size(_detail_font_size)
			.rect(ram_part_number_caption.rect())
			.rect().height(detail_height).snap_to(ram_part_number_caption.rect(), snap_type::bottom, 0.f);

		// add ram manufacturer
		auto& manufacturer_caption = lecui::widgets::label::add(ram_pane);
		manufacturer_caption
			.text("Manufacturer")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(ram_part_number_caption.rect())
			.rect().width(ram_pane.size().get_width()).snap_to(ram_part_number.rect(), snap_type::bottom, _margin);

		auto& manufacturer = lecui::widgets::label::add(ram_pane);
		manufacturer
			.text(ram.manufacturer)
			.font_size(_detail_font_size)
			.rect(manufacturer_caption.rect())
			.rect().height(detail_height).snap_to(manufacturer_caption.rect(), snap_type::bottom, 0.f);

		// add ram chip status
		auto& status_caption = lecui::widgets::label::add(ram_pane);
		status_caption
			.text("Status")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(ram_part_number_caption.rect())
			.rect().width(ram_pane.size().get_width() / 3.f).snap_to(manufacturer.rect(), snap_type::bottom_left, _margin);

		auto& status = lecui::widgets::label::add(ram_pane);
		status
			.text(ram.status)
			.font_size(_detail_font_size)
			.rect(status_caption.rect())
			.rect().height(detail_height).snap_to(status_caption.rect(), snap_type::bottom_left, 0.f);

		if (ram.status == "OK")
			status.color_text(_ok_color);

		// add ram type
		auto& type_caption = lecui::widgets::label::add(ram_pane);
		type_caption
			.text("Type")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(status_caption.rect())
			.rect().snap_to(status_caption.rect(), snap_type::right, 0.f);

		auto& type = lecui::widgets::label::add(ram_pane);
		type
			.text(ram.type)
			.font_size(_detail_font_size)
			.rect(type_caption.rect())
			.rect().height(detail_height)
			.snap_to(type_caption.rect(), snap_type::bottom, 0.f);

		// add ram form factor
		auto& form_factor_caption = lecui::widgets::label::add(ram_pane);
		form_factor_caption
			.text("Form Factor")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(type_caption.rect())
			.rect().snap_to(type_caption.rect(), snap_type::right, 0.f);

		auto& form_factor = lecui::widgets::label::add(ram_pane);
		form_factor
			.text(ram.form_factor)
			.font_size(_detail_font_size)
			.rect(form_factor_caption.rect())
			.rect().height(detail_height).snap_to(form_factor_caption.rect(), snap_type::bottom, 0.f);

		// add capacity and speed
		auto& additional = lecui::widgets::label::add(ram_pane);
		additional
			.text(leccore::format_size(ram.capacity) + " " +
				"<span style = 'font-size: 8.0pt;'>capacity</span>, " +
				std::to_string(ram.speed) + "MHz " +
				"<span style = 'font-size: 8.0pt;'>speed</span>")
			.font_size(_highlight_font_size)
			.rect(ram_part_number.rect())
			.rect().height(highlight_height).snap_to(type.rect(), snap_type::bottom, _margin);

		ram_number++;
	}

	ram_tab_pane.selected("RAM 0");
}

void main_form::add_drive_pane() {
	auto& home = get_page("home");
	auto& ram_pane = get_pane("home/ram_pane");

	auto& drive_pane = lecui::containers::pane::add(home, "drive_pane");
	drive_pane
		.rect(ram_pane.rect())
		.rect().height(255.f).snap_to(ram_pane.rect(), snap_type::bottom, _margin);

	drive_pane.events().mouse_enter = [&]() {
		try {
			auto& copy = get_image_view("home/drive_pane/copy");
			copy.opacity(50.f);
		}
		catch (const std::exception&) {}
	};

	drive_pane.events().mouse_leave = [&]() {
		try {
			auto& copy = get_image_view("home/drive_pane/copy");
			copy.opacity(0.f);
		}
		catch (const std::exception&) {}
	};

	// add drive title
	auto& drive_title = lecui::widgets::label::add(drive_pane, "drive_title");
	drive_title.text("<strong>DRIVE DETAILS</strong>")
		.font_size(_title_font_size)
		.rect({ 0.f, drive_pane.size().get_width(), 0.f, title_height });

	// add copy details icon
	auto& copy = lecui::widgets::image_view::add(drive_pane, "copy");
	copy
		.png_resource(get_dpi_scale() < 2.f ? png_copy_32 : png_copy_64)
		.tooltip("Copy Drive Details")
		.rect()
		.left(drive_pane.size().get_width() - 24.f).width(24.f)
		.height(24.f);

	copy
		.opacity(0.f)	// invisible by default
		.color_hot().alpha(0);

	copy
		.color_selected().alpha(0);

	copy.events().mouse_enter = [&]() {
		try {
			auto& copy = get_image_view("home/drive_pane/copy");
			copy.opacity(100.f);
		}
		catch (const std::exception&) {}
	};

	copy.events().mouse_leave = [&]() {
		try {
			auto& copy = get_image_view("home/drive_pane/copy");
			copy.opacity(50.f);
		}
		catch (const std::exception&) {}
	};

	copy.events().action = [&]() {
		std::string error;
		if (!leccore::clipboard::set_text(drive_details_text(), error))
			message(error);
		else
			message("Drive details copied to the clipboard.");
	};
}

void main_form::add_drive_tab_pane() {
	auto& drive_pane = get_pane("home/drive_pane");
	auto& drive_title = get_label("home/drive_pane/drive_title");

	auto& drive_tab_pane = lecui::containers::tab_pane::add(drive_pane, "drive_tab_pane");
	drive_tab_pane
		.rect({ 0.f, drive_pane.size().get_width(), drive_title.rect().bottom(), drive_pane.size().get_height() })
		.tab_side(lecui::containers::tab_pane::side::top);
	drive_tab_pane.color_tabs().alpha(0);
	drive_tab_pane.color_tabs_border().alpha(0);

	// add as many tab panes as there are drives
	int drive_number = 0;
	for (const auto& drive : _drives) {
		auto& drive_pane = lecui::containers::tab::add(drive_tab_pane, "Drive " + std::to_string(drive_number));

		// add drive model
		auto& drive_model_caption = lecui::widgets::label::add(drive_pane);
		drive_model_caption
			.text("Model")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect({ 0.f, drive_pane.size().get_width(), 0.f, caption_height });

		auto& drive_model = lecui::widgets::label::add(drive_pane);
		drive_model
			.text(drive.model)
			.font_size(_detail_font_size)
			.rect(drive_model_caption.rect())
			.rect().height(detail_height).snap_to(drive_model_caption.rect(), snap_type::bottom, 0.f);

		// add drive status
		auto& status_caption = lecui::widgets::label::add(drive_pane);
		status_caption
			.text("Status")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(drive_model_caption.rect())
			.rect().width(drive_pane.size().get_width() / 3.f).snap_to(drive_model.rect(), snap_type::bottom_left, _margin);

		auto& status = lecui::widgets::label::add(drive_pane, "status");
		status
			.text(drive.status)
			.font_size(_detail_font_size)
			.rect(status_caption.rect())
			.rect().height(detail_height).snap_to(status_caption.rect(), snap_type::bottom, 0.f);

		if (drive.status == "OK")
			status.color_text(_ok_color);

		// add storage type
		auto& storage_type_caption = lecui::widgets::label::add(drive_pane);
		storage_type_caption
			.text("Storage Type")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(status_caption.rect())
			.rect().snap_to(status_caption.rect(), snap_type::right, 0.f);

		auto& storage_type = lecui::widgets::label::add(drive_pane, "storage_type");
		storage_type
			.text(drive.storage_type)
			.font_size(_detail_font_size)
			.rect(status.rect())
			.rect().snap_to(status.rect(), snap_type::right, 0.f);

		// add bus type
		auto& bus_type_caption = lecui::widgets::label::add(drive_pane);
		bus_type_caption
			.text("Bus Type")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(storage_type_caption.rect())
			.rect().snap_to(storage_type_caption.rect(), snap_type::right, 0.f);

		auto& bus_type = lecui::widgets::label::add(drive_pane, "bus_type");
		bus_type
			.text(drive.bus_type)
			.font_size(_detail_font_size)
			.rect(storage_type.rect())
			.rect().snap_to(storage_type.rect(), snap_type::right, 0.f);

		// add drive serial number
		auto& serial_number_caption = lecui::widgets::label::add(drive_pane);
		serial_number_caption
			.text("Serial Number")
			.color_text(_caption_color)
			.font_size(_caption_font_size)
			.rect(drive_model_caption.rect())
			.rect().snap_to(status.rect(), snap_type::bottom_left, _margin);

		auto& serial_number = lecui::widgets::label::add(drive_pane);
		serial_number
			.text(drive.serial_number)
			.font_size(_detail_font_size)
			.rect(serial_number_caption.rect())
			.rect().height(detail_height).snap_to(serial_number_caption.rect(), snap_type::bottom, 0.f);

		// add capacity
		auto& capacity = lecui::widgets::label::add(drive_pane);
		capacity
			.text(leccore::format_size(drive.size) + " " + "<span style = 'font-size: 9.0pt;'>capacity</span>")
			.font_size(_highlight_font_size)
			.rect(serial_number.rect())
			.rect().height(highlight_height).snap_to(serial_number.rect(), snap_type::bottom, _margin);

		// add media type
		auto& additional = lecui::widgets::label::add(drive_pane);
		additional
			.text(drive.media_type)
			.font_size(_caption_font_size)
			.rect(capacity.rect())
			.rect().height(caption_height).snap_to(capacity.rect(), snap_type::bottom, 0.f);

		drive_number++;
	}

	drive_tab_pane.selected("Drive 0");
}
