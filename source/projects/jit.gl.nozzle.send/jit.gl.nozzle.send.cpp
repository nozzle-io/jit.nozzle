#include "c74_min.h"

extern "C" {
#include <bbb/nozzle/nozzle_c.h>
}

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

class jit_gl_bbb_nozzle_send : public object<jit_gl_bbb_nozzle_send> {
public:
	MIN_DESCRIPTION{"Publish OpenGL textures via nozzle (inter-process texture sharing)"};
	MIN_TAGS{"nozzle, gl, texture, sharing, jit"};
	MIN_AUTHOR{"ISHII 2bit"};

	inlet<> input{this, "(int) GL texture ID to publish"};
	outlet<> frame_out{this, "(list) width height frame_index on publish"};

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

	jit_gl_bbb_nozzle_send() {}
	~jit_gl_bbb_nozzle_send() {
		std::lock_guard<std::mutex> lock(mutex_);
		if(sender_) {
			nozzle_sender_destroy(sender_);
			sender_ = nullptr;
		}
	}

private:
	NozzleSender *sender_{nullptr};
	std::mutex mutex_;
	uint64_t frame_count_{0};
	uint32_t cached_gl_texture_name_{0};
	uint32_t cached_width_{0};
	uint32_t cached_height_{0};

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

		NozzleErrorCode nerr = nozzle_sender_publish_gl_texture(
			sender_,
			static_cast<uint32_t>(gl_id),
			0x0DE1,
			static_cast<uint32_t>(w),
			static_cast<uint32_t>(h),
			NOZZLE_FORMAT_RGBA8_UNORM
		);

		if(nerr != NOZZLE_OK) {
			cerr << "jit.gl.nozzle.send: publish failed (error " << nerr << ")" << endl;
			return;
		}

		cached_gl_texture_name_ = static_cast<uint32_t>(gl_id);
		cached_width_ = static_cast<uint32_t>(w);
		cached_height_ = static_cast<uint32_t>(h);
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

		NozzleErrorCode nerr = nozzle_sender_publish_gl_texture(
			sender_,
			gl_id,
			0x0DE1,
			static_cast<uint32_t>(w),
			static_cast<uint32_t>(h),
			NOZZLE_FORMAT_RGBA8_UNORM
		);

		if(nerr != NOZZLE_OK) {
			cerr << "jit.gl.nozzle.send: publish failed (error " << nerr << ")" << endl;
			return;
		}

		cached_gl_texture_name_ = gl_id;
		cached_width_ = static_cast<uint32_t>(w);
		cached_height_ = static_cast<uint32_t>(h);
		frame_count_++;

		frame_out.send({w, h, static_cast<long long>(frame_count_)});
	}

	void republish_cached() {
		if(!sender_) return;

		NozzleErrorCode nerr = nozzle_sender_publish_gl_texture(
			sender_,
			cached_gl_texture_name_,
			0x0DE1,
			cached_width_,
			cached_height_,
			NOZZLE_FORMAT_RGBA8_UNORM
		);

		if(nerr != NOZZLE_OK) {
			cerr << "jit.gl.nozzle.send: republish failed (error " << nerr << ")" << endl;
			return;
		}

		frame_count_++;
		frame_out.send({static_cast<int>(cached_width_), static_cast<int>(cached_height_), static_cast<long long>(frame_count_)});
	}
};

MIN_EXTERNAL(jit_gl_bbb_nozzle_send);
