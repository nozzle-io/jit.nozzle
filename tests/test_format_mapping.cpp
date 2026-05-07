#include <cassert>
#include <cstdio>
#include <cstring>

extern "C" {
#include <nozzle/nozzle_c.h>
}

#include "jit_nozzle_format_mapping.hpp"

using jit_nozzle::jitter_type;
using jit_nozzle::format_mapping;

struct mock_symbol { const char *s; };
static mock_symbol _jit_sym_char_val = {"char"};
static mock_symbol _jit_sym_float32_val = {"float32"};
static mock_symbol _jit_sym_long_val = {"long"};
static mock_symbol _jit_sym_jit_matrix_val = {"jit_matrix"};

#define _jit_sym_char       (&_jit_sym_char_val)
#define _jit_sym_float32    (&_jit_sym_float32_val)
#define _jit_sym_long       (&_jit_sym_long_val)
#define _jit_sym_jit_matrix (&_jit_sym_jit_matrix_val)

struct jitter_matrix_format {
    NozzleTextureFormat nozzle_fmt;
    uint32_t bytes_per_pixel;
};

static bool jitter_to_nozzle_format(
    mock_symbol *type, int planecount, jitter_matrix_format &out
) {
    if(type == _jit_sym_char) {
        switch(planecount) {
            case 1: out = {NOZZLE_FORMAT_R8_UNORM, 1}; return true;
            case 2: out = {NOZZLE_FORMAT_RG8_UNORM, 2}; return true;
            case 3: out = {NOZZLE_FORMAT_RGBA8_UNORM, 3}; return true;
            case 4: out = {NOZZLE_FORMAT_RGBA8_UNORM, 4}; return true;
        }
    } else if(type == _jit_sym_float32) {
        switch(planecount) {
            case 1: out = {NOZZLE_FORMAT_R32_FLOAT, 4}; return true;
            case 2: out = {NOZZLE_FORMAT_RG32_FLOAT, 8}; return true;
            case 3: out = {NOZZLE_FORMAT_RGBA32_FLOAT, 12}; return true;
            case 4: out = {NOZZLE_FORMAT_RGBA32_FLOAT, 16}; return true;
        }
    } else if(type == _jit_sym_long) {
        switch(planecount) {
            case 1: out = {NOZZLE_FORMAT_R32_UINT, 4}; return true;
            case 2: out = {NOZZLE_FORMAT_RGBA32_UINT, 8}; return true;
            case 3: out = {NOZZLE_FORMAT_RGBA32_UINT, 12}; return true;
            case 4: out = {NOZZLE_FORMAT_RGBA32_UINT, 16}; return true;
        }
    }
    return false;
}

static mock_symbol *type_to_mock(jitter_type t) {
    static mock_symbol *symbols[] = {_jit_sym_char, _jit_sym_float32, _jit_sym_long};
    return symbols[static_cast<int>(t)];
}

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
    std::printf("=== jitter_to_nozzle_format tests ===\n");

    {
        jitter_matrix_format out{};
        CHECK(jitter_to_nozzle_format(_jit_sym_char, 1, out), "char 1-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_R8_UNORM, "char 1-plane → R8_UNORM");
        CHECK_EQ(out.bytes_per_pixel, 1u, "char 1-plane → 1 byte");
    }
    {
        jitter_matrix_format out{};
        CHECK(jitter_to_nozzle_format(_jit_sym_char, 2, out), "char 2-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_RG8_UNORM, "char 2-plane → RG8_UNORM");
        CHECK_EQ(out.bytes_per_pixel, 2u, "char 2-plane → 2 bytes");
    }
    {
        jitter_matrix_format out{};
        CHECK(jitter_to_nozzle_format(_jit_sym_char, 4, out), "char 4-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_RGBA8_UNORM, "char 4-plane → RGBA8_UNORM");
        CHECK_EQ(out.bytes_per_pixel, 4u, "char 4-plane → 4 bytes");
    }
    {
        jitter_matrix_format out{};
        CHECK(jitter_to_nozzle_format(_jit_sym_float32, 1, out), "float32 1-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_R32_FLOAT, "float32 1-plane → R32_FLOAT");
        CHECK_EQ(out.bytes_per_pixel, 4u, "float32 1-plane → 4 bytes");
    }
    {
        jitter_matrix_format out{};
        CHECK(jitter_to_nozzle_format(_jit_sym_float32, 2, out), "float32 2-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_RG32_FLOAT, "float32 2-plane → RG32_FLOAT");
        CHECK_EQ(out.bytes_per_pixel, 8u, "float32 2-plane → 8 bytes");
    }
    {
        jitter_matrix_format out{};
        CHECK(jitter_to_nozzle_format(_jit_sym_float32, 4, out), "float32 4-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_RGBA32_FLOAT, "float32 4-plane → RGBA32_FLOAT");
        CHECK_EQ(out.bytes_per_pixel, 16u, "float32 4-plane → 16 bytes");
    }
    {
        jitter_matrix_format out{};
        CHECK(jitter_to_nozzle_format(_jit_sym_long, 1, out), "long 1-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_R32_UINT, "long 1-plane → R32_UINT");
        CHECK_EQ(out.bytes_per_pixel, 4u, "long 1-plane → 4 bytes");
    }
    {
        jitter_matrix_format out{};
        CHECK(jitter_to_nozzle_format(_jit_sym_long, 4, out), "long 4-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_RGBA32_UINT, "long 4-plane → RGBA32_UINT");
        CHECK_EQ(out.bytes_per_pixel, 16u, "long 4-plane → 16 bytes");
    }

    {
        jitter_matrix_format out{};
        CHECK(!jitter_to_nozzle_format(_jit_sym_jit_matrix, 1, out), "unknown type should fail");
    }
    {
        jitter_matrix_format out{};
        CHECK(!jitter_to_nozzle_format(_jit_sym_char, 5, out), "char 5-plane should fail");
    }

    std::printf("\n=== nozzle_to_jitter_format tests (shared helper) ===\n");

    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_R8_UNORM);
        CHECK_EQ(m.type, jitter_type::char_type, "R8_UNORM → char");
        CHECK_EQ(m.planecount, 1, "R8_UNORM → 1 plane");
        CHECK_EQ(m.bytes_per_pixel, 1u, "R8_UNORM → 1 byte");
    }
    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_RG8_UNORM);
        CHECK_EQ(m.type, jitter_type::char_type, "RG8_UNORM → char");
        CHECK_EQ(m.planecount, 2, "RG8_UNORM → 2 planes");
        CHECK_EQ(m.bytes_per_pixel, 2u, "RG8_UNORM → 2 bytes");
    }
    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_RGBA8_UNORM);
        CHECK_EQ(m.type, jitter_type::char_type, "RGBA8_UNORM → char");
        CHECK_EQ(m.planecount, 4, "RGBA8_UNORM → 4 planes");
        CHECK_EQ(m.bytes_per_pixel, 4u, "RGBA8_UNORM → 4 bytes");
    }
    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_BGRA8_UNORM);
        CHECK_EQ(m.type, jitter_type::char_type, "BGRA8_UNORM → char");
        CHECK_EQ(m.planecount, 4, "BGRA8_UNORM → 4 planes");
        CHECK_EQ(m.bytes_per_pixel, 4u, "BGRA8_UNORM → 4 bytes");
    }
    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_RGBA8_SRGB);
        CHECK_EQ(m.type, jitter_type::char_type, "RGBA8_SRGB → char");
        CHECK_EQ(m.planecount, 4, "RGBA8_SRGB → 4 planes");
        CHECK_EQ(m.bytes_per_pixel, 4u, "RGBA8_SRGB → 4 bytes");
    }
    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_BGRA8_SRGB);
        CHECK_EQ(m.type, jitter_type::char_type, "BGRA8_SRGB → char");
        CHECK_EQ(m.planecount, 4, "BGRA8_SRGB → 4 planes");
        CHECK_EQ(m.bytes_per_pixel, 4u, "BGRA8_SRGB → 4 bytes");
    }

    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_R32_FLOAT);
        CHECK_EQ(m.type, jitter_type::float32_type, "R32_FLOAT → float32");
        CHECK_EQ(m.planecount, 1, "R32_FLOAT → 1 plane");
        CHECK_EQ(m.bytes_per_pixel, 4u, "R32_FLOAT → 4 bytes");
    }
    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_RG32_FLOAT);
        CHECK_EQ(m.type, jitter_type::float32_type, "RG32_FLOAT → float32");
        CHECK_EQ(m.planecount, 2, "RG32_FLOAT → 2 planes");
        CHECK_EQ(m.bytes_per_pixel, 8u, "RG32_FLOAT → 8 bytes");
    }
    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_RGBA32_FLOAT);
        CHECK_EQ(m.type, jitter_type::float32_type, "RGBA32_FLOAT → float32");
        CHECK_EQ(m.planecount, 4, "RGBA32_FLOAT → 4 planes");
        CHECK_EQ(m.bytes_per_pixel, 16u, "RGBA32_FLOAT → 16 bytes");
    }

    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_R16_FLOAT);
        CHECK_EQ(m.type, jitter_type::float32_type, "R16_FLOAT → float32");
        CHECK_EQ(m.planecount, 1, "R16_FLOAT → 1 plane");
        CHECK_EQ(m.bytes_per_pixel, 4u, "R16_FLOAT → 4 bytes");
    }
    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_RG16_FLOAT);
        CHECK_EQ(m.type, jitter_type::float32_type, "RG16_FLOAT → float32");
        CHECK_EQ(m.planecount, 2, "RG16_FLOAT → 2 planes");
        CHECK_EQ(m.bytes_per_pixel, 8u, "RG16_FLOAT → 8 bytes");
    }
    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_RGBA16_FLOAT);
        CHECK_EQ(m.type, jitter_type::float32_type, "RGBA16_FLOAT → float32");
        CHECK_EQ(m.planecount, 4, "RGBA16_FLOAT → 4 planes");
        CHECK_EQ(m.bytes_per_pixel, 16u, "RGBA16_FLOAT → 16 bytes");
    }

    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_R16_UNORM);
        CHECK_EQ(m.type, jitter_type::long_type, "R16_UNORM → long");
        CHECK_EQ(m.planecount, 1, "R16_UNORM → 1 plane");
        CHECK_EQ(m.bytes_per_pixel, 2u, "R16_UNORM → 2 bytes");
    }
    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_RG16_UNORM);
        CHECK_EQ(m.type, jitter_type::long_type, "RG16_UNORM → long");
        CHECK_EQ(m.planecount, 2, "RG16_UNORM → 2 planes");
        CHECK_EQ(m.bytes_per_pixel, 4u, "RG16_UNORM → 4 bytes");
    }
    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_RGBA16_UNORM);
        CHECK_EQ(m.type, jitter_type::long_type, "RGBA16_UNORM → long");
        CHECK_EQ(m.planecount, 4, "RGBA16_UNORM → 4 planes");
        CHECK_EQ(m.bytes_per_pixel, 8u, "RGBA16_UNORM → 8 bytes");
    }

    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_R32_UINT);
        CHECK_EQ(m.type, jitter_type::long_type, "R32_UINT → long");
        CHECK_EQ(m.planecount, 1, "R32_UINT → 1 plane");
        CHECK_EQ(m.bytes_per_pixel, 4u, "R32_UINT → 4 bytes");
    }
    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_RGBA32_UINT);
        CHECK_EQ(m.type, jitter_type::long_type, "RGBA32_UINT → long");
        CHECK_EQ(m.planecount, 4, "RGBA32_UINT → 4 planes");
        CHECK_EQ(m.bytes_per_pixel, 16u, "RGBA32_UINT → 16 bytes");
    }

    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_DEPTH32_FLOAT);
        CHECK_EQ(m.type, jitter_type::float32_type, "DEPTH32_FLOAT → float32");
        CHECK_EQ(m.planecount, 1, "DEPTH32_FLOAT → 1 plane");
        CHECK_EQ(m.bytes_per_pixel, 4u, "DEPTH32_FLOAT → 4 bytes");
    }

    {
        auto m = jit_nozzle::nozzle_to_jitter_format(static_cast<NozzleTextureFormat>(999));
        CHECK_EQ(m.type, jitter_type::char_type, "unknown → char fallback");
        CHECK_EQ(m.planecount, 4, "unknown → 4 planes fallback");
        CHECK_EQ(m.bytes_per_pixel, 4u, "unknown → 4 bytes fallback");
    }

    std::printf("\n=== Results: %d/%d passed ===\n", tests_run - tests_failed, tests_run);
    return tests_failed > 0 ? 1 : 0;
}
