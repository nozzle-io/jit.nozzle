#include "c74_min.h"

extern "C" {
#include <nozzle/nozzle_c.h>
}

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#include "jit_nozzle_gl_format_mapping.hpp"

#include <iomanip>
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

struct gl_texture_format_query {
	uint32_t internal_format{0};
	NozzleTextureFormat nozzle_format{NOZZLE_FORMAT_UNKNOWN};
};

static gl_texture_format_query query_gl_texture_format(uint32_t gl_id, uint32_t target) {
	GLint previous_binding = 0;
	if(target == 0x0DE1) { // GL_TEXTURE_2D
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_binding);
	}

	glBindTexture(target, gl_id);
	GLint internal_format = 0;
	glGetTexLevelParameteriv(target, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);

	if(target == 0x0DE1) {
		glBindTexture(target, static_cast<GLuint>(previous_binding));
	} else {
		glBindTexture(target, 0);
	}

	return gl_texture_format_query{
		static_cast<uint32_t>(internal_format),
		jit_nozzle::gl_internal_format_to_nozzle_format(static_cast<uint32_t>(internal_format))
	};
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
	NozzleTextureFormat cached_format_{NOZZLE_FORMAT_UNKNOWN};

	void setup_sender(const std::string& name) {
		if(name.empty()) return;

		NozzleSenderDesc desc{};
		desc.name = name.c_str();
		desc.application_name = "jit.gl.nozzle.send";
		desc.ring_buffer_size = 3;
    desc.fallback_flags_valid = 1;
    desc.fallback_flags = NOZZLE_FALLBACK_SAFE_DEFAULTS;

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

		gl_texture_format_query format_query = query_gl_texture_format(static_cast<uint32_t>(gl_id), 0x0DE1);
		if(format_query.nozzle_format == NOZZLE_FORMAT_UNKNOWN) {
			cerr << "jit.gl.nozzle.send: unsupported GL texture internal format 0x" << std::hex
			     << format_query.internal_format << std::dec << "; refusing to guess RGBA8_UNORM" << endl;
			return;
		}

		NozzleErrorCode nerr = nozzle_sender_publish_gl_texture(
			sender_,
			static_cast<uint32_t>(gl_id),
			0x0DE1,
			static_cast<uint32_t>(w),
			static_cast<uint32_t>(h),
			format_query.nozzle_format
		);

		if(nerr != NOZZLE_OK) {
			cerr << "jit.gl.nozzle.send: publish failed (error " << nerr << ")" << endl;
			return;
		}

		cached_gl_texture_name_ = static_cast<uint32_t>(gl_id);
		cached_width_ = static_cast<uint32_t>(w);
		cached_height_ = static_cast<uint32_t>(h);
		cached_format_ = format_query.nozzle_format;
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

		gl_texture_format_query format_query = query_gl_texture_format(gl_id, 0x0DE1);
		if(format_query.nozzle_format == NOZZLE_FORMAT_UNKNOWN) {
			cerr << "jit.gl.nozzle.send: unsupported GL texture internal format 0x" << std::hex
			     << format_query.internal_format << std::dec << "; refusing to guess RGBA8_UNORM" << endl;
			return;
		}

		NozzleErrorCode nerr = nozzle_sender_publish_gl_texture(
			sender_,
			gl_id,
			0x0DE1,
			static_cast<uint32_t>(w),
			static_cast<uint32_t>(h),
			format_query.nozzle_format
		);

		if(nerr != NOZZLE_OK) {
			cerr << "jit.gl.nozzle.send: publish failed (error " << nerr << ")" << endl;
			return;
		}

		cached_gl_texture_name_ = gl_id;
		cached_width_ = static_cast<uint32_t>(w);
		cached_height_ = static_cast<uint32_t>(h);
		cached_format_ = format_query.nozzle_format;
		frame_count_++;

		frame_out.send({w, h, static_cast<long long>(frame_count_)});
	}

	void republish_cached() {
		if(!sender_) return;

		gl_texture_format_query format_query = query_gl_texture_format(cached_gl_texture_name_, 0x0DE1);
		if(format_query.nozzle_format == NOZZLE_FORMAT_UNKNOWN) {
			cerr << "jit.gl.nozzle.send: unsupported GL texture internal format 0x" << std::hex
			     << format_query.internal_format << std::dec << "; refusing to guess RGBA8_UNORM on republish" << endl;
			return;
		}
		cached_format_ = format_query.nozzle_format;

		NozzleErrorCode nerr = nozzle_sender_publish_gl_texture(
			sender_,
			cached_gl_texture_name_,
			0x0DE1,
			cached_width_,
			cached_height_,
			format_query.nozzle_format
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
