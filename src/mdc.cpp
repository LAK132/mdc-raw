#include "mdc.hpp"

#include <lak/binary_reader.hpp>

lak::result<mdc_raw, lak::out_of_data_error> mdc_raw::make(
  lak::span<const byte_t> source)
{
	lak::binary_reader strm{source};

	mdc_raw res;

	RES_TRY_ASSIGN(res.preamble =, strm.read<uint8_t>(0x200U));

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
