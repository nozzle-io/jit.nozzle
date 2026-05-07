#pragma once

#include <nozzle/nozzle_c.h>
#include <cstdint>

namespace jit_nozzle {

enum class jitter_type : int {
	char_type = 0,
	float32_type = 1,
	long_type = 2,
};

struct format_mapping {
	jitter_type type;
	int planecount;
	uint32_t bytes_per_pixel;
};

inline format_mapping nozzle_to_jitter_format(NozzleTextureFormat fmt) {
	switch(fmt) {
		case NOZZLE_FORMAT_R8_UNORM:      return {jitter_type::char_type, 1, 1};
		case NOZZLE_FORMAT_RG8_UNORM:     return {jitter_type::char_type, 2, 2};
		case NOZZLE_FORMAT_RGBA8_UNORM:   return {jitter_type::char_type, 4, 4};
		case NOZZLE_FORMAT_BGRA8_UNORM:   return {jitter_type::char_type, 4, 4};
		case NOZZLE_FORMAT_RGBA8_SRGB:    return {jitter_type::char_type, 4, 4};
		case NOZZLE_FORMAT_BGRA8_SRGB:    return {jitter_type::char_type, 4, 4};
		case NOZZLE_FORMAT_R32_FLOAT:     return {jitter_type::float32_type, 1, 4};
		case NOZZLE_FORMAT_RG32_FLOAT:    return {jitter_type::float32_type, 2, 8};
		case NOZZLE_FORMAT_RGBA32_FLOAT:  return {jitter_type::float32_type, 4, 16};
		case NOZZLE_FORMAT_R16_FLOAT:     return {jitter_type::float32_type, 1, 4};
		case NOZZLE_FORMAT_RG16_FLOAT:    return {jitter_type::float32_type, 2, 8};
		case NOZZLE_FORMAT_RGBA16_FLOAT:  return {jitter_type::float32_type, 4, 16};
		case NOZZLE_FORMAT_R16_UNORM:     return {jitter_type::long_type, 1, 2};
		case NOZZLE_FORMAT_RG16_UNORM:    return {jitter_type::long_type, 2, 4};
		case NOZZLE_FORMAT_RGBA16_UNORM:  return {jitter_type::long_type, 4, 8};
		case NOZZLE_FORMAT_R32_UINT:      return {jitter_type::long_type, 1, 4};
		case NOZZLE_FORMAT_RGBA32_UINT:   return {jitter_type::long_type, 4, 16};
		case NOZZLE_FORMAT_DEPTH32_FLOAT: return {jitter_type::float32_type, 1, 4};
		default:                          return {jitter_type::char_type, 4, 4};
	}
}

struct send_format_mapping {
	NozzleTextureFormat nozzle_fmt;
	uint32_t bytes_per_pixel;
};

inline bool jitter_to_nozzle_format(
	jitter_type type, int planecount, send_format_mapping &out
) {
	if(type == jitter_type::char_type) {
		switch(planecount) {
			case 1: out = {NOZZLE_FORMAT_R8_UNORM, 1}; return true;
			case 2: out = {NOZZLE_FORMAT_RG8_UNORM, 2}; return true;
			case 3: out = {NOZZLE_FORMAT_RGB8_UNORM, 3}; return true;
			case 4: out = {NOZZLE_FORMAT_RGBA8_UNORM, 4}; return true;
		}
	} else if(type == jitter_type::float32_type) {
		switch(planecount) {
			case 1: out = {NOZZLE_FORMAT_R32_FLOAT, 4}; return true;
			case 2: out = {NOZZLE_FORMAT_RG32_FLOAT, 8}; return true;
			case 3: out = {NOZZLE_FORMAT_RGB32_FLOAT, 12}; return true;
			case 4: out = {NOZZLE_FORMAT_RGBA32_FLOAT, 16}; return true;
		}
	} else if(type == jitter_type::long_type) {
		switch(planecount) {
			case 1: out = {NOZZLE_FORMAT_R32_UINT, 4}; return true;
			case 2: out = {NOZZLE_FORMAT_RGBA32_UINT, 8}; return true;
			case 3: out = {NOZZLE_FORMAT_RGB32_UINT, 12}; return true;
			case 4: out = {NOZZLE_FORMAT_RGBA32_UINT, 16}; return true;
		}
	}
	return false;
}

} // namespace jit_nozzle
