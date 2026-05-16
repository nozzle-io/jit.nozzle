#pragma once

#include <nozzle/nozzle_c.h>
#include <cstdint>
#include <cstring>

namespace jit_nozzle {

inline bool is_rgb_semantic(NozzleTextureFormat fmt) {
	return fmt == NOZZLE_FORMAT_RGB8_UNORM
		|| fmt == NOZZLE_FORMAT_RGB16_UNORM
		|| fmt == NOZZLE_FORMAT_RGB16_FLOAT
		|| fmt == NOZZLE_FORMAT_RGB32_FLOAT
		|| fmt == NOZZLE_FORMAT_RGB32_UINT;
}

inline bool is_valid_rgb_to_rgba_storage(NozzleTextureFormat semantic, NozzleTextureFormat storage) {
	if (semantic == NOZZLE_FORMAT_RGB8_UNORM) {
		return storage == NOZZLE_FORMAT_RGBA8_UNORM || storage == NOZZLE_FORMAT_BGRA8_UNORM;
	}
	if (semantic == NOZZLE_FORMAT_RGB16_UNORM) return storage == NOZZLE_FORMAT_RGBA16_UNORM;
	if (semantic == NOZZLE_FORMAT_RGB16_FLOAT) return storage == NOZZLE_FORMAT_RGBA16_FLOAT;
	if (semantic == NOZZLE_FORMAT_RGB32_FLOAT) return storage == NOZZLE_FORMAT_RGBA32_FLOAT;
	if (semantic == NOZZLE_FORMAT_RGB32_UINT) return storage == NOZZLE_FORMAT_RGBA32_UINT;
	return false;
}

inline uint32_t expected_storage_bpp(NozzleTextureFormat storage) {
	if (storage == NOZZLE_FORMAT_RGBA8_UNORM || storage == NOZZLE_FORMAT_BGRA8_UNORM) return 4;
	if (storage == NOZZLE_FORMAT_RGBA16_UNORM || storage == NOZZLE_FORMAT_RGBA16_FLOAT) return 8;
	if (storage == NOZZLE_FORMAT_RGBA32_FLOAT || storage == NOZZLE_FORMAT_RGBA32_UINT) return 16;
	return 0;
}

struct rgb_copy_result {
	bool ok;
	const char *error;
};

inline rgb_copy_result copy_3plane_to_storage(
	const uint8_t *src, uint8_t *dst,
	uint32_t width, uint32_t height,
	uint32_t src_row_bytes, int64_t dst_row_stride,
	uint32_t src_bpp, uint32_t dst_bpp,
	NozzleTextureFormat storage_format
) {
	if (!src || !dst) return {false, "null pointer"};
	if (width == 0 || height == 0) return {false, "zero dimensions"};

	bool is_bgra_8bit = (storage_format == NOZZLE_FORMAT_BGRA8_UNORM);

	for (uint32_t y = 0; y < height; y++) {
		const uint8_t *src_row = src + y * src_row_bytes;
		uint8_t *dst_row = dst + static_cast<int64_t>(y) * dst_row_stride;
		for (uint32_t x = 0; x < width; x++) {
			uint8_t *dst_px = dst_row + x * dst_bpp;
			const uint8_t *src_px = src_row + x * src_bpp;
			if (is_bgra_8bit) {
				dst_px[0] = src_px[2];
				dst_px[1] = src_px[1];
				dst_px[2] = src_px[0];
			} else {
				std::memcpy(dst_px, src_px, src_bpp);
			}
		}
	}
	return {true, nullptr};
}

} // namespace jit_nozzle
