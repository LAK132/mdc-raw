#ifndef MDC_RAW_MDC_HPP
#define MDC_RAW_MDC_HPP

#include <lak/array.hpp>
#include <lak/binary_traits.hpp>
#include <lak/image.hpp>
#include <lak/result.hpp>
#include <lak/span.hpp>
#include <lak/stdint.hpp>

// sensors are 768 * 494 pixels.
// final image size is 1528 * 1146.
// pixel aspect ratio is approximately 1.16 (25:29).

struct mdc_raw
{
public:
	lak::vec2i16_t red_offset    = {8, 0};
	lak::vec2i16_t green1_offset = {8, 0};
	lak::vec2i16_t green2_offset = {12, 4};
	lak::vec2i16_t blue_offset   = {0, 0};

	lak::array<uint8_t> preamble;
	lak::array<uint8_t> red;
	lak::array<uint8_t> green1;
	lak::array<uint8_t> green2;
	lak::array<uint8_t> blue;

	uint8_t shutter_speed;
	uint8_t aperture;
	int8_t exposure_compensation;
	uint8_t autoness;
	uint8_t focal_length;

	static lak::result<mdc_raw, lak::out_of_data_error> make(
	  lak::span<const byte_t> source);

	mdc_raw(const mdc_raw &)            = default;
	mdc_raw(mdc_raw &&)                 = default;
	mdc_raw &operator=(const mdc_raw &) = default;
	mdc_raw &operator=(mdc_raw &&)      = default;

	lak::image3_t process() const;

private:
	mdc_raw() = default;
};

#endif
