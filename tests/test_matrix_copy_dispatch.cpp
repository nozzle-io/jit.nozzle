#include <cstdio>
#include <cstdint>
#include <cstring>
#include <limits>

#include <nozzle/nozzle_c.h>

#include "jit_nozzle_matrix_copy.hpp"

using jit_nozzle::copy_2plane_long_to_rgba32_uint;
using jit_nozzle::choose_matrix_copy_path;
using jit_nozzle::jitter_type;
using jit_nozzle::matrix_copy_path;
using jit_nozzle::matrix_copy_request;

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) \
	do { \
		tests_run++; \
		if (!(cond)) { \
			std::printf("  FAIL: %s (line %d)\n", msg, __LINE__); \
			tests_failed++; \
		} \
	} while(0)

#define CHECK_EQ(a, b, msg) CHECK((a) == (b), msg)

static NozzleResolvedTextureFormat resolved_format(
	NozzleTextureFormat storage,
	NozzleTextureFormat semantic,
	uint8_t bytes_per_pixel
) {
	NozzleResolvedTextureFormat resolved{};
	resolved.storage_format = storage;
	resolved.semantic_format = semantic;
	resolved.bytes_per_pixel = bytes_per_pixel;
	return resolved;
}

int main() {
	std::printf("=== matrix copy dispatch tests ===\n");

	// --- copy_2plane_long_to_rgba32_uint: poisoned destination overwritten ---
	{
		uint32_t src[4] = {10u, 20u, 30u, 40u};
		uint32_t dst[8];
		for (uint32_t &v : dst) v = 0xDEADBEEFu;

		auto r = copy_2plane_long_to_rgba32_uint(
			reinterpret_cast<const uint8_t *>(src),
			reinterpret_cast<uint8_t *>(dst),
			2, 1, 2 * 8, 2 * 16, 8, 16,
			NOZZLE_FORMAT_RGBA32_UINT);

		CHECK(r.ok, "long2 helper copy ok");
		CHECK_EQ(dst[0], 10u, "px0 R copied");
		CHECK_EQ(dst[1], 20u, "px0 G copied");
		CHECK_EQ(dst[2], 0u, "px0 B synthesized zero over poison");
		CHECK_EQ(dst[3], 1u, "px0 A synthesized one over poison");
		CHECK_EQ(dst[4], 30u, "px1 R copied");
		CHECK_EQ(dst[5], 40u, "px1 G copied");
		CHECK_EQ(dst[6], 0u, "px1 B synthesized zero over poison");
		CHECK_EQ(dst[7], 1u, "px1 A synthesized one over poison");
	}

	// --- copy_2plane_long_to_rgba32_uint: multi-row stride and padding ---
	{
		constexpr uint32_t poison = 0xDEADBEEFu;
		constexpr uint32_t src_pad = 0xAAAAAAAAu;
		uint32_t src[10] = {
			1u, 2u, 3u, 4u, src_pad,
			5u, 6u, 7u, 8u, src_pad,
		};
		uint32_t dst[20];
		for (uint32_t &v : dst) v = poison;

		auto r = copy_2plane_long_to_rgba32_uint(
			reinterpret_cast<const uint8_t *>(src),
			reinterpret_cast<uint8_t *>(dst),
			2, 2, 5 * 4, 10 * 4, 8, 16,
			NOZZLE_FORMAT_RGBA32_UINT);

		CHECK(r.ok, "long2 helper stride copy ok");
		CHECK_EQ(dst[0], 1u, "row0 px0 R");
		CHECK_EQ(dst[1], 2u, "row0 px0 G");
		CHECK_EQ(dst[2], 0u, "row0 px0 B");
		CHECK_EQ(dst[3], 1u, "row0 px0 A");
		CHECK_EQ(dst[4], 3u, "row0 px1 R");
		CHECK_EQ(dst[5], 4u, "row0 px1 G");
		CHECK_EQ(dst[6], 0u, "row0 px1 B");
		CHECK_EQ(dst[7], 1u, "row0 px1 A");
		CHECK_EQ(dst[8], poison, "row0 dst padding untouched 0");
		CHECK_EQ(dst[9], poison, "row0 dst padding untouched 1");
		CHECK_EQ(dst[10], 5u, "row1 px0 R");
		CHECK_EQ(dst[11], 6u, "row1 px0 G");
		CHECK_EQ(dst[12], 0u, "row1 px0 B");
		CHECK_EQ(dst[13], 1u, "row1 px0 A");
		CHECK_EQ(dst[14], 7u, "row1 px1 R");
		CHECK_EQ(dst[15], 8u, "row1 px1 G");
		CHECK_EQ(dst[16], 0u, "row1 px1 B");
		CHECK_EQ(dst[17], 1u, "row1 px1 A");
		CHECK_EQ(dst[18], poison, "row1 dst padding untouched 0");
		CHECK_EQ(dst[19], poison, "row1 dst padding untouched 1");
	}

	// --- copy_2plane_long_to_rgba32_uint: validation failures ---
	{
		uint8_t buf[32]{};

		auto r = copy_2plane_long_to_rgba32_uint(
			nullptr, buf, 1, 1, 8, 16, 8, 16, NOZZLE_FORMAT_RGBA32_UINT);
		CHECK(!r.ok, "null src rejected");

		r = copy_2plane_long_to_rgba32_uint(
			buf, nullptr, 1, 1, 8, 16, 8, 16, NOZZLE_FORMAT_RGBA32_UINT);
		CHECK(!r.ok, "null dst rejected");

		r = copy_2plane_long_to_rgba32_uint(
			buf, buf, 1, 1, 8, 16, 8, 16, NOZZLE_FORMAT_RGBA32_FLOAT);
		CHECK(!r.ok, "wrong storage format rejected");

		r = copy_2plane_long_to_rgba32_uint(
			buf, buf, 1, 1, 8, 16, 4, 16, NOZZLE_FORMAT_RGBA32_UINT);
		CHECK(!r.ok, "wrong src bpp rejected");

		r = copy_2plane_long_to_rgba32_uint(
			buf, buf, 1, 1, 8, 16, 8, 8, NOZZLE_FORMAT_RGBA32_UINT);
		CHECK(!r.ok, "wrong dst bpp rejected");

		r = copy_2plane_long_to_rgba32_uint(
			buf, buf, 2, 1, 8, 32, 8, 16, NOZZLE_FORMAT_RGBA32_UINT);
		CHECK(!r.ok, "undersized src row rejected");

		r = copy_2plane_long_to_rgba32_uint(
			buf, buf, 2, 1, 16, 16, 8, 16, NOZZLE_FORMAT_RGBA32_UINT);
		CHECK(!r.ok, "undersized dst stride rejected");

		r = copy_2plane_long_to_rgba32_uint(
			buf, buf, 1, 1, 8, std::numeric_limits<int64_t>::min(),
			8, 16, NOZZLE_FORMAT_RGBA32_UINT);
		CHECK(!r.ok, "INT64_MIN dst stride rejected");
	}

	// --- dispatch: 2-plane long selects expansion, not direct memcpy ---
	{
		matrix_copy_request req{};
		req.type = jitter_type::long_type;
		req.planecount = 2;
		req.requested_format = NOZZLE_FORMAT_RGBA32_UINT;
		req.mapped_format = NOZZLE_FORMAT_RGBA32_UINT;
		req.resolved = resolved_format(
			NOZZLE_FORMAT_RGBA32_UINT, NOZZLE_FORMAT_RGBA32_UINT, 16);
		req.src_bpp = 8;

		auto d = choose_matrix_copy_path(req);
		CHECK_EQ(d.path, matrix_copy_path::long2_to_rgba32_uint,
			"long2 dispatch selects RGBA32_UINT expansion");
		CHECK(d.path != matrix_copy_path::direct_copy,
			"long2 dispatch does not select direct copy");

		uint32_t src[2] = {101u, 202u};
		uint32_t dst[4] = {
			0xDEADBEEFu, 0xDEADBEEFu, 0xDEADBEEFu, 0xDEADBEEFu,
		};
		auto r = copy_2plane_long_to_rgba32_uint(
			reinterpret_cast<const uint8_t *>(src),
			reinterpret_cast<uint8_t *>(dst),
			1, 1, 8, 16, 8, 16,
			NOZZLE_FORMAT_RGBA32_UINT);
		CHECK(r.ok, "long2 dispatched helper copy ok");
		CHECK_EQ(dst[0], 101u, "long2 dispatch output R");
		CHECK_EQ(dst[1], 202u, "long2 dispatch output G");
		CHECK_EQ(dst[2], 0u, "long2 dispatch output B synthesized");
		CHECK_EQ(dst[3], 1u, "long2 dispatch output A synthesized");
	}

	// --- dispatch: 2-plane char remains direct RG8 copy ---
	{
		matrix_copy_request req{};
		req.type = jitter_type::char_type;
		req.planecount = 2;
		req.requested_format = NOZZLE_FORMAT_RG8_UNORM;
		req.mapped_format = NOZZLE_FORMAT_RG8_UNORM;
		req.resolved = resolved_format(
			NOZZLE_FORMAT_RG8_UNORM, NOZZLE_FORMAT_RG8_UNORM, 2);
		req.src_bpp = 2;

		auto d = choose_matrix_copy_path(req);
		CHECK_EQ(d.path, matrix_copy_path::direct_copy,
			"char2 dispatch remains direct copy");
	}

	// --- dispatch: 2-plane float32 remains direct RG32_FLOAT copy ---
	{
		matrix_copy_request req{};
		req.type = jitter_type::float32_type;
		req.planecount = 2;
		req.requested_format = NOZZLE_FORMAT_RG32_FLOAT;
		req.mapped_format = NOZZLE_FORMAT_RG32_FLOAT;
		req.resolved = resolved_format(
			NOZZLE_FORMAT_RG32_FLOAT, NOZZLE_FORMAT_RG32_FLOAT, 8);
		req.src_bpp = 8;

		auto d = choose_matrix_copy_path(req);
		CHECK_EQ(d.path, matrix_copy_path::direct_copy,
			"float2 dispatch remains direct copy");
	}

	// --- dispatch: 4-plane long remains existing ARGB swizzle path ---
	{
		matrix_copy_request req{};
		req.type = jitter_type::long_type;
		req.planecount = 4;
		req.requested_format = NOZZLE_FORMAT_RGBA32_UINT;
		req.mapped_format = NOZZLE_FORMAT_RGBA32_UINT;
		req.resolved = resolved_format(
			NOZZLE_FORMAT_RGBA32_UINT, NOZZLE_FORMAT_RGBA32_UINT, 16);
		req.src_bpp = 16;

		auto d = choose_matrix_copy_path(req);
		CHECK_EQ(d.path, matrix_copy_path::argb_swizzle,
			"long4 dispatch remains ARGB swizzle");
		CHECK(d.path != matrix_copy_path::long2_to_rgba32_uint,
			"long4 dispatch does not select long2 helper");
	}

	// --- dispatch: direct copy rejects source/storage bpp mismatch ---
	{
		matrix_copy_request req{};
		req.type = jitter_type::char_type;
		req.planecount = 2;
		req.requested_format = NOZZLE_FORMAT_RG8_UNORM;
		req.mapped_format = NOZZLE_FORMAT_RG8_UNORM;
		req.resolved = resolved_format(
			NOZZLE_FORMAT_RG8_UNORM, NOZZLE_FORMAT_RG8_UNORM, 4);
		req.src_bpp = 2;

		auto d = choose_matrix_copy_path(req);
		CHECK_EQ(d.path, matrix_copy_path::invalid,
			"direct copy rejects source/storage bpp mismatch");
	}

	std::printf("\n%d/%d tests passed\n", tests_run - tests_failed, tests_run);
	return tests_failed > 0 ? 1 : 0;
}
