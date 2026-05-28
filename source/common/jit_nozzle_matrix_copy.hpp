#pragma once

#include <nozzle/nozzle_c.h>

#include "jit_nozzle_3plane_copy.hpp"
#include "jit_nozzle_format_mapping.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace jit_nozzle {

enum class matrix_copy_path : int {
	direct_copy = 0,
	argb_swizzle = 1,
	rgb3_to_storage = 2,
	long2_to_rgba32_uint = 3,
	invalid = 4,
};

struct matrix_copy_dispatch {
	matrix_copy_path path;
	const char *error;
};

struct matrix_copy_request {
	jitter_type type;
	int planecount;
	NozzleTextureFormat requested_format;
	NozzleTextureFormat mapped_format;
	NozzleResolvedTextureFormat resolved;
	uint32_t src_bpp;
};

inline bool is_jitter_swizzle_type(jitter_type type) {
	return type == jitter_type::char_type
		|| type == jitter_type::float32_type
		|| type == jitter_type::long_type;
}

inline bool validate_row_copy_bounds(
	uint32_t width,
	uint32_t height,
	uint32_t src_row_bytes,
	int64_t dst_row_stride,
	uint32_t src_bpp,
	uint32_t dst_bpp,
	const char **error
) {
	if (width == 0 || height == 0) {
		if (error) *error = "zero dimensions";
		return false;
	}

	uint64_t min_src_row = static_cast<uint64_t>(width) * src_bpp;
	if (min_src_row > UINT32_MAX) {
		if (error) *error = "src row bytes overflow";
		return false;
	}
	if (src_row_bytes < static_cast<uint32_t>(min_src_row)) {
		if (error) *error = "src_row_bytes too small";
		return false;
	}

	constexpr int64_t k_int64_min = std::numeric_limits<int64_t>::min();
	if (dst_row_stride == k_int64_min) {
		if (error) *error = "dst_row_stride is INT64_MIN";
		return false;
	}
	int64_t abs_dst_stride = dst_row_stride < 0 ? -dst_row_stride : dst_row_stride;
	if (abs_dst_stride == 0) {
		if (error) *error = "zero dst_row_stride";
		return false;
	}
	uint64_t min_dst_row = static_cast<uint64_t>(width) * dst_bpp;
	if (static_cast<uint64_t>(abs_dst_stride) < min_dst_row) {
		if (error) *error = "dst_row_stride too small";
		return false;
	}

	return true;
}

inline rgb_copy_result copy_direct_rows(
	const uint8_t *src,
	uint8_t *dst,
	uint32_t width,
	uint32_t height,
	uint32_t src_row_bytes,
	int64_t dst_row_stride,
	uint32_t bytes_per_pixel
) {
	if (!src || !dst) return {false, "null pointer"};

	const char *error = nullptr;
	if (!validate_row_copy_bounds(
		width, height, src_row_bytes, dst_row_stride,
		bytes_per_pixel, bytes_per_pixel, &error
	)) {
		return {false, error};
	}

	uint32_t copy_bytes = width * bytes_per_pixel;
	for (uint32_t y = 0; y < height; y++) {
		const uint8_t *src_row = src + static_cast<std::size_t>(y) * src_row_bytes;
		uint8_t *dst_row = dst + static_cast<int64_t>(y) * dst_row_stride;
		std::memcpy(dst_row, src_row, copy_bytes);
	}

	return {true, nullptr};
}

// Expand a 2-plane Jitter long source [R, G] into RGBA32_UINT storage
// [R, G, 0u, 1u]. This is the explicit compatibility policy used
// while Nozzle has no RG32_UINT public format.
inline rgb_copy_result copy_2plane_long_to_rgba32_uint(
	const uint8_t *src,
	uint8_t *dst,
	uint32_t width,
	uint32_t height,
	uint32_t src_row_bytes,
	int64_t dst_row_stride,
	uint32_t src_bpp,
	uint32_t dst_bpp,
	NozzleTextureFormat storage_format
) {
	if (!src || !dst) return {false, "null pointer"};
	if (storage_format != NOZZLE_FORMAT_RGBA32_UINT) return {false, "storage format must be RGBA32_UINT"};
	if (src_bpp != 8) return {false, "src_bpp must be 8 for 2-plane long"};
	if (dst_bpp != 16) return {false, "dst_bpp must be 16 for RGBA32_UINT"};

	const char *error = nullptr;
	if (!validate_row_copy_bounds(
		width, height, src_row_bytes, dst_row_stride,
		src_bpp, dst_bpp, &error
	)) {
		return {false, error};
	}

	for (uint32_t y = 0; y < height; y++) {
		const uint8_t *src_row = src + static_cast<std::size_t>(y) * src_row_bytes;
		uint8_t *dst_row = dst + static_cast<int64_t>(y) * dst_row_stride;
		for (uint32_t x = 0; x < width; x++) {
			const uint8_t *src_px = src_row + static_cast<std::size_t>(x) * src_bpp;
			uint8_t *dst_px = dst_row + static_cast<std::size_t>(x) * dst_bpp;
			uint32_t r = 0u;
			uint32_t g = 0u;
			uint32_t b = 0u;
			uint32_t a = 1u;
			std::memcpy(&r, src_px, sizeof(r));
			std::memcpy(&g, src_px + sizeof(r), sizeof(g));
			std::memcpy(dst_px, &r, sizeof(r));
			std::memcpy(dst_px + sizeof(r), &g, sizeof(g));
			std::memcpy(dst_px + sizeof(r) + sizeof(g), &b, sizeof(b));
			std::memcpy(dst_px + sizeof(r) + sizeof(g) + sizeof(b), &a, sizeof(a));
		}
	}

	return {true, nullptr};
}

inline matrix_copy_dispatch choose_matrix_copy_path(const matrix_copy_request &request) {
	if (!is_jitter_swizzle_type(request.type)) {
		return {matrix_copy_path::invalid, "unsupported jitter type"};
	}

	if (request.planecount == 4) {
		return {matrix_copy_path::argb_swizzle, nullptr};
	}

	if (request.type == jitter_type::long_type && request.planecount == 2) {
		if (request.requested_format != NOZZLE_FORMAT_RGBA32_UINT) {
			return {matrix_copy_path::invalid, "2-plane long requested format is not RGBA32_UINT"};
		}
		if (request.mapped_format != request.resolved.storage_format) {
			return {matrix_copy_path::invalid, "2-plane long mapped/storage format mismatch"};
		}
		if (request.resolved.storage_format != NOZZLE_FORMAT_RGBA32_UINT) {
			return {matrix_copy_path::invalid, "2-plane long storage format is not RGBA32_UINT"};
		}
		if (request.resolved.bytes_per_pixel != 16) {
			return {matrix_copy_path::invalid, "2-plane long resolved bpp is not 16"};
		}
		if (request.src_bpp != 8) {
			return {matrix_copy_path::invalid, "2-plane long source bpp is not 8"};
		}
		return {matrix_copy_path::long2_to_rgba32_uint, nullptr};
	}

	if (request.planecount == 3) {
		if (!is_rgb_semantic(request.resolved.semantic_format)) {
			return {matrix_copy_path::invalid, "3-plane semantic format is not RGB"};
		}
		if (!is_valid_rgb_to_rgba_storage(request.resolved.semantic_format, request.resolved.storage_format)) {
			return {matrix_copy_path::invalid, "3-plane RGB storage contract violated"};
		}
		uint32_t exp_bpp = expected_storage_bpp(request.resolved.storage_format);
		if (request.resolved.bytes_per_pixel != exp_bpp) {
			return {matrix_copy_path::invalid, "3-plane resolved bpp mismatch"};
		}
		if (request.mapped_format != request.resolved.storage_format) {
			return {matrix_copy_path::invalid, "3-plane mapped/storage format mismatch"};
		}
		return {matrix_copy_path::rgb3_to_storage, nullptr};
	}

	if (request.mapped_format != request.resolved.storage_format) {
		return {matrix_copy_path::invalid, "direct copy mapped/storage format mismatch"};
	}
	if (request.resolved.bytes_per_pixel != request.src_bpp) {
		return {matrix_copy_path::invalid, "direct copy source/storage bpp mismatch"};
	}

	return {matrix_copy_path::direct_copy, nullptr};
}

} // namespace jit_nozzle
