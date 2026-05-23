#include "main.hpp"
#include "mdc.hpp"

#include "binex/basic_window.hpp"
#include "binex/widgets.hpp"

#include <lak/imgui/widgets.hpp>

#include <lak/system/file.hpp>
#include <lak/system/opengl/state.hpp>

#include <lak/future.hpp>
#include <lak/strconv.hpp>
#include <lak/test.hpp>

#include <stb_image_write.h>

#include <filesystem>

#include <inttypes.h>

#define LAK_BASIC_PROGRAM_IMGUI_WINDOW_IMPL
#include <lak/basic_program.inl>

lak::array<lak::image<lak::color4_t>, 4> image_change(const lak::image4_t &img)
{
	lak::array<lak::image<lak::color4_t>, 4> res;
	for (size_t i = 0U; i < 4U; ++i)
	{
		auto &_res = res[i];
		_res.resize({img.size().x - 1U, img.size().y - 1U});
		for (size_t y = 0U; y < _res.size().y; ++y)
		{
			for (size_t x = 0U; x < _res.size().x; ++x)
			{
				_res[{x, y}].r = uint8_t(
				  float(((((int16_t(img[{x, y}][i]) - int16_t(img[{x + 1U, y}][i])) +
				           (int16_t(img[{x, y + 1U}][i]) -
				            int16_t(img[{x + 1U, y + 1U}][i]))) /
				          2U) /
				         INT8_MAX) +
				        0.5f) *
				  255);
				_res[{x, y}].g = uint8_t(
				  float(((((int16_t(img[{x, y}][i]) - int16_t(img[{x, y + 1U}][i])) +
				           (int16_t(img[{x + 1U, y}][i]) -
				            int16_t(img[{x + 1U, y + 1U}][i]))) /
				          2U) /
				         INT8_MAX) +
				        0.5f) *
				  255);
				_res[{x, y}].b = 0;
				_res[{x, y}].a = 255;
			}
		}
	}
	return res;
}

struct main_window : bex::basic_window<main_window>
{
	using super_window = bex::basic_window<main_window>;

	lak::path_getter open_pgetter, save_pgetter;
	float time_acc = 0.0f;

	int diff_offset[2] = {0, 0};

	int r_offset[2]  = {8, 0};
	int g1_offset[2] = {8, 0};
	int g2_offset[2] = {12, 4};
	int b_offset[2]  = {0, 0};

	float left_size  = -1.f;
	float right_size = -1.f;

	lak::fs::path binary_path;
	lak::optional<lak::future<void>> binary_load;
	lak::array<byte_t> binary;
	lak::optional<mdc_raw> raw_file;
	bool binary_update = false, raw_update = false;

	lak::image3_t processedimg;

	bex::texture rtex, g1tex, g1dtex, g2tex, g2dtex, btex, rgbtex, gdifftex,
	  rg1difftex, rg2difftex, processedtex;

	void load_binary(lak::fs::path path)
	{
		if (auto res = lak::read_file(path); res.is_ok())
			binary = lak::move(res.unsafe_unwrap());
		else
			ERROR(res.unsafe_unwrap_err());

		if (auto res = mdc_raw::make(lak::span(binary)); res.is_ok())
			raw_file = lak::move(res.unsafe_unwrap());
		else
			ERROR(res.unsafe_unwrap_err());

		binary_path = lak::move(path);
	}

	void open_file(const lak::fs::path &path)
	{
		binary_load =
		  lak::async([this](const lak::fs::path &p) { load_binary(p); }, path);
	}

	void save_file(const lak::fs::path &path)
	{
		stbi_write_png(
		  (const char *)path.u8string().c_str(),
		  int(processedimg.size().x),
		  int(processedimg.size().y),
		  3,
		  processedimg.data(),
		  int(processedimg.contig_size_bytes() / processedimg.size().y));
	}

	const lak::fs::path &file_path() { return binary_path; }

	lak::span<byte_t> file_data() { return lak::span(binary); }

	bool update() { return binary_update || raw_update; }

	void file_menu()
	{
		if (auto res = open_pgetter(); res) open_file(*res);
		if (auto res = save_pgetter(); res) save_file(*res);

		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open...", nullptr, false))
				open_pgetter.open_file(file_path(),
				                       "Minolta RD-175 Raw Files{.MDC},.*");

			if (ImGui::MenuItem(
			      "Save...", nullptr, false, processedimg.contig_size() != 0U))
				save_pgetter.save_file(file_path().parent_path() /
				                         (file_path().stem().u8string() + u8".PNG"),
				                       "Image Files{.PNG},.*");

			ImGui::EndMenu();
		}
	}

	void main_region(float frame_time)
	{
		if (binary_load)
		{
			ImGui::BeginChild(
			  "Mid", {-1, -1}, true, ImGuiWindowFlags_NoSavedSettings);
			time_acc += frame_time;
			if (time_acc > 3.0f) time_acc -= std::trunc(time_acc);
			if (time_acc > 2.0f)
				ImGui::Text("Loading...");
			else if (time_acc > 1.0f)
				ImGui::Text("Loading..");
			else
				ImGui::Text("Loading.");

			if (binary_load->has_value())
			{
				binary_load.reset();
				binary_update = true;
				raw_update    = true;
				time_acc      = 0.0f;
			}
			ImGui::EndChild();
		}
		else if (binary.empty())
		{
			ImGui::BeginChild(
			  "Mid", {-1, -1}, true, ImGuiWindowFlags_NoSavedSettings);
			ImGui::Text("No file");
			ImGui::EndChild();
		}
		else if (!raw_file)
			super_window::main_region(frame_time);
		else
		{
			if (raw_update)
			{
				{
					lak::image4_t rimg;
					rimg.resize({0x300U, 0x1EEU});
					for (size_t y = 0U; y < 0x1EEU; ++y)
					{
						const size_t _y2 = y * 0x180U;
						for (size_t x = 0U; x < 0x300U; ++x)
						{
							rimg[{x, y}] =
							  lak::color4_t(raw_file->red[(x / 2U) + _y2], 0, 0, 255);
						}
					}
					rtex.emplace(rimg);
				}
				{
					lak::image4_t g1img;
					g1img.resize({0x300U, 0x1EEU});
					for (size_t y = 0U; y < 0x1EEU; ++y)
					{
						const size_t _y = y * 0x300U;
						for (size_t x = 0U; x < 0x300U; ++x)
						{
							g1img[{x, y}] =
							  lak::color4_t(0, raw_file->green1[x + _y], 0, 255);
						}
					}
					g1tex.emplace(g1img);
					g1dtex.emplace(image_change(g1img)[1]);
				}
				{
					lak::image4_t g2img;
					g2img.resize({0x300U, 0x1EEU});
					for (size_t y = 0U; y < 0x1EEU; ++y)
					{
						const size_t _y = y * 0x300U;
						for (size_t x = 0U; x < 0x300U; ++x)
						{
							g2img[{x, y}] =
							  lak::color4_t(0, raw_file->green2[x + _y], 0, 255);
						}
					}
					g2tex.emplace(g2img);
					g2dtex.emplace(image_change(g2img)[1]);
				}
				{
					lak::image4_t bimg;
					bimg.resize({0x300U, 0x1EEU});
					for (size_t y = 0U; y < 0x1EEU; ++y)
					{
						const size_t _y2 = y * 0x180U;
						for (size_t x = 0U; x < 0x300U; ++x)
						{
							bimg[{x, y}] =
							  lak::color4_t(0, 0, raw_file->blue[(x / 2U) + _y2], 255);
						}
					}
					btex.emplace(bimg);
				}
				{
					lak::image4_t rgbimg;
					rgbimg.resize({0x300U, 0x1EEU});
					for (size_t y = 0U; y < 0x1EEU; ++y)
					{
						const size_t _y  = y * 0x300U;
						const size_t _y2 = y * 0x180U;
						for (size_t x = 0U; x < 0x300U; ++x)
						{
							rgbimg[{x, y}] = lak::color4_t(raw_file->red[(x / 2U) + _y2],
							                               raw_file->green1[x + _y],
							                               raw_file->blue[(x / 2U) + _y2],
							                               255);
						}
					}
					rgbtex.emplace(rgbimg);
				}
				{
					lak::image4_t gdiffimg;
					gdiffimg.resize({0x300U, 0x1EEU});
					for (size_t y = -std::min(0, diff_offset[1]);
					     y < size_t(0x1EEU - std::max(0, diff_offset[1]));
					     ++y)
					{
						const size_t _y  = y * 0x300U;
						const size_t _yd = (y + diff_offset[1]) * 0x300U;
						for (size_t x = -std::min(0, diff_offset[0]);
						     x < size_t(0x300U - std::max(0, diff_offset[0]));
						     ++x)
						{
							uint8_t v =
							  std::max(raw_file->green1[x + _y],
							           raw_file->green2[(x + diff_offset[0]) + _yd]) -
							  std::min(raw_file->green1[x + _y],
							           raw_file->green2[(x + diff_offset[0]) + _yd]);
							gdiffimg[{x, y}] = lak::color4_t(v, 255 - v, v, 255);
						}
					}
					gdifftex.emplace(gdiffimg);
				}
				{
					lak::image4_t rg1diffimg;
					rg1diffimg.resize({0x300U, 0x1EEU});
					for (size_t y = -std::min(0, diff_offset[1]);
					     y < size_t(0x1EEU - std::max(0, diff_offset[1]));
					     ++y)
					{
						const size_t _yr = y * 0x180U;
						const size_t _yd = (y + diff_offset[1]) * 0x300U;
						for (size_t x = -std::min(0, diff_offset[0]);
						     x < size_t(0x300U - std::max(0, diff_offset[0]));
						     ++x)
						{
							uint8_t v =
							  std::max(raw_file->red[(x / 2U) + _yr],
							           raw_file->green1[(x + diff_offset[0]) + _yd]) -
							  std::min(raw_file->red[(x / 2U) + _yr],
							           raw_file->green1[(x + diff_offset[0]) + _yd]);
							rg1diffimg[{x, y}] = lak::color4_t(v, 255 - v, v, 255);
						}
					}
					rg1difftex.emplace(rg1diffimg);
				}
				{
					lak::image4_t rg2diffimg;
					rg2diffimg.resize({0x300U, 0x1EEU});
					for (size_t y = -std::min(0, diff_offset[1]);
					     y < size_t(0x1EEU - std::max(0, diff_offset[1]));
					     ++y)
					{
						const size_t _yr = y * 0x180U;
						const size_t _yd = (y + diff_offset[1]) * 0x300U;
						for (size_t x = -std::min(0, diff_offset[0]);
						     x < size_t(0x300U - std::max(0, diff_offset[0]));
						     ++x)
						{
							uint8_t v =
							  std::max(raw_file->red[(x / 2U) + _yr],
							           raw_file->green2[(x + diff_offset[0]) + _yd]) -
							  std::min(raw_file->red[(x / 2U) + _yr],
							           raw_file->green2[(x + diff_offset[0]) + _yd]);
							rg2diffimg[{x, y}] = lak::color4_t(v, 255 - v, v, 255);
						}
					}
					rg2difftex.emplace(rg2diffimg);
				}
				{
					raw_file->red_offset = {int16_t(r_offset[0]), int16_t(r_offset[1])};
					raw_file->green1_offset = {int16_t(g1_offset[0]),
					                           int16_t(g1_offset[1])};
					raw_file->green2_offset = {int16_t(g2_offset[0]),
					                           int16_t(g2_offset[1])};
					raw_file->blue_offset = {int16_t(b_offset[0]), int16_t(b_offset[1])};
					processedimg          = raw_file->process();
					lak::image4_t img2;
					img2.resize(processedimg.size());
					for (const auto i :
					     lak::size_range_count(processedimg.contig_size()))
						img2[i] = {
						  processedimg[i].r, processedimg[i].g, processedimg[i].b, 255U};
					processedtex.emplace(img2);
				}
				raw_update = false;
			}

			{
				const auto content_size{ImGui::GetContentRegionAvail()};

				if (left_size <= 0.f || right_size <= 0.f)
				{
					left_size  = content_size.x / 2;
					right_size = content_size.x / 2;
				}

				lak::VertSplitter(left_size, right_size, content_size.x);

				ImGui::BeginChild(
				  "ImgLeft", {left_size, -1}, true, ImGuiWindowFlags_NoSavedSettings);
				LAK_TREE_NODE("Processed")
				{
					ImGui::Text("Sensor Alignment (x1/8th of a pixel)");
					ImGui::SliderInt2("Red", r_offset, -16, 16);
					if (ImGui::IsItemDeactivatedAfterEdit()) raw_update = true;
					ImGui::SliderInt2("Green 1", g1_offset, -16, 16);
					if (ImGui::IsItemDeactivatedAfterEdit()) raw_update = true;
					ImGui::SliderInt2("Green 2", g2_offset, -16, 16);
					if (ImGui::IsItemDeactivatedAfterEdit()) raw_update = true;
					ImGui::SliderInt2("Blue", b_offset, -16, 16);
					if (ImGui::IsItemDeactivatedAfterEdit()) raw_update = true;
					bex::view_image(processedtex, 1.0f);
				}
				LAK_TREE_NODE("RGB") { bex::view_image(rgbtex, 2.0f); }
				LAK_TREE_NODE("G1d") { bex::view_image(g1dtex, 2.0f); }
				LAK_TREE_NODE("G2d") { bex::view_image(g2dtex, 2.0f); }
				LAK_TREE_NODE("G diff")
				{
					ImGui::SliderInt2("diff offset", diff_offset, -2, 2);
					if (ImGui::IsItemDeactivatedAfterEdit()) raw_update = true;
					bex::view_image(gdifftex, 3.0f);
				}
				LAK_TREE_NODE("R G1 diff")
				{
					ImGui::SliderInt2("diff offset", diff_offset, -2, 2);
					if (ImGui::IsItemDeactivatedAfterEdit()) raw_update = true;
					bex::view_image(rg1difftex, 3.0f);
				}
				LAK_TREE_NODE("R G2 diff")
				{
					ImGui::SliderInt2("diff offset", diff_offset, -2, 2);
					if (ImGui::IsItemDeactivatedAfterEdit()) raw_update = true;
					bex::view_image(rg2difftex, 3.0f);
				}
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("ImgRight",
				                  {right_size, -1},
				                  true,
				                  ImGuiWindowFlags_NoSavedSettings);
				LAK_TREE_NODE("Info")
				{
					ImGui::Text("Shutter Speed 0x%" PRIX8, raw_file->shutter_speed);
					ImGui::Text("Aperture 0x%" PRIX8, raw_file->aperture);
					ImGui::Text("Exposure Compensation %-+1.1f",
					            float(raw_file->exposure_compensation) / 8.f);
					ImGui::Text("Focal Length 0x%" PRIX8, raw_file->focal_length);
				}
				LAK_TREE_NODE("R") { bex::view_image(rtex, 2.0f); }
				LAK_TREE_NODE("G1") { bex::view_image(g1tex, 2.0f); }
				LAK_TREE_NODE("G2") { bex::view_image(g2tex, 2.0f); }
				LAK_TREE_NODE("B") { bex::view_image(btex, 2.0f); }
				ImGui::EndChild();
			}
		}
	}
};

struct my_window : virtual public LAK_BASIC_PROGRAM(window_api)
{
	my_window() : LAK_BASIC_PROGRAM(window_api)() {}

	main_window bex_window;

	virtual void init() override final { window().set_title(L"" APP_NAME); }

	virtual ~my_window() {}

	virtual void handle_event(lak::event &event) override final
	{
		switch (event.type)
		{
			case lak::event_type::close_window:
				destroy();
				break;
			case lak::event_type::dropfile:
				bex_window.open_file(event.dropfile().path);
				break;
		}
	}

	virtual void loop(uint64_t counter_delta) override final
	{
		const float frame_time =
		  (float)counter_delta / lak::performance_frequency();
		bex_window.draw(frame_time);
		if (bex_window.binary_update)
		{
			window().set_title(L"" APP_NAME " " +
			                   bex_window.binary_path.generic_wstring());
			bex_window.binary_update = false;
		}
	}
};

lak::optional<lak::fs::path> preload_file;

lak::error_code<int> basic_program_preinit(lak::span<char *> args)
{
	if (!args.empty()) args = args.subspan(1U);

	if (args.size() == 1U && args[0U] == lak::astring("--version"))
	{
		std::cout << APP_NAME << "\n";
		return lak::err_t{EXIT_SUCCESS};
	}

	lak::debugger.std_out(u8"", u8"" APP_NAME "\n");

	lak::debugger.crash_path = std::filesystem::current_path() /
	                           "ATTACH-TO-ISSUE-ON-MDC-RAW-GITHUB-REPO.txt";

	lak::debugger.live_output_enabled = true;

	for (size_t arg = 0U; arg < args.size(); ++arg)
	{
		if (args[arg] == lak::astring("-h") || args[arg] == lak::astring("--help"))
		{
			lak::debugger.std_out(
			  u8"",
			  u8"mdc.exe "
			  "[--help] "
			  "[--in-place] "
			  "[--nogl] "
			  "[--onlyerr] "
			  "[--listtests | --laktestall | --laktests \"test1;test2\"] "
			  "[<filepath>]\n");

			return lak::err_t{0};
		}
		else if (args[arg] == lak::astring("--in-place"))
		{
			++arg;
			if (arg >= args.size()) FATAL("Missing file");
			if (!lak::path_exists(args[arg]).UNWRAP())
			{
				FATAL("file ", args[arg], " does not exists");
			}
			auto in_path = lak::fs::path(args[arg]);
			auto out_path =
			  in_path.parent_path() / (in_path.stem().u8string() + u8".PNG");
			lak::debugger.std_out(u8"", u8"Reading " + in_path.u8string() + u8"\n");
			auto binary       = lak::read_file(in_path).UNWRAP();
			auto raw_file     = mdc_raw::make(lak::span(binary)).UNWRAP();
			auto processedimg = raw_file.process();
			lak::debugger.std_out(u8"", u8"Writing " + out_path.u8string() + u8"\n");
			return lak::err_t{stbi_write_png(
			  (const char *)out_path.u8string().c_str(),
			  int(processedimg.size().x),
			  int(processedimg.size().y),
			  3,
			  processedimg.data(),
			  int(processedimg.contig_size_bytes() / processedimg.size().y))};
		}
		else if (args[arg] == lak::astring("--nogl"))
		{
			basic_window_force_software = true;
		}
		else if (args[arg] == lak::astring("--onlyerr"))
		{
			lak::debugger.live_errors_only = true;
		}
		else if (args[arg] == lak::astring("--listtests"))
		{
			lak::debugger.std_out(lak::u8string(),
			                      lak::u8string(u8"Available tests:\n"));
			for (const auto &[name, func] : lak::registered_tests())
			{
				lak::debugger.std_out(lak::u8string(),
				                      lak::to_u8string(name) + u8"\n");
			}
		}
		else if (args[arg] == lak::astring("--laktestall"))
		{
			return lak::err_t{lak::run_tests()};
		}
		else if (args[arg] == lak::astring("--laktests") ||
		         args[arg] == lak::astring("--laktest"))
		{
			++arg;
			if (arg >= args.size()) FATAL("Missing tests");
			return lak::err_t{lak::run_tests(
			  lak::as_u8string(lak::astring_view::from_c_str(args[arg])))};
		}
		else
		{
			if (lak::path_exists(args[arg]).UNWRAP())
			{
				preload_file = lak::fs::path(args[arg]);
			}
			else
				FATAL("file ", args[arg], " does not exists");
		}
	}

	basic_window_target_framerate                = 30;
	basic_window_opengl_settings.major           = 3;
	basic_window_opengl_settings.minor           = 2;
	basic_window_opengl_settings.double_buffered = true;

	return lak::ok_t{};
}

lak::weak_ptr<LAK_BASIC_PROGRAM(window_instance<my_window>)> my_window_ptr;

lak::error_code<int> basic_program_init()
{
	auto map_str_err = [](lak::u8string err) -> int
	{
		ERROR(err);
		return EXIT_FAILURE;
	};

	RES_TRY_ASSIGN(
	  my_window_ptr =,
	  LAK_BASIC_PROGRAM(create_window<my_window>)().map_err(map_str_err));

	{
		auto p = my_window_ptr.get();
		DEBUG_EXPR(p->window().graphics());
		if (preload_file) p->bex_window.open_file(*preload_file);
	}

	return lak::ok_t{};
}

void basic_program_handle_event(lak::event &event)
{
	switch (event.type)
	{
		case lak::event_type::quit_program:
			for (auto &inst : basic_window_instances()) inst->destroy();
			break;

		default:
			break;
	}
}

bool basic_program_loop(uint64_t counter_delta)
{
	LAK_UNUSED(counter_delta);
	return !basic_window_instances().empty();
}

int basic_program_quit()
{
	my_window_ptr.reset();
	return EXIT_SUCCESS;
}
