#include "mdc.hpp"

#include <lak/binary_reader.hpp>
#include <lak/integer_range.hpp>

#include <execution>
#include <ranges>

lak::result<mdc_raw, lak::out_of_data_error> mdc_raw::make(
  lak::span<const byte_t> source)
{
	lak::binary_reader strm{source};

	mdc_raw res;

	RES_TRY_ASSIGN(res.preamble =, strm.read<uint8_t>(0x200U));

	{
		lak::binary_reader pre_strm{
		  lak::span<const byte_t>(lak::span(res.preamble))};

		// 0x00

		RES_TRY_ASSIGN([[maybe_unused]] uint16_t unk1 =, pre_strm.read_u16le());

		// 0x02

		RES_TRY_ASSIGN([[maybe_unused]] uint16_t unk2 =, pre_strm.read_u16le());

		// 0x04

		RES_TRY_ASSIGN([[maybe_unused]] uint16_t unk3 =, pre_strm.read_u16le());

		// 0x06

		RES_TRY_ASSIGN([[maybe_unused]] uint16_t unk4 =, pre_strm.read_u16le());

		// 0x08

		// 0x04/0x02: no redeye/redeye

		// 0x12/0x14: flash/flash redeye

		// 0x04: M
		// 0x02: A/S/P

		// 0x0X: daylight/tungsten/flash/fluro/auto
		// 0x1X: external flash
		RES_TRY_ASSIGN(res.autoness =, pre_strm.read_u8le());

		// 0x09

		RES_TRY_ASSIGN([[maybe_unused]] uint8_t unk5 =, pre_strm.read_u8le());

		// 0x0A

		RES_TRY_ASSIGN([[maybe_unused]] uint8_t unk6 =, pre_strm.read_u8le());

		// 0x0B

		// 0x1E: 16mm
		// 0x1F: 17mm
		// 0x21:
		// 0x22:
		// 0x23: 20mm
		// 0x27: 24mm
		// 0x2B: 28mm
		// 0x30: 35mm
		// 0x32: 35mm
		// 0x34:
		// 0x3A: 50mm
		// 0x38: 50mm
		// 0x39: 50mm
		// 0x3C: 60mm
		// 0x3E:
		// 0x3F: 70mm
		// 0x40: 70mm
		// 0x41: 75mm
		// 0x48: 100mm
		// 0x4F: 135mm
		// 0x58: 200mm
		// 0x61: 300mm
		// 0x68: 400mm
		RES_TRY_ASSIGN(res.focal_length =, pre_strm.read_u8le());

		// 0x0C

		// 0x2C: f/4.5
		// 0x30: f/5.6
		// 0x34: f/6.7
		// 0x38: f/8
		// 0x3C: f/9.5
		// 0x40: f/11
		// 0x44: f/13
		// 0x48: f/16
		// 0x4C: f/19
		// 0x50: f/22
		RES_TRY_ASSIGN(res.aperture =, pre_strm.read_u8le());

		// 0x0D

		// something to do with focus?

		RES_TRY_ASSIGN([[maybe_unused]] uint16_t maybe_focus =,
		               pre_strm.read_u16le());

		// 0x0F

		// 0x1F/0x00:
		// 0x21/0x01:
		// 0x22/0x1F:
		// 0x23/0x27:
		// 0x27/0x15:
		// 0x2B/0x20:
		// 0x2B/0x63:
		// 0x30/0x1C:
		// 0x3F/0x77:
		// 0x40/0x63:
		// 0x41/0x7D:

		RES_TRY_ASSIGN(
		  [[maybe_unused]] uint8_t something_to_do_with_focal_length_maybe =,
		  pre_strm.read_u8le());

		// 0x10

		// 0x38: 1/2s
		// 0x3D: 1/3s
		// 0x40: 1/4s
		// 0x45: 1/6s
		// 0x48: 1/8s
		// 0x4D: 1/10s
		// 0x50: 1/15s
		// 0x55: 1/20s
		// 0x58: 1/30s
		// 0x5D?
		// 0x60: 1/60s
		// 0x64: 1/90s
		// 0x68: 1/125s
		// 0x6D?
		// 0x70: 1/250s
		// 0x74?
		// 0x78: 1/500s
		// 0x7D?
		// 0x80: 1/1000s
		// 0x85?: 1/1500s
		// 0x88: 1/2000s
		RES_TRY_ASSIGN(res.shutter_speed =, pre_strm.read_u8le());

		// 0x11

		// exp comp = read_s8 / 8.f

		// 0xE8: -3
		// 0xEC: -2.5
		// 0xF0: -2
		// 0xF4: -1.5
		// 0xF8: -1
		// 0xFC: -0.5
		// 0x00: 0
		// 0x04: +0.5
		// 0x08: +1
		// 0x0C: +1.5
		// 0x10: +2
		// 0x14: +2.5
		// 0x18: +3
		RES_TRY_ASSIGN(res.exposure_compensation =, pre_strm.read_s8le());

		// 0x12

		// affected below f/6.7?
		RES_TRY_ASSIGN([[maybe_unused]] uint16_t unk10 =, pre_strm.read_u16le());
	}

	// sensors are 768 * 494 (0x300 * 0x1EE) pixels.
	// sections are 768 * 82 (0x300 * 0x52) bytes.

	constexpr size_t sector_size = 0x300U * 0x52U;

	lak::array<lak::span<const byte_t>, 7U> g1, rb, g2;

	for (size_t i = 0U; i < 6U; ++i)
	{
		RES_TRY_ASSIGN(g1[i] =, strm.read_bytes(sector_size));
		RES_TRY_ASSIGN(rb[i] =, strm.read_bytes(sector_size));
	}
	for (size_t i = 0U; i < 6U; ++i)
	{
		RES_TRY_ASSIGN(g2[i] =, strm.read_bytes(sector_size));
	}
	RES_TRY_ASSIGN(g1[6] =, strm.read_bytes(0x300U * 2));
	RES_TRY_ASSIGN(rb[6] =, strm.read_bytes(0x300U * 2));
	RES_TRY_ASSIGN(g2[6] =, strm.read_bytes(0x300U * 2));

	res.red.resize(0x180U * 0x1EEU);
	for (size_t y = 0U; y < (0x52U * 6U); ++y)
	{
		const size_t _y  = y * 0x180U;
		const size_t _y6 = (y / 6U) * 0x300U;
		for (size_t x = 0U; x < 0x180U; ++x)
		{
			res.red[x + _y] = uint8_t(rb[y % 6U][(x * 2U) + _y6]);
		}
	}
	for (size_t y = (0x52U * 6U); y < 0x1EEU; ++y)
	{
		const size_t _y  = y * 0x180U;
		const size_t _y6 = (y - (0x52U * 6U)) * 0x300U;
		for (size_t x = 0U; x < 0x180U; ++x)
		{
			res.red[x + _y] = uint8_t(rb[6][(x * 2U) + _y6]);
		}
	}

	res.green1.resize(0x300U * 0x1EEU);
	for (size_t y = 0U; y < (0x52U * 6U); ++y)
	{
		const size_t _y  = y * 0x300U;
		const size_t _y6 = (y / 6U) * 0x300U;
		for (size_t x = 0U; x < 0x300U; ++x)
		{
			res.green1[x + _y] = uint8_t(g1[y % 6U][x + _y6]);
		}
	}
	for (size_t y = (0x52U * 6U); y < 0x1EEU; ++y)
	{
		const size_t _y  = y * 0x300U;
		const size_t _y6 = (y - (0x52U * 6U)) * 0x300U;
		for (size_t x = 0U; x < 0x300U; ++x)
		{
			res.green1[x + _y] = uint8_t(g1[6][x + _y6]);
		}
	}

	res.green2.resize(0x300U * 0x1EEU);
	for (size_t y = 0U; y < (0x52U * 6U); ++y)
	{
		const size_t _y  = y * 0x300U;
		const size_t _y6 = (y / 6U) * 0x300U;
		for (size_t x = 0U; x < 0x300U; ++x)
		{
			res.green2[x + _y] = uint8_t(g2[y % 6U][x + _y6]);
		}
	}
	for (size_t y = (0x52U * 6U); y < 0x1EEU; ++y)
	{
		const size_t _y  = y * 0x300U;
		const size_t _y6 = (y - (0x52U * 6U)) * 0x300U;
		for (size_t x = 0U; x < 0x300U; ++x)
		{
			res.green2[x + _y] = uint8_t(g2[6][x + _y6]);
		}
	}

	res.blue.resize(0x180U * 0x1EEU);
	for (size_t y = 0U; y < (0x52U * 6U); ++y)
	{
		const size_t _y  = y * 0x180U;
		const size_t _y6 = (y / 6U) * 0x300U;
		for (size_t x = 0U; x < 0x180U; ++x)
		{
			res.blue[x + _y] = uint8_t(rb[y % 6U][1U + (x * 2U) + _y6]);
		}
	}
	for (size_t y = (0x52U * 6U); y < 0x1EEU; ++y)
	{
		const size_t _y  = y * 0x180U;
		const size_t _y6 = (y - (0x52U * 6U)) * 0x300U;
		for (size_t x = 0U; x < 0x180U; ++x)
		{
			res.blue[x + _y] = uint8_t(rb[6][1U + (x * 2U) + _y6]);
		}
	}

	// TODO: there's 6 more rows of pixels at the end that make up the last 2
	// rows of pixels per sensor.

	return lak::move_ok(res);
}

lak::image3_t mdc_raw::process() const
{
	const lak::vec2s_t sensor_size{768U, 494U};
	const lak::vec2s_t aspect_ratio{25U, 29U};
	const lak::vec2s_t inset{4U, 2U};
	const lak::vec2s_t offset{8U, 0U};
	lak::image3_t result;
	// 768 x 494
	// - 2*inset      = 760 x 490
	// * aspect_ratio = 19000 x 14210
	// / 10           = 1900 x 1421
	result.resize(((sensor_size - (inset * size_t(2U))) * aspect_ratio) /
	              size_t(10U));

	auto sample_channel = [](const lak::span<const uint8_t> data,
	                         const lak::vec2s_t source_size,
	                         lak::vec2s_t index) -> uint8_t
	{
		const auto t_x = uint32_t(index.x % 8U);
		const auto t_y = uint32_t(index.y % 8U);
		index          = index / size_t(8U);

		const size_t x1 = std::min(index.x, source_size.x - 1U);
		const size_t x2 = std::min(index.x + 1U, source_size.x - 1U);
		const size_t y1 = std::min(index.y, source_size.y - 1U);
		const size_t y2 = std::min(index.y + 1U, source_size.y - 1U);

		const auto d11 = data[x1 + (y1 * source_size.x)];
		const auto d21 = data[x2 + (y1 * source_size.x)];
		const uint32_t dt1 =
		  ((uint32_t(d11) * (8U - t_x)) + (uint32_t(d21) * t_x)) / 8U;

		const auto d12 = data[x1 + (y2 * source_size.x)];
		const auto d22 = data[x2 + (y2 * source_size.x)];
		const uint32_t dt2 =
		  ((uint32_t(d12) * (8U - t_x)) + (uint32_t(d22) * t_x)) / 8U;

		return uint8_t(((uint32_t(dt1) * (8U - t_y)) + (uint32_t(dt2) * t_y)) /
		               8U);
	};

	auto sample = [&, this](const lak::vec2s_t r_pos,
	                        const lak::vec2s_t g1_pos,
	                        const lak::vec2s_t g2_pos,
	                        const lak::vec2s_t b_pos) -> lak::color3_t
	{
		const auto r_sample  = sample_channel(red, {0x180U, 0x1EEU}, r_pos);
		const auto g1_sample = sample_channel(green1, {0x300U, 0x1EEU}, g1_pos);
		const auto g2_sample = sample_channel(green2, {0x300U, 0x1EEU}, g2_pos);
		const auto b_sample  = sample_channel(blue, {0x180U, 0x1EEU}, b_pos);
		// :TODO: better interpolate g1 and g2.
		return {r_sample,
		        uint8_t((uint16_t(g1_sample) + uint16_t(g2_sample)) / 2U),
		        b_sample};
	};

	std::ranges::iota_view range(size_t(0U), size_t(result.contig_size()));
	std::transform(
	  std::execution::par_unseq,
	  range.begin(),
	  range.end(),
	  result.data(),
	  [&](const size_t i) -> lak::color3_t
	  {
		  const size_t x = i % result.size().x;
		  const size_t y = i / result.size().x;

		  const size_t _y   = size_t(((uint64_t(y) * 10U * 8U) / aspect_ratio.y) +
                               inset.y + offset.y);
		  const size_t r_y  = red_offset.y < 0
		                        ? _y - std::min(_y, size_t(-red_offset.y))
		                        : _y + size_t(red_offset.y);
		  const size_t g1_y = green1_offset.y < 0
		                        ? _y - std::min(_y, size_t(-green1_offset.y))
		                        : _y + size_t(green1_offset.y);
		  const size_t g2_y = green2_offset.y < 0
		                        ? _y - std::min(_y, size_t(-green2_offset.y))
		                        : _y + size_t(green2_offset.y);
		  const size_t b_y  = blue_offset.y < 0
		                        ? _y - std::min(_y, size_t(-blue_offset.y))
		                        : _y + size_t(blue_offset.y);

		  const size_t _x = size_t(((uint64_t(x) * 10U * 8U) / aspect_ratio.x) +
		                           inset.x + offset.x);
		  const size_t r_x =
		    (red_offset.x < 0 ? _y - std::min(_x, size_t(-red_offset.x))
		                      : _x + size_t(red_offset.x)) /
		    2U;
		  const size_t g1_x = green1_offset.x < 0
		                        ? _y - std::min(_x, size_t(-green1_offset.x))
		                        : _x + size_t(green1_offset.x);
		  const size_t g2_x = green2_offset.x < 0
		                        ? _y - std::min(_x, size_t(-green2_offset.x))
		                        : _x + size_t(green2_offset.x);
		  const size_t b_x =
		    (blue_offset.x < 0 ? _y - std::min(_x, size_t(-blue_offset.x))
		                       : _x + size_t(blue_offset.x)) /
		    2U;

		  return sample({r_x, r_y}, {g1_x, g1_y}, {g2_x, g2_y}, {b_x, b_y});
	  });

	return result;
}
