/*
** MIT License
**
** Copyright(c) 2021 Alec Musasa
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this softwareand associated documentation files(the "Software"), to deal
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

#include "version_info.h"
#include <liblec/lecui/containers/page.h>
#include <liblec/lecui/containers/pane.h>
#include <liblec/lecui/containers/tab_pane.h>
#include <liblec/lecui/appearance.h>
#include <liblec/lecui/controls.h>
#include <liblec/lecui/instance.h>
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/progress_bar.h>
#include <liblec/lecui/widgets/progress_indicator.h>
#include <liblec/lecui/timer.h>
#include <liblec/leccore/pc_info.h>
using namespace liblec;
using snap_type = lecui::rect::snap_type;

class dashboard : public lecui::form {
	const float margin_ = 10.f;
	const float title_font_size_ = 12.f;
	const float highlight_font_size_ = 14.f;
	const float detail_font_size_ = 10.f;
	const float caption_font_size_ = 8.f;
	const std::string sample_text_ = "<u><strong>Aq</strong></u>";
	const std::string font_ = "Segoe UI";
	const lecui::color caption_color_{ 100, 100, 100 };
	const lecui::color ok_color_{ 0, 150, 0 };
	const lecui::color not_ok_color_{ 200, 0, 0 };
	const unsigned long refresh_interval_ = 3000;
	lecui::controls ctrls_{ *this };
	lecui::page_management page_man_{ *this };
	lecui::appearance apprnc_{ *this };
	lecui::dimensions dim_{ *this };
	lecui::instance_management instance_man_{ *this, "{F7660410-F00A-4BD0-B4B5-2A76F29D03E0}" };
	lecui::timer_management timer_man_{ *this };

	leccore::pc_info pc_info_;
	leccore::pc_info::pc_details pc_details_;
	std::vector<leccore::pc_info::cpu_info> cpus_;
	std::vector<leccore::pc_info::gpu_info> gpus_;
	leccore::pc_info::ram_info ram_;
	std::vector<leccore::pc_info::drive_info> drives_;
	leccore::pc_info::power_info power_;

	float title_height;
	float highlight_height;
	float detail_height;
	float caption_height;

	void on_start() override {
		start_refresh_timer();
	}

	void start_refresh_timer() {
		timer_man_.add("refresh", refresh_interval_, [&]() { on_refresh(); });
	}

	void stop_refresh_timer() {
		timer_man_.stop("refresh");
	}

public:
	dashboard(const std::string& caption) :
		form(caption) {
		std::string error;

		// read pc details
		if (!pc_info_.pc(pc_details_, error)) {}

		// read power info
		if (!pc_info_.power(power_, error)) {}

		// read cpu info
		if (!pc_info_.cpu(cpus_, error)) {}

		// read gpu info
		if (!pc_info_.gpu(gpus_, error)) {}

		// read memory into
		if (!pc_info_.ram(ram_, error)) {}

		// read drive info
		if (!pc_info_.drives(drives_, error)) {}
	}

private:
	bool on_layout(std::string& error) override {
		ctrls_.resize(false);
		apprnc_.theme(lecui::themes::light);
		dim_.size({ 1120, 570 });

		auto& home = page_man_.add("home");

		// compute label heights
		const lecui::rect page_rect = { 0.f, home.size().width, 0.f, home.size().height };
		title_height = dim_.measure_label(sample_text_, font_, title_font_size_, true, false, page_rect).height();
		highlight_height = dim_.measure_label(sample_text_, font_, highlight_font_size_, true, false, page_rect).height();
		detail_height = dim_.measure_label(sample_text_, font_, detail_font_size_, true, false, page_rect).height();
		caption_height = dim_.measure_label(sample_text_, font_, caption_font_size_, true, false, page_rect).height();

		//////////////////////////////////////////////////
		// 1. Add pane for pc details
		lecui::containers::pane pc_details_pane(home, "pc_details_pane");
		pc_details_pane().rect = { margin_, margin_ + 200.f, margin_, home.size().height - margin_ };

		// add pc details title
		lecui::widgets::label pc_details_title(pc_details_pane.get());
		pc_details_title().rect = { 0.f, pc_details_pane.get().size().width,
			0.f, title_height };
		pc_details_title().font_size = title_font_size_;
		pc_details_title().text = "<strong>PC DETAILS</strong>";

		// add pc name
		lecui::widgets::label pc_name_caption(pc_details_pane.get());
		pc_name_caption().rect = pc_details_title().rect;
		pc_name_caption().rect.height(caption_height);
		pc_name_caption().rect.snap_to(pc_details_title().rect, snap_type::bottom, margin_);
		pc_name_caption().color_text = caption_color_;
		pc_name_caption().font_size = caption_font_size_;
		pc_name_caption().text = "Name";

		lecui::widgets::label pc_name(pc_details_pane.get());
		pc_name().rect = pc_details_title().rect;
		pc_name().rect.height(highlight_height);
		pc_name().rect.snap_to(pc_name_caption().rect, snap_type::bottom, 0.f);
		pc_name().font_size = highlight_font_size_;
		pc_name().text = pc_details_.name;

		// add pc manufacturer
		lecui::widgets::label manufacturer_caption(pc_details_pane.get());
		manufacturer_caption().rect = pc_name_caption().rect;
		manufacturer_caption().rect.snap_to(pc_name().rect, snap_type::bottom, margin_);
		manufacturer_caption().color_text = caption_color_;
		manufacturer_caption().font_size = caption_font_size_;
		manufacturer_caption().text = "Manufacturer";

		lecui::widgets::label manufacturer(pc_details_pane.get());
		manufacturer().rect = manufacturer_caption().rect;
		manufacturer().rect.height(detail_height);
		manufacturer().rect.snap_to(manufacturer_caption().rect, snap_type::bottom, 0.f);
		manufacturer().font_size = detail_font_size_;
		manufacturer().text = pc_details_.manufacturer;

		// add model
		lecui::widgets::label model_caption(pc_details_pane.get());
		model_caption().rect = pc_name_caption().rect;
		model_caption().rect.snap_to(manufacturer().rect, snap_type::bottom, margin_);
		model_caption().color_text = caption_color_;
		model_caption().font_size = caption_font_size_;
		model_caption().text = "Model";

		lecui::widgets::label model(pc_details_pane.get());
		model().rect = manufacturer().rect;
		model().rect.snap_to(model_caption().rect, snap_type::bottom, 0.f);
		model().font_size = detail_font_size_;
		model().text = pc_details_.model;

		// add system type
		lecui::widgets::label type_caption(pc_details_pane.get());
		type_caption().rect = pc_name_caption().rect;
		type_caption().rect.snap_to(model().rect, snap_type::bottom, margin_);
		type_caption().color_text = caption_color_;
		type_caption().font_size = caption_font_size_;
		type_caption().text = "System type";

		lecui::widgets::label type(pc_details_pane.get());
		type().rect = model().rect;
		type().rect.snap_to(type_caption().rect, snap_type::bottom, 0.f);
		type().font_size = detail_font_size_;
		type().text = pc_details_.system_type;

		// add bios serial number
		lecui::widgets::label bios_sn_caption(pc_details_pane.get());
		bios_sn_caption().rect = pc_name_caption().rect;
		bios_sn_caption().rect.snap_to(type().rect, snap_type::bottom, margin_);
		bios_sn_caption().color_text = caption_color_;
		bios_sn_caption().font_size = caption_font_size_;
		bios_sn_caption().text = "BIOS Serial Number";

		lecui::widgets::label bios_sn(pc_details_pane.get());
		bios_sn().rect = type().rect;
		bios_sn().rect.snap_to(bios_sn_caption().rect, snap_type::bottom, 0.f);
		bios_sn().font_size = detail_font_size_;
		bios_sn().text = pc_details_.bios_serial_number;

		// add board serial number
		lecui::widgets::label board_sn_caption(pc_details_pane.get());
		board_sn_caption().rect = pc_name_caption().rect;
		board_sn_caption().rect.snap_to(bios_sn().rect, snap_type::bottom, margin_);
		board_sn_caption().color_text = caption_color_;
		board_sn_caption().font_size = caption_font_size_;
		board_sn_caption().text = "Motherboard Serial Number";

		lecui::widgets::label board_sn(pc_details_pane.get());
		board_sn().rect = bios_sn().rect;
		board_sn().rect.snap_to(board_sn_caption().rect, snap_type::bottom, 0.f);
		board_sn().font_size = detail_font_size_;
		board_sn().text = pc_details_.motherboard_serial_number;

		// add hardware summary details
		if (true) {
			lecui::widgets::label cpu_summary(pc_details_pane.get());
			cpu_summary().rect = pc_name().rect;
			cpu_summary().rect.height(highlight_height);
			cpu_summary().rect.snap_to(board_sn().rect, snap_type::bottom, 2 * margin_);
			cpu_summary().font_size = highlight_font_size_;
			cpu_summary().text = std::to_string(cpus_.size()) + "<span style = 'font-size: 8.0pt;'>" +
				std::string(cpus_.size() == 1 ? " CPU" : " CPUs") +
				"</span>";

			lecui::widgets::label gpu_summary(pc_details_pane.get());
			gpu_summary().rect = pc_name().rect;
			gpu_summary().rect.height(highlight_height);
			gpu_summary().rect.snap_to(cpu_summary().rect, snap_type::bottom, 0.f);
			gpu_summary().font_size = highlight_font_size_;
			gpu_summary().text = std::to_string(gpus_.size()) + "<span style = 'font-size: 8.0pt;'>" +
				std::string(gpus_.size() == 1 ? " GPU" : " GPUs") +
				"</span>";

			lecui::widgets::label ram_summary(pc_details_pane.get());
			ram_summary().rect = pc_name().rect;
			ram_summary().rect.height(highlight_height);
			ram_summary().rect.snap_to(gpu_summary().rect, snap_type::bottom, 0.f);
			ram_summary().font_size = highlight_font_size_;
			ram_summary().text = std::to_string(ram_.ram_chips.size()) + "<span style = 'font-size: 8.0pt;'>" +
				std::string(ram_.ram_chips.size() == 1 ? " RAM chip" : " RAM chips") +
				"</span>";

			lecui::widgets::label drive_summary(pc_details_pane.get(), "drive_summary");
			drive_summary().rect = pc_name().rect;
			drive_summary().rect.height(highlight_height);
			drive_summary().rect.snap_to(ram_summary().rect, snap_type::bottom, 0.f);
			drive_summary().font_size = highlight_font_size_;
			drive_summary().text = std::to_string(drives_.size()) + "<span style = 'font-size: 8.0pt;'>" +
				std::string(drives_.size() == 1 ? " drive" : " drives") +
				"</span>";

			lecui::widgets::label battery_summary(pc_details_pane.get(), "battery_summary");
			battery_summary().rect = pc_name().rect;
			battery_summary().rect.height(highlight_height);
			battery_summary().rect.snap_to(drive_summary().rect, snap_type::bottom, 0.f);
			battery_summary().font_size = highlight_font_size_;
			battery_summary().text = std::to_string(power_.batteries.size()) + "<span style = 'font-size: 8.0pt;'>" +
				std::string(power_.batteries.size() == 1 ? " battery" : " batteries") +
				"</span>";
		}

		//////////////////////////////////////////////////
		// 2. Add pane for power details
		lecui::containers::pane power_pane(home, "power_pane");
		power_pane().rect = pc_details_pane().rect;
		power_pane().rect.width(270.f);
		power_pane().rect.snap_to(pc_details_pane().rect, snap_type::right_top, margin_);

		// add pc details title
		lecui::widgets::label power_details_title(power_pane.get());
		power_details_title().rect = { 0.f, power_pane.get().size().width,
			0.f, title_height };
		power_details_title().font_size = title_font_size_;
		power_details_title().text = "<strong>POWER DETAILS</strong>";

		// add power status
		lecui::widgets::label power_status_caption(power_pane.get());
		power_status_caption().rect = power_details_title().rect;
		power_status_caption().rect.height(caption_height);
		power_status_caption().rect.snap_to(power_details_title().rect, snap_type::bottom, margin_);
		power_status_caption().color_text = caption_color_;
		power_status_caption().font_size = caption_font_size_;
		power_status_caption().text = "Status";

		lecui::widgets::label power_status(power_pane.get(), "power_status");
		power_status().rect = power_details_title().rect;
		power_status().rect.height(highlight_height);
		power_status().rect.snap_to(power_status_caption().rect, snap_type::bottom, 0.f);
		power_status().font_size = highlight_font_size_;

		power_status().text = power_.ac ? "On AC" : "On Battery";
		power_status().text += ", ";
		power_status().text += ("<span style = 'font-size: 8.0pt;'>" +
			pc_info_.to_string(power_.status) + "</span>");

		// add power level
		lecui::widgets::label level(power_pane.get(), "level");
		level().rect = power_status_caption().rect;
		level().rect.height(detail_height);
		level().rect.snap_to(power_status().rect, snap_type::bottom, margin_);
		level().font_size = detail_font_size_;
		level().text = (power_.level != -1 ?
			(std::to_string(power_.level) + "% ") : std::string("<em>Unknown</em> ")) +
			"<span style = 'font-size: 8.0pt;'>overall power level</span>";

		lecui::widgets::progress_bar level_bar(power_pane.get(), "level_bar");
		level_bar().rect.width(power_status().rect.width());
		level_bar().rect.snap_to(level().rect, snap_type::bottom, margin_ / 2.f);
		level_bar().percentage = static_cast<float>(power_.level);

		// add life remaining label
		lecui::widgets::label life_remaining(power_pane.get(), "life_remaining");
		life_remaining().rect = level_bar().rect;
		life_remaining().rect.height(caption_height);
		life_remaining().rect.snap_to(level_bar().rect, snap_type::bottom, margin_ / 2.f);
		life_remaining().font_size = caption_font_size_;
		life_remaining().color_text = caption_color_;
		life_remaining().text = power_.lifetime_remaining.empty() ? std::string() :
			(power_.lifetime_remaining + " remaining");

		add_battery_pane(power_pane.get(), life_remaining().rect.bottom);

		//////////////////////////////////////////////////
		// 2. Add pane for cpu details
		lecui::containers::pane cpu_pane(home);
		cpu_pane().rect.left = power_pane().rect.right + margin_;
		cpu_pane().rect.width(300.f);
		cpu_pane().rect.top = margin_;
		cpu_pane().rect.height(240.f);

		// add cpu title
		lecui::widgets::label cpu_title(cpu_pane.get());
		cpu_title().rect = { 0.f, cpu_pane.get().size().width,
			0.f, title_height };
		cpu_title().font_size = title_font_size_;
		cpu_title().text = "<strong>CPU DETAILS</strong>";

		lecui::containers::tab_pane cpu_tab_pane(cpu_pane.get());
		cpu_tab_pane().rect.left = 0.f;
		cpu_tab_pane().rect.right = cpu_pane.get().size().width;
		cpu_tab_pane().rect.top = cpu_title().rect.bottom;
		cpu_tab_pane().rect.bottom = cpu_pane.get().size().height;
		cpu_tab_pane().tab_side = lecui::containers::tab_pane::side::top;
		cpu_tab_pane().color_tabs.alpha = 0;
		cpu_tab_pane().color_tabs_border.alpha = 0;
		
		// add as many tab panes as there are cpus
		int cpu_number = 0;
		for (const auto& cpu : cpus_) {
			lecui::containers::tab cpu_pane(cpu_tab_pane, "CPU " + std::to_string(cpu_number));

			// add cpu name
			lecui::widgets::label cpu_name_caption(cpu_pane.get());
			cpu_name_caption().rect = { 0.f, cpu_pane.get().size().width, 0.f, caption_height };
			cpu_name_caption().color_text = caption_color_;
			cpu_name_caption().font_size = caption_font_size_;
			cpu_name_caption().text = "Name";

			lecui::widgets::label cpu_name(cpu_pane.get());
			cpu_name().rect = cpu_name_caption().rect;
			cpu_name().rect.height(detail_height);
			cpu_name().rect.snap_to(cpu_name_caption().rect, snap_type::bottom, 0.f);
			cpu_name().font_size = detail_font_size_;
			cpu_name().text = cpu.name;

			// add cpu status
			lecui::widgets::label status_caption(cpu_pane.get());
			status_caption().rect = cpu_name_caption().rect;
			status_caption().rect.snap_to(cpu_name().rect, snap_type::bottom, margin_);
			status_caption().color_text = caption_color_;
			status_caption().font_size = caption_font_size_;
			status_caption().text = "Status";

			lecui::widgets::label status(cpu_pane.get());
			status().rect = status_caption().rect;
			status().rect.height(detail_height);
			status().rect.snap_to(status_caption().rect, snap_type::bottom, 0.f);
			status().font_size = detail_font_size_;
			status().text = cpu.status;
			if (cpu.status == "OK")
				status().color_text = ok_color_;

			// add base speed
			lecui::widgets::label base_speed(cpu_pane.get());
			base_speed().rect = status().rect;
			base_speed().rect.height(highlight_height);
			base_speed().rect.snap_to(status().rect, snap_type::bottom, margin_);
			base_speed().font_size = highlight_font_size_;
			base_speed().text = leccore::round_off::tostr<char>(cpu.base_speed, 2) + "GHz "
				"<span style = 'font-size: 8.0pt;'>base speed</span>";

			// add cpu cores
			lecui::widgets::label cores(cpu_pane.get());
			cores().rect = base_speed().rect;
			cores().rect.height(highlight_height);
			cores().rect.snap_to(base_speed().rect, snap_type::bottom, margin_);
			cores().font_size = highlight_font_size_;
			cores().text = std::to_string(cpu.cores) +
				"<span style = 'font-size: 8.0pt;'>" +
				std::string(cpu.cores == 1 ? " core" : " cores") +
				"</span>, " +
				std::to_string(cpu.logical_processors) +
				"<span style = 'font-size: 8.0pt;'>" +
				std::string(cpu.cores == 1 ? " logical processor" : " logical processors") +
				"</span>";

			cpu_number++;
		}

		cpu_tab_pane.select("CPU 0");

		//////////////////////////////////////////////////
		// 3. Add pane for gpu details
		lecui::containers::pane gpu_pane(home);
		gpu_pane().rect.left = cpu_pane().rect.right + margin_;
		gpu_pane().rect.width(300.f);
		gpu_pane().rect.top = margin_;
		gpu_pane().rect.height(240.f);

		// add gpu title
		lecui::widgets::label gpu_title(gpu_pane.get());
		gpu_title().rect = { 0.f, gpu_pane.get().size().width,
			0.f, title_height };
		gpu_title().font_size = title_font_size_;
		gpu_title().text = "<strong>GPU DETAILS</strong>";

		lecui::containers::tab_pane gpu_tab_pane(gpu_pane.get());
		gpu_tab_pane().rect.left = 0.f;
		gpu_tab_pane().rect.right = gpu_pane.get().size().width;
		gpu_tab_pane().rect.top = gpu_title().rect.bottom;
		gpu_tab_pane().rect.bottom = gpu_pane.get().size().height;
		gpu_tab_pane().tab_side = lecui::containers::tab_pane::side::top;
		gpu_tab_pane().color_tabs.alpha = 0;
		gpu_tab_pane().color_tabs_border.alpha = 0;

		// add as many tab panes as there are gpus
		int gpu_number = 0;
		for (const auto& gpu : gpus_) {
			lecui::containers::tab gpu_pane(gpu_tab_pane, "GPU " + std::to_string(gpu_number));

			// add gpu name
			lecui::widgets::label gpu_name_caption(gpu_pane.get());
			gpu_name_caption().rect = { 0.f, gpu_pane.get().size().width, 0.f, caption_height };
			gpu_name_caption().color_text = caption_color_;
			gpu_name_caption().font_size = caption_font_size_;
			gpu_name_caption().text = "Name";

			lecui::widgets::label gpu_name(gpu_pane.get());
			gpu_name().rect = gpu_name_caption().rect;
			gpu_name().rect.height(detail_height);
			gpu_name().rect.snap_to(gpu_name_caption().rect, snap_type::bottom, 0.f);
			gpu_name().font_size = detail_font_size_;
			gpu_name().text = gpu.name;

			// add gpu status
			lecui::widgets::label status_caption(gpu_pane.get());
			status_caption().rect = gpu_name_caption().rect;
			status_caption().rect.snap_to(gpu_name().rect, snap_type::bottom, margin_);
			status_caption().color_text = caption_color_;
			status_caption().font_size = caption_font_size_;
			status_caption().text = "Status";

			lecui::widgets::label status(gpu_pane.get());
			status().rect = status_caption().rect;
			status().rect.height(detail_height);
			status().rect.snap_to(status_caption().rect, snap_type::bottom, 0.f);
			status().font_size = detail_font_size_;
			status().text = gpu.status;
			if (gpu.status == "OK")
				status().color_text = ok_color_;

			// add gpu resolution
			lecui::widgets::label resolution_caption(gpu_pane.get());
			resolution_caption().rect = gpu_name_caption().rect;
			resolution_caption().rect.snap_to(status().rect, snap_type::bottom, margin_);
			resolution_caption().color_text = caption_color_;
			resolution_caption().font_size = caption_font_size_;
			resolution_caption().text = "Resolution";

			lecui::widgets::label resolution(gpu_pane.get());
			resolution().rect = resolution_caption().rect;
			resolution().rect.height(detail_height);
			resolution().rect.snap_to(resolution_caption().rect, snap_type::bottom, 0.f);
			resolution().font_size = detail_font_size_;
			resolution().text = std::to_string(gpu.horizontal_resolution) + "x" +
				std::to_string(gpu.vertical_resolution);

			// add refresh rate and memory
			lecui::widgets::label additional(gpu_pane.get());
			additional().rect = resolution().rect;
			additional().rect.height(highlight_height);
			additional().rect.snap_to(resolution().rect, snap_type::bottom, margin_);
			additional().font_size = highlight_font_size_;
			additional().text = std::to_string(gpu.refresh_rate) + "Hz " +
				"<span style = 'font-size: 8.0pt;'>refresh rate</span>, " +
				leccore::format_size(gpu.ram) + " " +
				"<span style = 'font-size: 8.0pt;'>graphics memory</span>";

			gpu_number++;
		}

		gpu_tab_pane.select("GPU 0");

		//////////////////////////////////////////////////
		// 4. Add pane for ram details
		lecui::containers::pane ram_pane(home);
		ram_pane().rect.left = cpu_pane().rect.left;
		ram_pane().rect.width(300.f);
		ram_pane().rect.top = cpu_pane().rect.bottom + margin_;
		ram_pane().rect.height(270.f);

		// add ram title
		lecui::widgets::label ram_title(ram_pane.get());
		ram_title().rect = { 0.f, ram_pane.get().size().width,
			0.f, title_height };
		ram_title().font_size = title_font_size_;
		ram_title().text = "<strong>RAM DETAILS</strong>";

		// add ram summary
		lecui::widgets::label ram_summary(ram_pane.get());
		ram_summary().rect = ram_title().rect;
		ram_summary().rect.snap_to(ram_title().rect, snap_type::bottom, 0.f);
		ram_summary().rect.height(highlight_height);
		ram_summary().font_size = highlight_font_size_;
		ram_summary().text = leccore::format_size(ram_.size) + " " +
			"<span style = 'font-size: 8.0pt;'>capacity</span>, " +
			std::to_string(ram_.speed) + "MHz " +
			"<span style = 'font-size: 8.0pt;'>speed</span>";

		lecui::containers::tab_pane ram_tab_pane(ram_pane.get());
		ram_tab_pane().rect.left = 0.f;
		ram_tab_pane().rect.right = ram_pane.get().size().width;
		ram_tab_pane().rect.top = ram_summary().rect.bottom;
		ram_tab_pane().rect.bottom = ram_pane.get().size().height;
		ram_tab_pane().tab_side = lecui::containers::tab_pane::side::top;
		ram_tab_pane().color_tabs.alpha = 0;
		ram_tab_pane().color_tabs_border.alpha = 0;

		// add as many tab panes as there are rams
		int ram_number = 0;
		for (const auto& ram : ram_.ram_chips) {
			lecui::containers::tab ram_pane(ram_tab_pane, "RAM " + std::to_string(ram_number));

			// add ram part number
			lecui::widgets::label ram_part_number_caption(ram_pane.get());
			ram_part_number_caption().rect = { 0.f, ram_pane.get().size().width, 0.f, caption_height };
			ram_part_number_caption().color_text = caption_color_;
			ram_part_number_caption().font_size = caption_font_size_;
			ram_part_number_caption().text = "Part Number";

			lecui::widgets::label ram_part_number(ram_pane.get());
			ram_part_number().rect = ram_part_number_caption().rect;
			ram_part_number().rect.height(detail_height);
			ram_part_number().rect.snap_to(ram_part_number_caption().rect, snap_type::bottom, 0.f);
			ram_part_number().font_size = detail_font_size_;
			ram_part_number().text = ram.part_number;

			// add ram manufacturer
			lecui::widgets::label manufacturer_caption(ram_pane.get());
			manufacturer_caption().rect = ram_part_number_caption().rect;
			manufacturer_caption().rect.width(ram_pane.get().size().width);
			manufacturer_caption().rect.snap_to(ram_part_number().rect, snap_type::bottom, margin_);
			manufacturer_caption().color_text = caption_color_;
			manufacturer_caption().font_size = caption_font_size_;
			manufacturer_caption().text = "Manufacturer";

			lecui::widgets::label manufacturer(ram_pane.get());
			manufacturer().rect = manufacturer_caption().rect;
			manufacturer().rect.height(detail_height);
			manufacturer().rect.snap_to(manufacturer_caption().rect, snap_type::bottom, 0.f);
			manufacturer().font_size = detail_font_size_;
			manufacturer().text = ram.manufacturer;

			// add ram type
			lecui::widgets::label type_caption(ram_pane.get());
			type_caption().rect = ram_part_number_caption().rect;
			type_caption().rect.width(ram_pane.get().size().width / 2.f);
			type_caption().rect.snap_to(manufacturer().rect, snap_type::bottom_left, margin_);
			type_caption().color_text = caption_color_;
			type_caption().font_size = caption_font_size_;
			type_caption().text = "Type";

			lecui::widgets::label type(ram_pane.get());
			type().rect = type_caption().rect;
			type().rect.height(detail_height);
			type().rect.snap_to(type_caption().rect, snap_type::bottom, 0.f);
			type().font_size = detail_font_size_;
			type().text = ram.type;

			// add ram form factor
			lecui::widgets::label form_factor_caption(ram_pane.get());
			form_factor_caption().rect = type_caption().rect;
			form_factor_caption().rect.snap_to(type_caption().rect, snap_type::right, 0.f);
			form_factor_caption().color_text = caption_color_;
			form_factor_caption().font_size = caption_font_size_;
			form_factor_caption().text = "Form Factor";

			lecui::widgets::label form_factor(ram_pane.get());
			form_factor().rect = form_factor_caption().rect;
			form_factor().rect.height(detail_height);
			form_factor().rect.snap_to(form_factor_caption().rect, snap_type::bottom, 0.f);
			form_factor().font_size = detail_font_size_;
			form_factor().text = ram.form_factor;

			// add capacity and speed
			lecui::widgets::label additional(ram_pane.get());
			additional().rect = ram_part_number().rect;
			additional().rect.height(highlight_height);
			additional().rect.snap_to(type().rect, snap_type::bottom, margin_);
			additional().font_size = highlight_font_size_;
			additional().text = leccore::format_size(ram.capacity) + " " +
				"<span style = 'font-size: 8.0pt;'>capacity</span>, " +
				std::to_string(ram.speed) + "MHz " +
				"<span style = 'font-size: 8.0pt;'>speed</span>";

			ram_number++;
		}

		ram_tab_pane.select("RAM 0");

		//////////////////////////////////////////////////
		// 5. Add pane for drive details
		lecui::containers::pane drive_pane(home, "drive_pane");
		drive_pane().rect = ram_pane().rect;
		drive_pane().rect.snap_to(ram_pane().rect, snap_type::right, margin_);

		// add drive title
		lecui::widgets::label drive_title(drive_pane.get(), "drive_title");
		drive_title().rect = { 0.f, drive_pane.get().size().width,
			0.f, title_height };
		drive_title().font_size = title_font_size_;
		drive_title().text = "<strong>DRIVE DETAILS</strong>";

		// add pane for drive details
		add_drive_details_pane(drive_pane.get(), drive_title().rect.bottom);

		page_man_.show("home");
		return true;
	}

	void add_battery_pane(lecui::containers::page& power_pane, const float top) {
		// add pane for battery details
		lecui::containers::tab_pane battery_tab_pane(power_pane, "battery_tab_pane");
		battery_tab_pane().rect.left = 0.f;
		battery_tab_pane().rect.right = power_pane.size().width;
		battery_tab_pane().rect.top = top;
		battery_tab_pane().rect.bottom = power_pane.size().height;
		battery_tab_pane().tab_side = lecui::containers::tab_pane::side::top;
		battery_tab_pane().color_tabs.alpha = 0;
		battery_tab_pane().color_tabs_border.alpha = 0;

		// add as many tab panes as there are batteries
		int battery_number = 0;
		for (const auto& battery : power_.batteries) {
			lecui::containers::tab battery_pane(battery_tab_pane, "Battery " + std::to_string(battery_number));

			// add battery name
			lecui::widgets::label battery_name_caption(battery_pane.get());
			battery_name_caption().rect = { 0.f, battery_pane.get().size().width, 0.f, caption_height };
			battery_name_caption().color_text = caption_color_;
			battery_name_caption().font_size = caption_font_size_;
			battery_name_caption().text = "Name";

			lecui::widgets::label battery_name(battery_pane.get());
			battery_name().rect = battery_name_caption().rect;
			battery_name().rect.height(detail_height);
			battery_name().rect.snap_to(battery_name_caption().rect, snap_type::bottom, 0.f);
			battery_name().font_size = detail_font_size_;
			battery_name().text = battery.name;

			// add battery manufacturer
			lecui::widgets::label manufacturer_caption(battery_pane.get());
			manufacturer_caption().rect = battery_name_caption().rect;
			manufacturer_caption().rect.width(battery_pane.get().size().width);
			manufacturer_caption().rect.snap_to(battery_name().rect, snap_type::bottom, margin_);
			manufacturer_caption().color_text = caption_color_;
			manufacturer_caption().font_size = caption_font_size_;
			manufacturer_caption().text = "Manufacturer";

			lecui::widgets::label manufacturer(battery_pane.get());
			manufacturer().rect = manufacturer_caption().rect;
			manufacturer().rect.height(detail_height);
			manufacturer().rect.snap_to(manufacturer_caption().rect, snap_type::bottom, 0.f);
			manufacturer().font_size = detail_font_size_;
			manufacturer().text = battery.manufacturer;

			// add battery designed capacity
			lecui::widgets::label designed_capacity_caption(battery_pane.get());
			designed_capacity_caption().rect = manufacturer_caption().rect;
			designed_capacity_caption().rect.width(battery_pane.get().size().width / 2.f);
			designed_capacity_caption().rect.snap_to(manufacturer().rect, snap_type::bottom_left, margin_);
			designed_capacity_caption().color_text = caption_color_;
			designed_capacity_caption().font_size = caption_font_size_;
			designed_capacity_caption().text = "Designed Capacity";

			lecui::widgets::label designed_capacity(battery_pane.get(), "designed_capacity");
			designed_capacity().rect = designed_capacity_caption().rect;
			designed_capacity().rect.height(detail_height);
			designed_capacity().rect.snap_to(designed_capacity_caption().rect, snap_type::bottom, 0.f);
			designed_capacity().font_size = detail_font_size_;
			designed_capacity().text = std::to_string(battery.designed_capacity) + "mWh";

			// add battery fully charged capacity
			lecui::widgets::label fully_charged_capacity_caption(battery_pane.get());
			fully_charged_capacity_caption().rect = designed_capacity_caption().rect;
			fully_charged_capacity_caption().rect.snap_to(designed_capacity_caption().rect, snap_type::right, 0.f);
			fully_charged_capacity_caption().color_text = caption_color_;
			fully_charged_capacity_caption().font_size = caption_font_size_;
			fully_charged_capacity_caption().text = "Fully Charged Capacity";

			lecui::widgets::label fully_charged_capacity(battery_pane.get(), "fully_charged_capacity");
			fully_charged_capacity().rect = fully_charged_capacity_caption().rect;
			fully_charged_capacity().rect.height(detail_height);
			fully_charged_capacity().rect.snap_to(fully_charged_capacity_caption().rect, snap_type::bottom, 0.f);
			fully_charged_capacity().font_size = detail_font_size_;
			fully_charged_capacity().text = std::to_string(battery.fully_charged_capacity) + "mWh";

			// add battery health
			lecui::widgets::progress_indicator health(battery_pane.get(), "health");
			health().rect.snap_to(designed_capacity().rect, snap_type::bottom_left, margin_);
			health().percentage = static_cast<float>(battery.health);

			lecui::widgets::label health_caption(battery_pane.get());
			health_caption().rect = health().rect;
			health_caption().rect.right = battery_name().rect.right;
			health_caption().rect.snap_to(health().rect, snap_type::right, margin_);
			health_caption().color_text = caption_color_;
			health_caption().font_size = caption_font_size_;
			health_caption().text = "BATTERY HEALTH";
			health_caption().center_v = true;

			// add battery current capacity
			lecui::widgets::label current_capacity_caption(battery_pane.get());
			current_capacity_caption().rect = battery_name().rect;
			current_capacity_caption().rect.width(battery_pane.get().size().width / 2.f);
			current_capacity_caption().rect.snap_to(health().rect, snap_type::bottom_left, margin_);
			current_capacity_caption().color_text = caption_color_;
			current_capacity_caption().font_size = caption_font_size_;
			current_capacity_caption().text = "Current Capacity";

			lecui::widgets::label current_capacity(battery_pane.get(), "current_capacity");
			current_capacity().rect = current_capacity_caption().rect;
			current_capacity().rect.height(detail_height);
			current_capacity().rect.snap_to(current_capacity_caption().rect, snap_type::bottom, 0.f);
			current_capacity().font_size = detail_font_size_;
			current_capacity().text = std::to_string(battery.current_capacity) + "mWh";

			// add battery fully charged capacity
			lecui::widgets::label charge_level_caption(battery_pane.get());
			charge_level_caption().rect = current_capacity_caption().rect;
			charge_level_caption().rect.snap_to(current_capacity_caption().rect, snap_type::right, 0.f);
			charge_level_caption().color_text = caption_color_;
			charge_level_caption().font_size = caption_font_size_;
			charge_level_caption().text = "Charge Level";

			lecui::widgets::label charge_level(battery_pane.get(), "charge_level");
			charge_level().rect = charge_level_caption().rect;
			charge_level().rect.height(detail_height);
			charge_level().rect.snap_to(charge_level_caption().rect, snap_type::bottom, 0.f);
			charge_level().font_size = detail_font_size_;
			charge_level().text = leccore::round_off::tostr<char>(battery.level, 1) + "%";

			// add battery current voltage
			lecui::widgets::label current_voltage_caption(battery_pane.get());
			current_voltage_caption().rect = battery_name().rect;
			current_voltage_caption().rect.width(battery_pane.get().size().width / 2.f);
			current_voltage_caption().rect.snap_to(current_capacity().rect, snap_type::bottom_left, margin_);
			current_voltage_caption().color_text = caption_color_;
			current_voltage_caption().font_size = caption_font_size_;
			current_voltage_caption().text = "Current Voltage";

			lecui::widgets::label current_voltage(battery_pane.get(), "current_voltage");
			current_voltage().rect = current_voltage_caption().rect;
			current_voltage().rect.height(detail_height);
			current_voltage().rect.snap_to(current_voltage_caption().rect, snap_type::bottom, 0.f);
			current_voltage().font_size = detail_font_size_;
			current_voltage().text = std::to_string(battery.current_voltage) + "mV";

			// add battery current charge rate
			lecui::widgets::label charge_rate_caption(battery_pane.get());
			charge_rate_caption().rect = current_voltage_caption().rect;
			charge_rate_caption().rect.snap_to(current_voltage_caption().rect, snap_type::right, 0.f);
			charge_rate_caption().color_text = caption_color_;
			charge_rate_caption().font_size = caption_font_size_;
			charge_rate_caption().text = "Charge Rate";

			lecui::widgets::label charge_rate(battery_pane.get(), "charge_rate");
			charge_rate().rect = charge_rate_caption().rect;
			charge_rate().rect.height(detail_height);
			charge_rate().rect.snap_to(charge_rate_caption().rect, snap_type::bottom, 0.f);
			charge_rate().font_size = detail_font_size_;
			charge_rate().text = std::to_string(battery.current_charge_rate) + "mW";

			// add battery status
			lecui::widgets::label status_caption(battery_pane.get());
			status_caption().rect = battery_name_caption().rect;
			status_caption().rect.width(battery_pane.get().size().width);
			status_caption().rect.snap_to(current_voltage().rect, snap_type::bottom, margin_);
			status_caption().color_text = caption_color_;
			status_caption().font_size = caption_font_size_;
			status_caption().text = "Status";

			lecui::widgets::label status(battery_pane.get(), "status");
			status().rect = status_caption().rect;
			status().rect.height(detail_height);
			status().rect.snap_to(status_caption().rect, snap_type::bottom, 0.f);
			status().font_size = detail_font_size_;
			status().text = pc_info_.to_string(battery.status);

			battery_number++;
		}

		battery_tab_pane.select("Battery 0");
	}

	void add_drive_details_pane(lecui::containers::page& drive_pane, const float top) {
		lecui::containers::tab_pane drive_tab_pane(drive_pane, "drive_tab_pane");
		drive_tab_pane().rect.left = 0.f;
		drive_tab_pane().rect.right = drive_pane.size().width;
		drive_tab_pane().rect.top = top;
		drive_tab_pane().rect.bottom = drive_pane.size().height;
		drive_tab_pane().tab_side = lecui::containers::tab_pane::side::top;
		drive_tab_pane().color_tabs.alpha = 0;
		drive_tab_pane().color_tabs_border.alpha = 0;

		// add as many tab panes as there are drives
		int drive_number = 0;
		for (const auto& drive : drives_) {
			lecui::containers::tab drive_pane(drive_tab_pane, "Drive " + std::to_string(drive_number));

			// add drive model
			lecui::widgets::label drive_model_caption(drive_pane.get());
			drive_model_caption().rect = { 0.f, drive_pane.get().size().width, 0.f, caption_height };
			drive_model_caption().color_text = caption_color_;
			drive_model_caption().font_size = caption_font_size_;
			drive_model_caption().text = "Model";

			lecui::widgets::label drive_model(drive_pane.get());
			drive_model().rect = drive_model_caption().rect;
			drive_model().rect.height(detail_height);
			drive_model().rect.snap_to(drive_model_caption().rect, snap_type::bottom, 0.f);
			drive_model().font_size = detail_font_size_;
			drive_model().text = drive.model;

			// add drive status
			lecui::widgets::label status_caption(drive_pane.get());
			status_caption().rect = drive_model_caption().rect;
			status_caption().rect.width(drive_pane.get().size().width / 3.f);
			status_caption().rect.snap_to(drive_model().rect, snap_type::bottom_left, margin_);
			status_caption().color_text = caption_color_;
			status_caption().font_size = caption_font_size_;
			status_caption().text = "Status";

			lecui::widgets::label status(drive_pane.get(), "status");
			status().rect = status_caption().rect;
			status().rect.height(detail_height);
			status().rect.snap_to(status_caption().rect, snap_type::bottom, 0.f);
			status().font_size = detail_font_size_;
			status().text = drive.status;
			if (drive.status == "OK")
				status().color_text = ok_color_;

			// add storage type
			lecui::widgets::label storage_type_caption(drive_pane.get());
			storage_type_caption().rect = status_caption().rect;
			storage_type_caption().rect.snap_to(status_caption().rect, snap_type::right, 0.f);
			storage_type_caption().color_text = caption_color_;
			storage_type_caption().font_size = caption_font_size_;
			storage_type_caption().text = "Storage Type";

			lecui::widgets::label storage_type(drive_pane.get());
			storage_type().rect = status().rect;
			storage_type().rect.snap_to(status().rect, snap_type::right, 0.f);
			storage_type().font_size = detail_font_size_;
			storage_type().text = drive.storage_type;

			// add bus type
			lecui::widgets::label bus_type_caption(drive_pane.get());
			bus_type_caption().rect = storage_type_caption().rect;
			bus_type_caption().rect.snap_to(storage_type_caption().rect, snap_type::right, 0.f);
			bus_type_caption().color_text = caption_color_;
			bus_type_caption().font_size = caption_font_size_;
			bus_type_caption().text = "Bus Type";

			lecui::widgets::label bus_type(drive_pane.get());
			bus_type().rect = storage_type().rect;
			bus_type().rect.snap_to(storage_type().rect, snap_type::right, 0.f);
			bus_type().font_size = detail_font_size_;
			bus_type().text = drive.bus_type;

			// add drive serial number
			lecui::widgets::label serial_number_caption(drive_pane.get());
			serial_number_caption().rect = drive_model_caption().rect;
			serial_number_caption().rect.snap_to(status().rect, snap_type::bottom_left, margin_);
			serial_number_caption().color_text = caption_color_;
			serial_number_caption().font_size = caption_font_size_;
			serial_number_caption().text = "Serial Number";

			lecui::widgets::label serial_number(drive_pane.get());
			serial_number().rect = serial_number_caption().rect;
			serial_number().rect.height(detail_height);
			serial_number().rect.snap_to(serial_number_caption().rect, snap_type::bottom, 0.f);
			serial_number().font_size = detail_font_size_;
			serial_number().text = drive.serial_number;

			// add capacity
			lecui::widgets::label capacity(drive_pane.get());
			capacity().rect = serial_number().rect;
			capacity().rect.height(highlight_height);
			capacity().rect.snap_to(serial_number().rect, snap_type::bottom, margin_);
			capacity().font_size = highlight_font_size_;
			capacity().text = leccore::format_size(drive.size) + " " +
				"<span style = 'font-size: 9.0pt;'>capacity</span>";

			// add media type
			lecui::widgets::label additional(drive_pane.get());
			additional().rect = capacity().rect;
			additional().rect.height(caption_height);
			additional().rect.snap_to(capacity().rect, snap_type::bottom, 0.f);
			additional().font_size = caption_font_size_;
			additional().text = drive.media_type;

			drive_number++;
		}

		drive_tab_pane.select("Drive 0");
	}

	void on_refresh() {
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
				auto& drive_summary = lecui::widgets::label::specs(*this, "home/pc_details_pane/drive_summary");
				drive_summary.text = std::to_string(drives_.size()) + "<span style = 'font-size: 8.0pt;'>" +
					std::string(drives_.size() == 1 ? " drive" : " drives") +
					"</span>";
			}

			if (power_old_.batteries.size() != power_.batteries.size()) {
				auto& battery_summary = lecui::widgets::label::specs(*this, "home/pc_details_pane/battery_summary");
				battery_summary.text = std::to_string(power_.batteries.size()) + "<span style = 'font-size: 8.0pt;'>" +
					std::string(power_.batteries.size() == 1 ? " battery" : " batteries") +
					"</span>";
			}
		}
		catch (const std::exception) {}

		try {
			// refresh power details
			if (power_old_.ac != power_.ac) {
				auto& power_status = lecui::widgets::label::specs(*this, "home/power_pane/power_status");
				power_status.text = power_.ac ? "On AC" : "On Battery";
				power_status.text += ", ";
				power_status.text += ("<span style = 'font-size: 8.0pt;'>" +
					pc_info_.to_string(power_.status) + "</span>");
				refresh_ui = true;
			}

			if (power_old_.level != power_.level) {
				auto& level = lecui::widgets::label::specs(*this, "home/power_pane/level");
				level.text = (power_.level != -1 ?
					(std::to_string(power_.level) + "% ") : std::string("<em>Unknown</em> ")) +
					"<span style = 'font-size: 8.0pt;'>overall power level</span>";

				auto& level_bar = lecui::widgets::progress_bar::specs(*this, "home/power_pane/level_bar");
				level_bar.percentage = static_cast<float>(power_.level);
				refresh_ui = true;
			}

			if (power_old_.lifetime_remaining != power_.lifetime_remaining) {
				auto& life_remaining = lecui::widgets::label::specs(*this, "home/power_pane/life_remaining");
				life_remaining.text = power_.lifetime_remaining.empty() ? std::string() :
					(power_.lifetime_remaining + " remaining");
				refresh_ui = true;
			}

			if (power_old_.batteries.size() != power_.batteries.size()) {
				// close old tab pane
				page_man_.close("home/power_pane/battery_tab_pane");

				auto& life_remaining = lecui::widgets::label::specs(*this, "home/power_pane/life_remaining");
				auto& power_pane = lecui::containers::pane::get(*this, "home/power_pane");

				// add battery pane
				add_battery_pane(power_pane, life_remaining.rect.bottom);

				refresh_ui = true;
			}
			else {
				for (size_t battery_number = 0; battery_number < power_.batteries.size(); battery_number++) {
					auto& battery_old = power_old_.batteries[battery_number];
					auto& battery = power_.batteries[battery_number];

					if (battery_old != battery) {
						if (battery_old.current_capacity != battery.current_capacity) {
							auto& current_capacity = lecui::widgets::label::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/current_capacity");
							current_capacity.text = std::to_string(battery.current_capacity) + "mWh";
						}

						if (battery_old.level != battery.level) {
							auto& charge_level = lecui::widgets::label::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/charge_level");
							charge_level.text = leccore::round_off::tostr<char>(battery.level, 1) + "%";
						}

						if (battery_old.current_charge_rate != battery.current_charge_rate) {
							auto& charge_rate = lecui::widgets::label::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/charge_rate");
							charge_rate.text = std::to_string(battery.current_charge_rate) + "mW";
						}

						if (battery_old.current_voltage != battery.current_voltage) {
							auto& current_voltage = lecui::widgets::label::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/current_voltage");
							current_voltage.text = std::to_string(battery.current_voltage) + "mV";
						}

						if (battery_old.status != battery.status) {
							auto& status = lecui::widgets::label::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/status");
							status.text = pc_info_.to_string(battery.status);
						}

						if (battery_old.designed_capacity != battery.designed_capacity) {
							auto& designed_capacity = lecui::widgets::label::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/designed_capacity");
							designed_capacity.text = std::to_string(battery.designed_capacity) + "mWh";
						}

						if (battery_old.fully_charged_capacity != battery.fully_charged_capacity) {
							auto& fully_charged_capacity = lecui::widgets::label::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/fully_charged_capacity");
							fully_charged_capacity.text = std::to_string(battery.fully_charged_capacity) + "mWh";
						}

						if (battery_old.health != battery.health) {
							auto& health = lecui::widgets::progress_indicator::specs(*this, "home/power_pane/battery_tab_pane/Battery " + std::to_string(battery_number) + "/health");
							health.percentage = static_cast<float>(battery.health);
						}

						refresh_ui = true;
					}
				}
			}
		}
		catch (const std::exception) {}

		try {
			// to-do: refresh drive details
			if (drives_old_.size() != drives_.size()) {
				// close old tab pane
				page_man_.close("home/drive_pane/drive_tab_pane");

				auto& drive_title = lecui::widgets::label::specs(*this, "home/drive_pane/drive_title");
				auto& drive_pane = lecui::containers::pane::get(*this, "home/drive_pane");

				// add drive details pane
				add_drive_details_pane(drive_pane, drive_title.rect.bottom);

				refresh_ui = true;
			}
			else {
				for (size_t drive_number = 0; drive_number < drives_.size(); drive_number++) {
					auto& drive_old = drives_old_[drive_number];
					auto& drive = drives_[drive_number];

					if (drive_old.status != drive.status) {
						auto& status = lecui::widgets::label::specs(*this, "home/drive_pane/drive_tab_pane/Drive " + std::to_string(drive_number) + "/status");
						status.text = drive.status;
						if (drive.status == "OK")
							status.color_text = ok_color_;
						else {
							status.color_text = not_ok_color_;
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
};

// gui app using main
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

int main() {
	std::string error;
	dashboard dash(appname);
	if (!dash.show(error))
		dash.message(error);
	return 0;
}
