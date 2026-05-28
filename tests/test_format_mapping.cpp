#include <cassert>
#include <cstdio>
#include <cstring>

extern "C" {
#include <nozzle/nozzle_c.h>
}

#include "jit_nozzle_format_mapping.hpp"
#include "jit_nozzle_gl_format_mapping.hpp"

using jit_nozzle::jitter_type;
using jit_nozzle::send_format_mapping;

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
    std::printf("=== jitter_to_nozzle_format tests (send, shared helper) ===\n");

    {
        send_format_mapping out{};
        CHECK(jit_nozzle::jitter_to_nozzle_format(jitter_type::char_type, 1, out), "char 1-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_R8_UNORM, "char 1-plane → R8_UNORM");
        CHECK_EQ(out.source_bytes_per_pixel, 1u, "char 1-plane → 1 source byte");
    }
    {
        send_format_mapping out{};
        CHECK(jit_nozzle::jitter_to_nozzle_format(jitter_type::char_type, 2, out), "char 2-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_RG8_UNORM, "char 2-plane → RG8_UNORM");
        CHECK_EQ(out.source_bytes_per_pixel, 2u, "char 2-plane → 2 source bytes");
    }
    {
        send_format_mapping out{};
        CHECK(jit_nozzle::jitter_to_nozzle_format(jitter_type::char_type, 3, out), "char 3-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_RGB8_UNORM, "char 3-plane → RGB8_UNORM");
        CHECK_EQ(out.source_bytes_per_pixel, 3u, "char 3-plane → 3 source bytes");
    }
    {
        send_format_mapping out{};
        CHECK(jit_nozzle::jitter_to_nozzle_format(jitter_type::char_type, 4, out), "char 4-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_RGBA8_UNORM, "char 4-plane → RGBA8_UNORM");
        CHECK_EQ(out.source_bytes_per_pixel, 4u, "char 4-plane → 4 source bytes");
    }
    {
        send_format_mapping out{};
        CHECK(jit_nozzle::jitter_to_nozzle_format(jitter_type::float32_type, 1, out), "float32 1-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_R32_FLOAT, "float32 1-plane full precision send policy → R32_FLOAT");
        CHECK_EQ(out.source_bytes_per_pixel, 4u, "float32 1-plane → 4 source bytes");
    }
    {
        send_format_mapping out{};
        CHECK(jit_nozzle::jitter_to_nozzle_format(jitter_type::float32_type, 2, out), "float32 2-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_RG32_FLOAT, "float32 2-plane full precision send policy → RG32_FLOAT");
        CHECK_EQ(out.source_bytes_per_pixel, 8u, "float32 2-plane → 8 source bytes");
    }
    {
        send_format_mapping out{};
        CHECK(jit_nozzle::jitter_to_nozzle_format(jitter_type::float32_type, 3, out), "float32 3-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_RGB32_FLOAT, "float32 3-plane requested semantic send policy → RGB32_FLOAT");
        CHECK_EQ(out.source_bytes_per_pixel, 12u, "float32 3-plane → 12 source bytes");
    }
    {
        send_format_mapping out{};
        CHECK(jit_nozzle::jitter_to_nozzle_format(jitter_type::float32_type, 4, out), "float32 4-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_RGBA32_FLOAT, "float32 4-plane full precision send policy → RGBA32_FLOAT");
        CHECK_EQ(out.source_bytes_per_pixel, 16u, "float32 4-plane → 16 source bytes");
    }
    {
        const NozzleTextureFormat half_formats[] = {
            NOZZLE_FORMAT_R16_FLOAT,
            NOZZLE_FORMAT_RG16_FLOAT,
            NOZZLE_FORMAT_RGBA16_FLOAT,
        };
        for (int planes = 1; planes <= 4; ++planes) {
            send_format_mapping out{};
            CHECK(jit_nozzle::jitter_to_nozzle_format(jitter_type::float32_type, planes, out),
                  "float32 send policy should accept 1-4 planes");
            for (NozzleTextureFormat half_format : half_formats) {
                CHECK(out.nozzle_fmt != half_format,
                      "float32 send policy must not request 16F without explicit numeric conversion");
            }
        }
    }

    {
        send_format_mapping out{};
        CHECK(jit_nozzle::jitter_to_nozzle_format(jitter_type::long_type, 1, out), "long 1-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_R32_UINT, "long 1-plane → R32_UINT");
        CHECK_EQ(out.source_bytes_per_pixel, 4u, "long 1-plane → 4 source bytes");
    }
    {
        send_format_mapping out{};
        CHECK(jit_nozzle::jitter_to_nozzle_format(jitter_type::long_type, 2, out), "long 2-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_RGBA32_UINT, "long 2-plane → RGBA32_UINT");
        CHECK_EQ(out.source_bytes_per_pixel, 8u, "long 2-plane → 8 source bytes");
    }
    {
        send_format_mapping out{};
        CHECK(jit_nozzle::jitter_to_nozzle_format(jitter_type::long_type, 3, out), "long 3-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_RGB32_UINT, "long 3-plane → RGB32_UINT");
        CHECK_EQ(out.source_bytes_per_pixel, 12u, "long 3-plane → 12 source bytes");
    }
    {
        send_format_mapping out{};
        CHECK(jit_nozzle::jitter_to_nozzle_format(jitter_type::long_type, 4, out), "long 4-plane should succeed");
        CHECK_EQ(out.nozzle_fmt, NOZZLE_FORMAT_RGBA32_UINT, "long 4-plane → RGBA32_UINT");
        CHECK_EQ(out.source_bytes_per_pixel, 16u, "long 4-plane → 16 source bytes");
    }

    {
        send_format_mapping out{};
        CHECK(!jit_nozzle::jitter_to_nozzle_format(jitter_type::char_type, 5, out), "char 5-plane should fail");
    }

    {
        send_format_mapping out{};
        auto invalid_type = static_cast<jitter_type>(99);
        CHECK(!jit_nozzle::jitter_to_nozzle_format(invalid_type, 1, out), "unknown jitter_type should fail");
        CHECK(!jit_nozzle::jitter_to_nozzle_format(invalid_type, 4, out), "unknown jitter_type 4-plane should fail");
    }

    std::printf("\n=== nozzle_to_jitter_format tests (receive, shared helper) ===\n");

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
        CHECK_EQ(m.type, jitter_type::float32_type, "R16_FLOAT receive widen policy → float32");
        CHECK_EQ(m.planecount, 1, "R16_FLOAT → 1 plane");
        CHECK_EQ(m.bytes_per_pixel, 4u, "R16_FLOAT receive widens to 4 float32 bytes");
    }
    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_RG16_FLOAT);
        CHECK_EQ(m.type, jitter_type::float32_type, "RG16_FLOAT receive widen policy → float32");
        CHECK_EQ(m.planecount, 2, "RG16_FLOAT → 2 planes");
        CHECK_EQ(m.bytes_per_pixel, 8u, "RG16_FLOAT receive widens to 8 float32 bytes");
    }
    {
        auto m = jit_nozzle::nozzle_to_jitter_format(NOZZLE_FORMAT_RGBA16_FLOAT);
        CHECK_EQ(m.type, jitter_type::float32_type, "RGBA16_FLOAT receive widen policy → float32");
        CHECK_EQ(m.planecount, 4, "RGBA16_FLOAT → 4 planes");
        CHECK_EQ(m.bytes_per_pixel, 16u, "RGBA16_FLOAT receive widens to 16 float32 bytes");
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


    std::printf("\n=== GL internal format mapping tests (jit.gl.nozzle.send) ===\n");

    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_R8), NOZZLE_FORMAT_R8_UNORM, "GL_R8 → R8_UNORM");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_RG8), NOZZLE_FORMAT_RG8_UNORM, "GL_RG8 → RG8_UNORM");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_RGB8), NOZZLE_FORMAT_RGB8_UNORM, "GL_RGB8 → RGB8_UNORM");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_RGBA8), NOZZLE_FORMAT_RGBA8_UNORM, "GL_RGBA8 → RGBA8_UNORM");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_BGRA8_EXT), NOZZLE_FORMAT_BGRA8_UNORM, "GL_BGRA8_EXT → BGRA8_UNORM");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_SRGB8_ALPHA8), NOZZLE_FORMAT_RGBA8_SRGB, "GL_SRGB8_ALPHA8 → RGBA8_SRGB");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_R16F), NOZZLE_FORMAT_R16_FLOAT, "GL_R16F → R16_FLOAT");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_RG16F), NOZZLE_FORMAT_RG16_FLOAT, "GL_RG16F → RG16_FLOAT");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_RGB16F), NOZZLE_FORMAT_RGB16_FLOAT, "GL_RGB16F → RGB16_FLOAT");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_RGBA16F), NOZZLE_FORMAT_RGBA16_FLOAT, "GL_RGBA16F → RGBA16_FLOAT");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_R32F), NOZZLE_FORMAT_R32_FLOAT, "GL_R32F → R32_FLOAT");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_RG32F), NOZZLE_FORMAT_RG32_FLOAT, "GL_RG32F → RG32_FLOAT");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_RGB32F), NOZZLE_FORMAT_RGB32_FLOAT, "GL_RGB32F → RGB32_FLOAT");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_RGBA32F), NOZZLE_FORMAT_RGBA32_FLOAT, "GL_RGBA32F → RGBA32_FLOAT");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_R16), NOZZLE_FORMAT_R16_UNORM, "GL_R16 → R16_UNORM");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_RG16), NOZZLE_FORMAT_RG16_UNORM, "GL_RG16 → RG16_UNORM");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_RGB16), NOZZLE_FORMAT_RGB16_UNORM, "GL_RGB16 → RGB16_UNORM");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_RGBA16), NOZZLE_FORMAT_RGBA16_UNORM, "GL_RGBA16 → RGBA16_UNORM");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_R32UI), NOZZLE_FORMAT_R32_UINT, "GL_R32UI → R32_UINT");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_RGB32UI), NOZZLE_FORMAT_RGB32_UINT, "GL_RGB32UI → RGB32_UINT");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_RGBA32UI), NOZZLE_FORMAT_RGBA32_UINT, "GL_RGBA32UI → RGBA32_UINT");
    CHECK_EQ(jit_nozzle::gl_internal_format_to_nozzle_format(GL_DEPTH_COMPONENT32F), NOZZLE_FORMAT_DEPTH32_FLOAT, "GL_DEPTH_COMPONENT32F → DEPTH32_FLOAT");
    {
        NozzleTextureFormat mapped = jit_nozzle::gl_internal_format_to_nozzle_format(0xDEADu);
        CHECK_EQ(mapped, NOZZLE_FORMAT_UNKNOWN, "unknown GL internal format → UNKNOWN");
        CHECK(mapped != NOZZLE_FORMAT_RGBA8_UNORM, "unknown GL internal format must not fall back to RGBA8_UNORM");
    }

    std::printf("\n=== GL frame copy format policy tests (jit.gl.nozzle.receive) ===\n");

    {
        NozzleTextureFormat copy_format = NOZZLE_FORMAT_UNKNOWN;
        CHECK(jit_nozzle::nozzle_frame_format_to_gl_copy_format(NOZZLE_FORMAT_RGBA8_UNORM, copy_format), "RGBA8_UNORM GL copy should be supported");
        CHECK_EQ(copy_format, NOZZLE_FORMAT_RGBA8_UNORM, "RGBA8_UNORM GL copy → same format");
    }
    {
        NozzleTextureFormat copy_format = NOZZLE_FORMAT_UNKNOWN;
        CHECK(jit_nozzle::nozzle_frame_format_to_gl_copy_format(NOZZLE_FORMAT_BGRA8_SRGB, copy_format), "BGRA8_SRGB GL copy should be supported via unorm target");
        CHECK_EQ(copy_format, NOZZLE_FORMAT_BGRA8_UNORM, "BGRA8_SRGB GL copy → BGRA8_UNORM target");
    }
    {
        NozzleTextureFormat copy_format = NOZZLE_FORMAT_UNKNOWN;
        CHECK(jit_nozzle::nozzle_frame_format_to_gl_copy_format(NOZZLE_FORMAT_DEPTH32_FLOAT, copy_format), "DEPTH32_FLOAT GL copy should be supported via R32_FLOAT target");
        CHECK_EQ(copy_format, NOZZLE_FORMAT_R32_FLOAT, "DEPTH32_FLOAT GL copy → R32_FLOAT copy target");
    }
    {
        NozzleTextureFormat copy_format = NOZZLE_FORMAT_RGBA8_UNORM;
        CHECK(!jit_nozzle::nozzle_frame_format_to_gl_copy_format(NOZZLE_FORMAT_RGB8_UNORM, copy_format), "RGB8_UNORM GL receive copy is unsupported until explicitly implemented");
        CHECK_EQ(copy_format, NOZZLE_FORMAT_UNKNOWN, "unsupported GL receive copy resets output to UNKNOWN");
        CHECK(copy_format != NOZZLE_FORMAT_RGBA8_UNORM, "unsupported GL receive copy must not fall back to RGBA8_UNORM");
    }
    {
        NozzleTextureFormat copy_format = NOZZLE_FORMAT_RGBA8_UNORM;
        CHECK(!jit_nozzle::nozzle_frame_format_to_gl_copy_format(static_cast<NozzleTextureFormat>(999), copy_format), "unknown nozzle frame format GL copy should fail");
        CHECK_EQ(copy_format, NOZZLE_FORMAT_UNKNOWN, "unknown nozzle frame format GL copy resets output to UNKNOWN");
        CHECK(copy_format != NOZZLE_FORMAT_RGBA8_UNORM, "unknown nozzle frame format must not fall back to RGBA8_UNORM");
    }

    std::printf("\n=== Results: %d/%d passed ===\n", tests_run - tests_failed, tests_run);
    return tests_failed > 0 ? 1 : 0;
}
