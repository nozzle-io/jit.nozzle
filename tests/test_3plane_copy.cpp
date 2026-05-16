#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>

#include <nozzle/nozzle_c.h>

#include "jit_nozzle_3plane_copy.hpp"

using jit_nozzle::is_rgb_semantic;
using jit_nozzle::is_valid_rgb_to_rgba_storage;
using jit_nozzle::expected_storage_bpp;
using jit_nozzle::copy_3plane_to_storage;

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

int main() {
	std::printf("=== 3-plane RGB→RGBA/BGRA copy tests ===\n");

	// --- is_rgb_semantic ---
	{
		CHECK(is_rgb_semantic(NOZZLE_FORMAT_RGB8_UNORM), "rgb8 is rgb semantic");
		CHECK(is_rgb_semantic(NOZZLE_FORMAT_RGB32_FLOAT), "rgb32f is rgb semantic");
		CHECK(is_rgb_semantic(NOZZLE_FORMAT_RGB32_UINT), "rgb32ui is rgb semantic");
		CHECK(!is_rgb_semantic(NOZZLE_FORMAT_RGBA8_UNORM), "rgba8 is not rgb semantic");
		CHECK(!is_rgb_semantic(NOZZLE_FORMAT_R8_UNORM), "r8 is not rgb semantic");
	}

	// --- is_valid_rgb_to_rgba_storage ---
	{
		CHECK(is_valid_rgb_to_rgba_storage(NOZZLE_FORMAT_RGB8_UNORM, NOZZLE_FORMAT_RGBA8_UNORM),
			"rgb8→rgba8 valid");
		CHECK(is_valid_rgb_to_rgba_storage(NOZZLE_FORMAT_RGB8_UNORM, NOZZLE_FORMAT_BGRA8_UNORM),
			"rgb8→bgra8 valid");
		CHECK(!is_valid_rgb_to_rgba_storage(NOZZLE_FORMAT_RGB8_UNORM, NOZZLE_FORMAT_RGBA32_FLOAT),
			"rgb8→rgba32f invalid");
		CHECK(is_valid_rgb_to_rgba_storage(NOZZLE_FORMAT_RGB32_FLOAT, NOZZLE_FORMAT_RGBA32_FLOAT),
			"rgb32f→rgba32f valid");
		CHECK(is_valid_rgb_to_rgba_storage(NOZZLE_FORMAT_RGB32_UINT, NOZZLE_FORMAT_RGBA32_UINT),
			"rgb32ui→rgba32ui valid");
		CHECK(!is_valid_rgb_to_rgba_storage(NOZZLE_FORMAT_RGB8_UNORM, NOZZLE_FORMAT_RGB8_UNORM),
			"rgb8→rgb8 invalid (not rgba storage)");
	}

	// --- expected_storage_bpp ---
	{
		CHECK_EQ(expected_storage_bpp(NOZZLE_FORMAT_RGBA8_UNORM), 4u, "rgba8 bpp=4");
		CHECK_EQ(expected_storage_bpp(NOZZLE_FORMAT_BGRA8_UNORM), 4u, "bgra8 bpp=4");
		CHECK_EQ(expected_storage_bpp(NOZZLE_FORMAT_RGBA16_UNORM), 8u, "rgba16 bpp=8");
		CHECK_EQ(expected_storage_bpp(NOZZLE_FORMAT_RGBA32_FLOAT), 16u, "rgba32f bpp=16");
		CHECK_EQ(expected_storage_bpp(NOZZLE_FORMAT_RGBA32_UINT), 16u, "rgba32ui bpp=16");
		CHECK_EQ(expected_storage_bpp(NOZZLE_FORMAT_R8_UNORM), 0u, "r8 bpp=0 (unknown)");
	}

	// --- copy_3plane_to_storage: RGB8 → RGBA8 ---
	{
		uint8_t src[3] = {0x11, 0x22, 0x33};
		uint8_t dst[4] = {0xAA, 0xAA, 0xAA, 0xAA};

		auto r = copy_3plane_to_storage(src, dst, 1, 1, 3, 4, 3, 4,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(r.ok, "rgba8 copy ok");
		CHECK_EQ(dst[0], 0x11, "rgba8 R preserved");
		CHECK_EQ(dst[1], 0x22, "rgba8 G preserved");
		CHECK_EQ(dst[2], 0x33, "rgba8 B preserved");
		// alpha not touched by copy_3plane (filled separately by nozzle_fill_opaque_alpha_channel)
	}

	// --- copy_3plane_to_storage: RGB8 → BGRA8 (R/B swap) ---
	{
		uint8_t src[3] = {0x11, 0x22, 0x33};
		uint8_t dst[4] = {0xAA, 0xAA, 0xAA, 0xAA};

		auto r = copy_3plane_to_storage(src, dst, 1, 1, 3, 4, 3, 4,
			NOZZLE_FORMAT_BGRA8_UNORM);
		CHECK(r.ok, "bgra8 copy ok");
		CHECK_EQ(dst[0], 0x33, "bgra8 dst[0]=B=src[2]");
		CHECK_EQ(dst[1], 0x22, "bgra8 dst[1]=G=src[1]");
		CHECK_EQ(dst[2], 0x11, "bgra8 dst[2]=R=src[0]");
	}

	// --- copy_3plane_to_storage: multi-pixel RGBA8 ---
	{
		uint8_t src[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
		uint8_t dst[8] = {};

		auto r = copy_3plane_to_storage(src, dst, 2, 1, 6, 8, 3, 4,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(r.ok, "multi-pixel rgba8 copy ok");
		CHECK_EQ(dst[0], 0x11, "px0 R");
		CHECK_EQ(dst[1], 0x22, "px0 G");
		CHECK_EQ(dst[2], 0x33, "px0 B");
		CHECK_EQ(dst[4], 0x44, "px1 R");
		CHECK_EQ(dst[5], 0x55, "px1 G");
		CHECK_EQ(dst[6], 0x66, "px1 B");
	}

	// --- copy_3plane_to_storage: multi-pixel BGRA8 ---
	{
		uint8_t src[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
		uint8_t dst[8] = {};

		auto r = copy_3plane_to_storage(src, dst, 2, 1, 6, 8, 3, 4,
			NOZZLE_FORMAT_BGRA8_UNORM);
		CHECK(r.ok, "multi-pixel bgra8 copy ok");
		CHECK_EQ(dst[0], 0x33, "px0 B=src[2]");
		CHECK_EQ(dst[1], 0x22, "px0 G=src[1]");
		CHECK_EQ(dst[2], 0x11, "px0 R=src[0]");
		CHECK_EQ(dst[4], 0x66, "px1 B=src[5]");
		CHECK_EQ(dst[5], 0x55, "px1 G=src[4]");
		CHECK_EQ(dst[6], 0x44, "px1 R=src[3]");
	}

	// --- copy_3plane_to_storage: RGB32_FLOAT → RGBA32_FLOAT (4-byte components) ---
	{
		float src_f[3] = {0.25f, 0.5f, 0.75f};
		float dst_f[4] = {};

		auto r = copy_3plane_to_storage(
			reinterpret_cast<const uint8_t *>(src_f),
			reinterpret_cast<uint8_t *>(dst_f),
			1, 1, 12, 16, 12, 16,
			NOZZLE_FORMAT_RGBA32_FLOAT);
		CHECK(r.ok, "rgba32f copy ok");
		CHECK_EQ(dst_f[0], 0.25f, "rgba32f R preserved");
		CHECK_EQ(dst_f[1], 0.5f, "rgba32f G preserved");
		CHECK_EQ(dst_f[2], 0.75f, "rgba32f B preserved");
	}

	// --- copy_3plane_to_storage: RGB32_UINT → RGBA32_UINT ---
	// 32-bit uint storage (long matrix type in Jitter context)
	{
		uint32_t src_u[3] = {100u, 200u, 300u};
		uint32_t dst_u[4] = {};

		auto r = copy_3plane_to_storage(
			reinterpret_cast<const uint8_t *>(src_u),
			reinterpret_cast<uint8_t *>(dst_u),
			1, 1, 12, 16, 12, 16,
			NOZZLE_FORMAT_RGBA32_UINT);
		CHECK(r.ok, "rgba32ui copy ok");
		CHECK_EQ(dst_u[0], 100u, "rgba32ui R preserved");
		CHECK_EQ(dst_u[1], 200u, "rgba32ui G preserved");
		CHECK_EQ(dst_u[2], 300u, "rgba32ui B preserved");
	}

	// --- copy_3plane_to_storage: multi-row with stride ---
	{
		uint8_t src[2 * 6] = {
			0x10, 0x20, 0x30, 0x40, 0x50, 0x60,
			0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0
		};
		uint8_t dst[2 * 8] = {};

		auto r = copy_3plane_to_storage(src, dst, 2, 2, 6, 8, 3, 4,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(r.ok, "multi-row rgba8 copy ok");
		// Row 0
		CHECK_EQ(dst[0], 0x10, "row0 px0 R");
		CHECK_EQ(dst[1], 0x20, "row0 px0 G");
		CHECK_EQ(dst[2], 0x30, "row0 px0 B");
		CHECK_EQ(dst[4], 0x40, "row0 px1 R");
		CHECK_EQ(dst[5], 0x50, "row0 px1 G");
		CHECK_EQ(dst[6], 0x60, "row0 px1 B");
		// Row 1
		CHECK_EQ(dst[8], 0x70, "row1 px0 R");
		CHECK_EQ(dst[9], 0x80, "row1 px0 G");
		CHECK_EQ(dst[10], 0x90, "row1 px0 B");
		CHECK_EQ(dst[12], 0xA0, "row1 px1 R");
		CHECK_EQ(dst[13], 0xB0, "row1 px1 G");
		CHECK_EQ(dst[14], 0xC0, "row1 px1 B");
	}

	// --- copy_3plane_to_storage: null pointer rejected ---
	{
		uint8_t buf[4]{};
		auto r = copy_3plane_to_storage(nullptr, buf, 1, 1, 3, 4, 3, 4,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(!r.ok, "null src rejected");
		r = copy_3plane_to_storage(buf, nullptr, 1, 1, 3, 4, 3, 4,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(!r.ok, "null dst rejected");
	}

	// --- copy_3plane_to_storage: zero dimensions rejected ---
	{
		uint8_t buf[4]{};
		auto r = copy_3plane_to_storage(buf, buf, 0, 1, 3, 4, 3, 4,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(!r.ok, "zero width rejected");
		r = copy_3plane_to_storage(buf, buf, 1, 0, 3, 4, 3, 4,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(!r.ok, "zero height rejected");
	}

	// --- copy_3plane_to_storage: unsupported storage format rejected ---
	{
		uint8_t src[3] = {0x11, 0x22, 0x33};
		uint8_t dst[4]{};
		auto r = copy_3plane_to_storage(src, dst, 1, 1, 3, 4, 3, 4,
			NOZZLE_FORMAT_RGB8_UNORM);
		CHECK(!r.ok, "rgb8 storage format rejected");
		r = copy_3plane_to_storage(src, dst, 1, 1, 3, 4, 3, 4,
			NOZZLE_FORMAT_R8_UNORM);
		CHECK(!r.ok, "r8 storage format rejected");
	}

	// --- copy_3plane_to_storage: dst_bpp mismatch rejected ---
	{
		uint8_t src[3] = {0x11, 0x22, 0x33};
		uint8_t dst[8]{};
		auto r = copy_3plane_to_storage(src, dst, 1, 1, 3, 8, 3, 8,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(!r.ok, "dst_bpp mismatch rejected (expected 4 for rgba8)");
	}

	// --- copy_3plane_to_storage: src_row_bytes too small rejected ---
	{
		uint8_t src[3] = {0x11, 0x22, 0x33};
		uint8_t dst[4]{};
		auto r = copy_3plane_to_storage(src, dst, 2, 1, 3, 8, 3, 4,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(!r.ok, "src_row_bytes too small rejected (2*3=6 > 3)");
	}

	// --- copy_3plane_to_storage: zero dst_row_stride rejected ---
	{
		uint8_t src[3] = {0x11, 0x22, 0x33};
		uint8_t dst[4]{};
		auto r = copy_3plane_to_storage(src, dst, 1, 1, 3, 0, 3, 4,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(!r.ok, "zero dst_row_stride rejected");
	}

	// --- copy_3plane_to_storage: dst_row_stride too small rejected ---
	{
		uint8_t src[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
		uint8_t dst[4]{};
		auto r = copy_3plane_to_storage(src, dst, 2, 1, 6, 4, 3, 4,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(!r.ok, "dst_row_stride too small rejected (2*4=8 > 4)");
	}

	// --- copy_3plane_to_storage: negative dst_row_stride accepted ---
	{
		uint8_t src[2 * 6] = {
			0x10, 0x20, 0x30, 0x40, 0x50, 0x60,
			0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0
		};
		uint8_t dst[2 * 8] = {};
		uint8_t *logical_first_row = dst + 8;
		auto r = copy_3plane_to_storage(src, logical_first_row, 2, 2, 6, -8, 3, 4,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(r.ok, "negative dst_row_stride accepted");
		// logical row 0 at dst+8, logical row 1 at dst+0
		CHECK_EQ(dst[8], 0x10, "neg stride logical row0 px0 R");
		CHECK_EQ(dst[9], 0x20, "neg stride logical row0 px0 G");
		CHECK_EQ(dst[10], 0x30, "neg stride logical row0 px0 B");
		CHECK_EQ(dst[12], 0x40, "neg stride logical row0 px1 R");
		CHECK_EQ(dst[0], 0x70, "neg stride logical row1 px0 R");
		CHECK_EQ(dst[1], 0x80, "neg stride logical row1 px0 G");
		CHECK_EQ(dst[2], 0x90, "neg stride logical row1 px0 B");
		CHECK_EQ(dst[4], 0xA0, "neg stride logical row1 px1 R");
	}

	// --- copy_3plane_to_storage: INT64_MIN dst_row_stride rejected ---
	{
		uint8_t src[3] = {0x11, 0x22, 0x33};
		uint8_t dst[4]{};
		auto r = copy_3plane_to_storage(src, dst, 1, 1, 3,
			std::numeric_limits<int64_t>::min(), 3, 4,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(!r.ok, "INT64_MIN dst_row_stride rejected");
	}

	// --- copy_3plane_to_storage: src_bpp mismatch rejected ---
	{
		uint8_t src[2] = {0x11, 0x22};
		uint8_t dst[4]{};
		auto r = copy_3plane_to_storage(src, dst, 1, 1, 2, 4, 2, 4,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(!r.ok, "src_bpp=2 rejected for rgba8 (expected 3)");
		r = copy_3plane_to_storage(src, dst, 1, 1, 1, 4, 1, 4,
			NOZZLE_FORMAT_RGBA8_UNORM);
		CHECK(!r.ok, "src_bpp=1 rejected for rgba8 (expected 3)");
	}

	// --- copy_3plane_to_storage: width*src_bpp overflow rejected ---
	{
		uint8_t buf[4]{};
		auto r = copy_3plane_to_storage(buf, buf, UINT32_MAX, 1, 12, 16, 12, 16,
			NOZZLE_FORMAT_RGBA32_FLOAT);
		CHECK(!r.ok, "width*src_bpp overflow rejected");
	}

	std::printf("\n%d/%d tests passed\n", tests_run - tests_failed, tests_run);
	return tests_failed > 0 ? 1 : 0;
}
