#include "main.hpp"
#include "mdc.hpp"

#include "binex/basic_window.hpp"
#include "binex/widgets.hpp"

#include <lak/imgui/widgets.hpp>

#include <lak/opengl/state.hpp>

#include <lak/file.hpp>
#include <lak/future.hpp>
#include <lak/strconv.hpp>
#include <lak/test.hpp>

#include <filesystem>

#define LAK_BASIC_PROGRAM_IMGUI_WINDOW_IMPL
#include <lak/basic_program.inl>

int opengl_major, opengl_minor;
lak::graphics_mode graphics_mode;
bool force_only_error = false;

lak::fs::path binary_path;
lak::optional<lak::future<void>> binary_load;
lak::array<byte_t> binary;
lak::optional<mdc_raw> raw_file;
bool binary_update = false, raw_update = false;

bex::texture rtex, g1tex, g1dtex, g2tex, g2dtex, btex, rgbtex, gdifftex;

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

void load_binary_async(const lak::fs::path &path)
{
	binary_load = lak::async(load_binary, path);
}

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

	static void open_file(const lak::fs::path &path) { load_binary_async(path); }

	static const lak::fs::path &file_path() { return binary_path; }

	static lak::span<byte_t> file_data() { return lak::span(binary); }

	static lak::graphics_mode graphics_mode() { return ::graphics_mode; }

	static bool update() { return binary_update || raw_update; }

	static void main_region(float frame_time)
	{
		if (binary_load)
		{
			ImGui::BeginChild(
			  "Mid", {-1, -1}, true, ImGuiWindowFlags_NoSavedSettings);
			static float time_acc = 0.0f;
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
			static int diff_offset[2] = {0, 0};

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
					rtex = bex::create_texture(rimg, graphics_mode());
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
					g1tex = bex::create_texture(g1img, graphics_mode());
					g1dtex =
					  bex::create_texture(image_change(g1img)[1], graphics_mode());
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
					g2tex = bex::create_texture(g2img, graphics_mode());
					g2dtex =
					  bex::create_texture(image_change(g2img)[1], graphics_mode());
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
					btex = bex::create_texture(bimg, graphics_mode());
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
					rgbtex = bex::create_texture(rgbimg, graphics_mode());
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
					gdifftex = bex::create_texture(gdiffimg, graphics_mode());
				}
				raw_update = false;
			}

			{
				const auto content_size{ImGui::GetContentRegionAvail()};

				static float left_size  = content_size.x / 2;
				static float right_size = content_size.x / 2;

				lak::VertSplitter(left_size, right_size, content_size.x);

				ImGui::BeginChild(
				  "ImgLeft", {left_size, -1}, true, ImGuiWindowFlags_NoSavedSettings);
				// bex::image_view(rgbtex, 2.0f);
				// bex::image_view(g1tex, 2.0f);
				// bex::image_view(g1dtex, 2.0f);
				ImGui::SliderInt2("diff offset", diff_offset, -2, 2);
				if (ImGui::IsItemDeactivatedAfterEdit()) raw_update = true;
				bex::image_view(gdifftex, 3.0f);
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("ImgRight",
				                  {right_size, -1},
				                  true,
				                  ImGuiWindowFlags_NoSavedSettings);
				bex::image_view(rtex, 2.0f);
				bex::image_view(g2tex, 2.0f);
				bex::image_view(g2dtex, 2.0f);
				bex::image_view(btex, 2.0f);
				ImGui::EndChild();
			}
		}
	}
};

lak::optional<int> basic_program_init(int argc, char **argv)
{
	if (argc == 2 && argv[1] == lak::astring("--version"))
	{
		std::cout << APP_NAME << "\n";
		return lak::optional<int>(0);
	}

	lak::debugger.std_out(u8"", u8"" APP_NAME "\n");

	for (int arg = 1; arg < argc; ++arg)
	{
		if (argv[arg] == lak::astring("-h") || argv[arg] == lak::astring("--help"))
		{
			std::cout << "mdc.exe "
			             "[--help] "
			             "[--nogl] "
			             "[--onlyerr] "
			             "[--listtests | --laktestall | --laktests \"test1;test2\"] "
			             "[<filepath>]\n";

			return lak::optional<int>(0);
		}
		else if (argv[arg] == lak::astring("--nogl"))
		{
			basic_window_force_software = true;
		}
		else if (argv[arg] == lak::astring("--onlyerr"))
		{
			force_only_error = true;
		}
		else if (argv[arg] == lak::astring("--listtests"))
		{
			lak::debugger.std_out(lak::u8string(),
			                      lak::u8string(u8"Available tests:\n"));
			for (const auto &[name, func] : lak::registered_tests())
			{
				lak::debugger.std_out(lak::u8string(),
				                      lak::to_u8string(name) + u8"\n");
			}
		}
		else if (argv[arg] == lak::astring("--laktestall"))
		{
			return lak::optional<int>(lak::run_tests());
		}
		else if (argv[arg] == lak::astring("--laktests") ||
		         argv[arg] == lak::astring("--laktest"))
		{
			++arg;
			if (arg >= argc) FATAL("Missing tests");
			return lak::optional<int>(lak::run_tests(
			  lak::as_u8string(lak::astring_view::from_c_str(argv[arg]))));
		}
		else
		{
			if (lak::path_exists(argv[arg]).UNWRAP())
			{
				// :TODO: do something with the file
			}
			else
				FATAL("file ", argv[arg], " does not exists");
		}
	}

#ifdef LAK_OS_APPLE
	basic_window_force_software = true;
#endif

	basic_window_target_framerate      = 30;
	basic_window_opengl_settings.major = 3;
	basic_window_opengl_settings.minor = 2;
	basic_window_clear_colour          = {0.0f, 0.0f, 0.0f, 1.0f};
	basic_imgui_main_window_flags =
	  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar |
	  ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings |
	  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove;

	basic_create_window().UNWRAP();

	return lak::nullopt;
}

bool mdc_raw_running = true;
bool basic_program_loop(uint64_t counter_delta)
{
	LAK_UNUSED(counter_delta);
	return mdc_raw_running && !basic_window_instances.empty();
}

int basic_program_quit() { return EXIT_SUCCESS; }

void basic_window_init(lak::window &window)
{
	lak::debugger.crash_path =
	  std::filesystem::current_path() /
	  "ATTACH-TO-ISSUE-ON-SOURCE-EXPLORER-GITHUB-REPO.txt";

	lak::debugger.live_output_enabled = true;

	graphics_mode = window.graphics();

	DEBUG("Graphics: ", graphics_mode);
	if (!lak::debugger.live_output_enabled || lak::debugger.live_errors_only)
		std::cout << "Graphics: " << graphics_mode << "\n";

	switch (graphics_mode)
	{
		case lak::graphics_mode::OpenGL:
		{
			opengl_major = lak::opengl::get_uint(GL_MAJOR_VERSION).UNWRAP();
			opengl_minor = lak::opengl::get_uint(GL_MINOR_VERSION).UNWRAP();
		}
		break;

		default:
			break;
	}

	window.set_title(L"MDC RAW");
}

void basic_window_handle_event(lak::window *window, lak::event &event)
{
	switch (event.type)
	{
		case lak::event_type::close_window:
			basic_destroy_window(*window);
			ASSERT(!!window);
			break;

		case lak::event_type::quit_program:
			mdc_raw_running = false;
			break;

		case lak::event_type::dropfile:
			load_binary_async(lak::fs::path(event.dropfile().path));
			break;

		default:
			break;
	}
}

void basic_window_loop(lak::window &window, uint64_t counter_delta)
{
	const float frame_time = (float)counter_delta / lak::performance_frequency();

	main_window::draw(frame_time);

	if (binary_update)
	{
		window.set_title(L"MDC RAW " + binary_path.generic_wstring());
		binary_update = false;
	}
}

void basic_window_quit(lak::window &) {}
