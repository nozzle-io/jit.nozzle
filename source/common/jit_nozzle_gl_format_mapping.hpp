#pragma once

#include <cstdint>

extern "C" {
#include <nozzle/nozzle_c.h>
}

#ifndef GL_R8
#define GL_R8 0x8229
#endif
#ifndef GL_RG8
#define GL_RG8 0x822B
#endif
#ifndef GL_RGB8
#define GL_RGB8 0x8051
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_BGRA8_EXT
#define GL_BGRA8_EXT 0x93A1
#endif
#ifndef GL_SRGB8_ALPHA8
#define GL_SRGB8_ALPHA8 0x8C43
#endif
#ifndef GL_R16F
#define GL_R16F 0x822D
#endif
#ifndef GL_RG16F
#define GL_RG16F 0x822F
#endif
#ifndef GL_RGB16F
#define GL_RGB16F 0x881B
#endif
#ifndef GL_RGBA16F
#define GL_RGBA16F 0x881A
#endif
#ifndef GL_R32F
#define GL_R32F 0x822E
#endif
#ifndef GL_RG32F
#define GL_RG32F 0x8230
#endif
#ifndef GL_RGB32F
#define GL_RGB32F 0x8815
#endif
#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif
#ifndef GL_R16
#define GL_R16 0x822A
#endif
#ifndef GL_RG16
#define GL_RG16 0x822C
#endif
#ifndef GL_RGB16
#define GL_RGB16 0x8054
#endif
#ifndef GL_RGBA16
#define GL_RGBA16 0x805B
#endif
#ifndef GL_R32UI
#define GL_R32UI 0x8236
#endif
#ifndef GL_RGB32UI
#define GL_RGB32UI 0x8D71
#endif
#ifndef GL_RGBA32UI
#define GL_RGBA32UI 0x8D70
#endif
#ifndef GL_DEPTH_COMPONENT32F
#define GL_DEPTH_COMPONENT32F 0x8CAC
#endif
#ifndef GL_TEXTURE_INTERNAL_FORMAT
#define GL_TEXTURE_INTERNAL_FORMAT 0x1003
#endif
#ifndef GL_TEXTURE_BINDING_2D
#define GL_TEXTURE_BINDING_2D 0x8069
#endif

namespace jit_nozzle {

inline NozzleTextureFormat gl_internal_format_to_nozzle_format(std::uint32_t gl_internal_format) noexcept {
	switch(gl_internal_format) {
		case GL_R8: return NOZZLE_FORMAT_R8_UNORM;
		case GL_RG8: return NOZZLE_FORMAT_RG8_UNORM;
		case GL_RGB8: return NOZZLE_FORMAT_RGB8_UNORM;
		case GL_RGBA8: return NOZZLE_FORMAT_RGBA8_UNORM;
		case GL_BGRA8_EXT: return NOZZLE_FORMAT_BGRA8_UNORM;
		case GL_SRGB8_ALPHA8: return NOZZLE_FORMAT_RGBA8_SRGB;
		case GL_R16F: return NOZZLE_FORMAT_R16_FLOAT;
		case GL_RG16F: return NOZZLE_FORMAT_RG16_FLOAT;
		case GL_RGB16F: return NOZZLE_FORMAT_RGB16_FLOAT;
		case GL_RGBA16F: return NOZZLE_FORMAT_RGBA16_FLOAT;
		case GL_R32F: return NOZZLE_FORMAT_R32_FLOAT;
		case GL_RG32F: return NOZZLE_FORMAT_RG32_FLOAT;
		case GL_RGB32F: return NOZZLE_FORMAT_RGB32_FLOAT;
		case GL_RGBA32F: return NOZZLE_FORMAT_RGBA32_FLOAT;
		case GL_R16: return NOZZLE_FORMAT_R16_UNORM;
		case GL_RG16: return NOZZLE_FORMAT_RG16_UNORM;
		case GL_RGB16: return NOZZLE_FORMAT_RGB16_UNORM;
		case GL_RGBA16: return NOZZLE_FORMAT_RGBA16_UNORM;
		case GL_R32UI: return NOZZLE_FORMAT_R32_UINT;
		case GL_RGB32UI: return NOZZLE_FORMAT_RGB32_UINT;
		case GL_RGBA32UI: return NOZZLE_FORMAT_RGBA32_UINT;
		case GL_DEPTH_COMPONENT32F: return NOZZLE_FORMAT_DEPTH32_FLOAT;
		default: return NOZZLE_FORMAT_UNKNOWN;
	}
}

inline bool nozzle_frame_format_to_gl_copy_format(
	NozzleTextureFormat frame_format,
	NozzleTextureFormat &out_copy_format) noexcept
{
	switch(frame_format) {
		case NOZZLE_FORMAT_R8_UNORM:
		case NOZZLE_FORMAT_RG8_UNORM:
		case NOZZLE_FORMAT_RGBA8_UNORM:
		case NOZZLE_FORMAT_BGRA8_UNORM:
		case NOZZLE_FORMAT_R16_UNORM:
		case NOZZLE_FORMAT_RG16_UNORM:
		case NOZZLE_FORMAT_RGBA16_UNORM:
		case NOZZLE_FORMAT_R16_FLOAT:
		case NOZZLE_FORMAT_RG16_FLOAT:
		case NOZZLE_FORMAT_RGBA16_FLOAT:
		case NOZZLE_FORMAT_R32_FLOAT:
		case NOZZLE_FORMAT_RG32_FLOAT:
		case NOZZLE_FORMAT_RGBA32_FLOAT:
		case NOZZLE_FORMAT_R32_UINT:
		case NOZZLE_FORMAT_RGBA32_UINT:
			out_copy_format = frame_format;
			return true;
		case NOZZLE_FORMAT_RGBA8_SRGB:
			out_copy_format = NOZZLE_FORMAT_RGBA8_UNORM;
			return true;
		case NOZZLE_FORMAT_BGRA8_SRGB:
			out_copy_format = NOZZLE_FORMAT_BGRA8_UNORM;
			return true;
		case NOZZLE_FORMAT_DEPTH32_FLOAT:
			// Depth frames use R32_FLOAT only as a GL copy target compatibility policy.
			// This is not a semantic conversion from depth to color.
			out_copy_format = NOZZLE_FORMAT_R32_FLOAT;
			return true;
		default:
			out_copy_format = NOZZLE_FORMAT_UNKNOWN;
			return false;
	}
}

} // namespace jit_nozzle
