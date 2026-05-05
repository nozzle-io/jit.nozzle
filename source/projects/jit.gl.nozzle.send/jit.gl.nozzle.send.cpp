#include "c74_min.h"

extern "C" {
#include <nozzle/nozzle_c.h>
}

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
// Windows <GL/gl.h> only covers OpenGL 1.1; define modern constants directly.
#ifndef GL_R8
#define GL_R8 0x8229
#endif
#ifndef GL_RG8
#define GL_RG8 0x822B
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
#ifndef GL_RGBA16F
#define GL_RGBA16F 0x881A
#endif
#ifndef GL_R32F
#define GL_R32F 0x822E
#endif
#ifndef GL_RG32F
#define GL_RG32F 0x8230
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
#ifndef GL_RGBA16
#define GL_RGBA16 0x805B
#endif
#ifndef GL_R32UI
#define GL_R32UI 0x8236
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
#include <GL/gl.h>
#endif

#include <mutex>
#include <string>

using namespace c74::min;

static std::string to_string(const symbol &s) {
	return std::string((const char *)s);
}

static std::string attr_to_string(const attribute<symbol> &a) {
	const symbol &s = a;
	return to_string(s);
}

static NozzleTextureFormat gl_internal_format_to_nozzle(GLint gl_fmt) {
	switch (gl_fmt) {
		case GL_R8:            return NOZZLE_FORMAT_R8_UNORM;
		case GL_RG8:           return NOZZLE_FORMAT_RG8_UNORM;
		case GL_RGBA8:         return NOZZLE_FORMAT_RGBA8_UNORM;
#ifdef GL_BGRA8_EXT
		case GL_BGRA8_EXT:     return NOZZLE_FORMAT_BGRA8_UNORM;
#endif
#ifdef GL_SRGB8_ALPHA8
		case GL_SRGB8_ALPHA8:  return NOZZLE_FORMAT_RGBA8_SRGB;
#endif
		case GL_R16F:          return NOZZLE_FORMAT_R16_FLOAT;
		case GL_RG16F:         return NOZZLE_FORMAT_RG16_FLOAT;
		case GL_RGBA16F:       return NOZZLE_FORMAT_RGBA16_FLOAT;
		case GL_R32F:          return NOZZLE_FORMAT_R32_FLOAT;
		case GL_RG32F:         return NOZZLE_FORMAT_RG32_FLOAT;
		case GL_RGBA32F:       return NOZZLE_FORMAT_RGBA32_FLOAT;
		case GL_R16:           return NOZZLE_FORMAT_R16_UNORM;
		case GL_RG16:          return NOZZLE_FORMAT_RG16_UNORM;
		case GL_RGBA16:        return NOZZLE_FORMAT_RGBA16_UNORM;
#ifdef GL_R32UI
		case GL_R32UI:         return NOZZLE_FORMAT_R32_UINT;
#endif
#ifdef GL_RGBA32UI
		case GL_RGBA32UI:      return NOZZLE_FORMAT_RGBA32_UINT;
#endif
		case GL_DEPTH_COMPONENT32F: return NOZZLE_FORMAT_DEPTH32_FLOAT;
		default:               return NOZZLE_FORMAT_UNKNOWN;
	}
}

static NozzleTextureFormat query_gl_texture_format(uint32_t gl_id, uint32_t target) {
	glBindTexture(target, gl_id);
	GLint internal_format = 0;
	glGetTexLevelParameteriv(target, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);
	glBindTexture(target, 0);
	NozzleTextureFormat fmt = gl_internal_format_to_nozzle(internal_format);
	if (fmt == NOZZLE_FORMAT_UNKNOWN) {
		fmt = NOZZLE_FORMAT_RGBA8_UNORM;
	}
	return fmt;
}

class jit_gl_nozzle_send : public object<jit_gl_nozzle_send> {
public:
	MIN_DESCRIPTION{"Publish OpenGL textures via nozzle (inter-process texture sharing)"};
	MIN_TAGS{"nozzle, gl, texture, sharing, jit"};
	MIN_AUTHOR{"ISHII 2bit"};

	inlet<> input{this, "(int) GL texture ID to publish"};
	outlet<> frame_out{this, "(list) width height frame_index on publish"};

private:
	NozzleSender *sender_{nullptr};

public:
	attribute<symbol> name_attr{this, "name", "nozzle_sender",
		description{"Sender name (used for discovery by receivers)"},
		setter{[this](const atoms& args, int) -> atoms {
			if(args.size() > 0) {
				setup_sender(to_string(symbol(args[0])));
			}
			return args;
		}}
	};

	attribute<int> width_attr{this, "width", 0,
		description{"Texture width (required for publish)"}
	};

	attribute<int> height_attr{this, "height", 0,
		description{"Texture height (required for publish)"}
	};

	message<> int_msg{this, "int", "Publish a GL texture by its ID",
		MIN_FUNCTION {
			if(args.size() < 1) return {};
			int gl_id = args[0];
			publish_gl_texture(static_cast<uint32_t>(gl_id));
			return {};
		}
	};

	message<> jit_gl_texture_msg{this, "jit_gl_texture", "Receive jit.gl.texture name, look up and publish",
		MIN_FUNCTION {
			if(args.size() < 1) return {};
			publish_by_texture_name(to_string(symbol(args[0])));
			return {};
		}
	};

	message<> bang_msg{this, "bang", "Re-publish last texture",
		MIN_FUNCTION {
			if(cached_gl_texture_name_ != 0) {
				republish_cached();
			}
			return {};
		}
	};

	message<> dump_msg{this, "dump", "Print status",
		MIN_FUNCTION {
			cout << "jit.gl.nozzle.send status:" << endl;
			cout << "  name: " << attr_to_string(name_attr) << endl;
			cout << "  sender: " << (sender_ ? "active" : "inactive") << endl;
			cout << "  cached gl texture: " << cached_gl_texture_name_ << endl;
			cout << "  size: " << cached_width_ << " x " << cached_height_ << endl;
			cout << "  frames sent: " << frame_count_ << endl;
			return {};
		}
	};

	jit_gl_nozzle_send() {}
	~jit_gl_nozzle_send() {
		std::lock_guard<std::mutex> lock(mutex_);
		if(sender_) {
			nozzle_sender_destroy(sender_);
			sender_ = nullptr;
		}
	}

private:
	std::mutex mutex_;
	uint64_t frame_count_{0};
	uint32_t cached_gl_texture_name_{0};
	uint32_t cached_width_{0};
	uint32_t cached_height_{0};
	NozzleTextureFormat cached_format_{NOZZLE_FORMAT_RGBA8_UNORM};

	void setup_sender(const std::string& name) {
		if(name.empty()) return;

		NozzleSenderDesc desc{};
		desc.name = name.c_str();
		desc.application_name = "jit.gl.nozzle.send";
		desc.ring_buffer_size = 3;

		NozzleErrorCode err = nozzle_sender_create(&desc, &sender_);
		if(err != NOZZLE_OK) {
			cerr << "jit.gl.nozzle.send: failed to create sender '" << name
			     << "' (error " << err << ")" << endl;
			sender_ = nullptr;
		}
	}

	void publish_by_texture_name(const std::string &tex_name_str) {
		using namespace c74::max;

		void *tex_obj = jit_object_findregistered(gensym(tex_name_str.c_str()));
		if(!tex_obj) {
			cerr << "jit.gl.nozzle.send: texture '" << tex_name_str << "' not found" << endl;
			return;
		}
		if(jit_object_classname(tex_obj) != gensym("jit_gl_texture")) {
			cerr << "jit.gl.nozzle.send: object '" << tex_name_str << "' is not a jit.gl.texture" << endl;
			return;
		}

		long gl_id = (long)jit_object_method(tex_obj, gensym("gl_name"));
		if(gl_id <= 0) {
			cerr << "jit.gl.nozzle.send: texture '" << tex_name_str << "' has no valid GL name" << endl;
			return;
		}

		long w = (long)jit_object_method(tex_obj, _jit_sym_dim);
		long h = (long)jit_object_method(tex_obj, _jit_sym_dim, 1);

		if(w <= 0 || h <= 0) {
			cerr << "jit.gl.nozzle.send: texture has invalid dimensions" << endl;
			return;
		}

		{
			std::lock_guard<std::mutex> lock(mutex_);
			if(!sender_) {
				setup_sender(attr_to_string(name_attr));
			}
		}
		if(!sender_) return;

		NozzleTextureFormat fmt = query_gl_texture_format(static_cast<uint32_t>(gl_id), 0x0DE1);

		NozzleErrorCode nerr = nozzle_sender_publish_gl_texture(
			sender_,
			static_cast<uint32_t>(gl_id),
			0x0DE1,
			static_cast<uint32_t>(w),
			static_cast<uint32_t>(h),
			fmt
		);

		if(nerr != NOZZLE_OK) {
			cerr << "jit.gl.nozzle.send: publish failed (error " << nerr << ")" << endl;
			return;
		}

		cached_gl_texture_name_ = static_cast<uint32_t>(gl_id);
		cached_width_ = static_cast<uint32_t>(w);
		cached_height_ = static_cast<uint32_t>(h);
		cached_format_ = fmt;
		frame_count_++;

		frame_out.send({static_cast<int>(w), static_cast<int>(h), static_cast<long long>(frame_count_)});
	}

	void publish_gl_texture(uint32_t gl_id) {
		int w = width_attr;
		int h = height_attr;

		if(w <= 0 || h <= 0) {
			cerr << "jit.gl.nozzle.send: set @width and @height before publishing" << endl;
			return;
		}

		{
			std::lock_guard<std::mutex> lock(mutex_);
			if(!sender_) {
				setup_sender(attr_to_string(name_attr));
			}
		}
		if(!sender_) return;

	NozzleTextureFormat fmt = query_gl_texture_format(gl_id, 0x0DE1);

	NozzleErrorCode nerr = nozzle_sender_publish_gl_texture(
		sender_,
		gl_id,
		0x0DE1,
		static_cast<uint32_t>(w),
		static_cast<uint32_t>(h),
		fmt
	);

	if(nerr != NOZZLE_OK) {
		cerr << "jit.gl.nozzle.send: publish failed (error " << nerr << ")" << endl;
		return;
	}

	cached_gl_texture_name_ = gl_id;
	cached_width_ = static_cast<uint32_t>(w);
	cached_height_ = static_cast<uint32_t>(h);
	cached_format_ = fmt;
		frame_count_++;

		frame_out.send({w, h, static_cast<long long>(frame_count_)});
	}

	void republish_cached() {
		if(!sender_) return;

		NozzleTextureFormat fmt = query_gl_texture_format(cached_gl_texture_name_, 0x0DE1);
		cached_format_ = fmt;

		NozzleErrorCode nerr = nozzle_sender_publish_gl_texture(
			sender_,
			cached_gl_texture_name_,
			0x0DE1,
			cached_width_,
			cached_height_,
			fmt
		);

		if(nerr != NOZZLE_OK) {
			cerr << "jit.gl.nozzle.send: republish failed (error " << nerr << ")" << endl;
			return;
		}

		frame_count_++;
		frame_out.send({static_cast<int>(cached_width_), static_cast<int>(cached_height_), static_cast<long long>(frame_count_)});
	}
};

MIN_EXTERNAL(jit_gl_nozzle_send);
