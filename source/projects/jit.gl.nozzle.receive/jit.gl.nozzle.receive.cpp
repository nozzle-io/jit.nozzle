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

class jit_gl_bbb_nozzle_receive : public object<jit_gl_bbb_nozzle_receive> {
public:
	MIN_DESCRIPTION{"Receive OpenGL textures via nozzle (inter-process texture sharing)"};
	MIN_TAGS{"nozzle, gl, texture, sharing, jit"};
	MIN_AUTHOR{"ISHII 2bit"};

	inlet<> input{this, "(bang) poll for new frame"};
	outlet<> texture_out{this, "(jit_gl_texture) output texture name"};
	outlet<> info_out{this, "(anything) frame info events"};

	attribute<symbol> name_attr{this, "name", "nozzle_sender",
		description{"Sender name to connect to"},
		setter{[this](const atoms& args, int) -> atoms {
			if(args.size() > 0) {
				setup_receiver(std::string(symbol(args[0]).c_str()));
			}
			return args;
		}}
	};

	attribute<symbol> out_name{this, "out_name", "nozzle_recv_tex",
		description{"Name of the internal jit.gl.texture object output to downstream objects"}
	};

	attribute<int> timeout{this, "timeout", 0,
		description{"Frame acquisition timeout in ms (0 = no wait)"}
	};

	message<> bang_msg{this, "bang", "Poll for new frame",
		MIN_FUNCTION {
			poll_frame();
			return {};
		}
	};

	message<> draw_msg{this, "draw", "Acquire frame and copy to GL texture (call from render context)",
		MIN_FUNCTION {
			draw_frame();
			return {};
		}
	};

	message<> connect_msg{this, "connect", "Reconnect to sender",
		MIN_FUNCTION {
			setup_receiver(attr_to_string(name_attr));
			return {};
		}
	};

	message<> info_msg{this, "info", "Print sender info",
		MIN_FUNCTION {
			if(!receiver_) {
				cout << "jit.gl.nozzle.receive: not connected" << endl;
				return {};
			}
			NozzleConnectedSenderInfo info{};
			NozzleErrorCode err = nozzle_receiver_get_connected_info(receiver_, &info);
			if(err == NOZZLE_OK) {
				cout << "jit.gl.nozzle.receive connected to:" << endl;
				cout << "  name: " << (info.name ? info.name : "(null)") << endl;
				cout << "  app:  " << (info.application_name ? info.application_name : "(null)") << endl;
				cout << "  size: " << info.width << " x " << info.height << endl;
				cout << "  frames: " << info.frame_counter << endl;
			} else {
				cout << "jit.gl.nozzle.receive: not connected (error " << err << ")" << endl;
			}
			return {};
		}
	};

	jit_gl_bbb_nozzle_receive() {}
	~jit_gl_bbb_nozzle_receive() {
		std::lock_guard<std::mutex> lock(mutex_);
		if(receiver_) {
			nozzle_receiver_destroy(receiver_);
			receiver_ = nullptr;
		}
		if(output_tex_obj_) {
			c74::max::jit_object_free(output_tex_obj_);
			output_tex_obj_ = nullptr;
		}
	}

private:
	NozzleReceiver *receiver_{nullptr};
	c74::max::t_object *output_tex_obj_{nullptr};
	std::mutex mutex_;
	uint64_t frame_count_{0};
	uint32_t last_width_{0};
	uint32_t last_height_{0};

	void setup_receiver(const std::string& name) {
		if(name.empty()) return;

		NozzleReceiverDesc desc{};
		desc.name = name.c_str();
		desc.application_name = "jit.gl.nozzle.receive";
		desc.receive_mode = NOZZLE_RECEIVE_LATEST_ONLY;

		NozzleErrorCode err = nozzle_receiver_create(&desc, &receiver_);
		if(err != NOZZLE_OK) {
			cerr << "jit.gl.nozzle.receive: failed to connect to '" << name
			     << "' (error " << err << ")" << endl;
			receiver_ = nullptr;
		} else {
			cout << "jit.gl.nozzle.receive: connected to '" << name << "'" << endl;
		}
	}

	void poll_frame() {
		using namespace c74::max;

		{
			std::lock_guard<std::mutex> lock(mutex_);
			if(!receiver_) {
				setup_receiver(attr_to_string(name_attr));
			}
		}
		if(!receiver_) return;

		NozzleAcquireDesc acq{};
		acq.timeout_ms = static_cast<uint64_t>(timeout);

		NozzleFrame *frame = nullptr;
		NozzleErrorCode err = nozzle_receiver_acquire_frame(receiver_, &acq, &frame);

		if(err != NOZZLE_OK || !frame) return;

		NozzleFrameInfo finfo{};
		nozzle_frame_get_info(frame, &finfo);
		frame_count_ = finfo.frame_index;

		info_out.send("frame", static_cast<long long>(finfo.width),
			static_cast<long long>(finfo.height),
			static_cast<long long>(finfo.frame_index));

		nozzle_frame_release(frame);
	}

	void draw_frame() {
		using namespace c74::max;

		{
			std::lock_guard<std::mutex> lock(mutex_);
			if(!receiver_) {
				setup_receiver(attr_to_string(name_attr));
			}
		}
		if(!receiver_) return;

		NozzleAcquireDesc acq{};
		acq.timeout_ms = static_cast<uint64_t>(timeout);

		NozzleFrame *frame = nullptr;
		NozzleErrorCode err = nozzle_receiver_acquire_frame(receiver_, &acq, &frame);

		if(err != NOZZLE_OK || !frame) return;

		NozzleFrameInfo finfo{};
		nozzle_frame_get_info(frame, &finfo);

		uint32_t w = finfo.width;
		uint32_t h = finfo.height;

		{
			std::lock_guard<std::mutex> lock(mutex_);

			std::string tex_name_str = attr_to_string(out_name);

			if(!output_tex_obj_ || w != last_width_ || h != last_height_) {
				if(output_tex_obj_) {
					jit_object_free(output_tex_obj_);
					output_tex_obj_ = nullptr;
				}

				output_tex_obj_ = (t_object *)jit_object_new(
					gensym("jit_gl_texture"),
					gensym(tex_name_str.c_str())
				);

				if(!output_tex_obj_) {
					cerr << "jit.gl.nozzle.receive: failed to create jit.gl.texture" << endl;
					nozzle_frame_release(frame);
					return;
				}

				last_width_ = w;
				last_height_ = h;
			}

			long gl_id = jit_attr_getlong(output_tex_obj_, gensym("gl_name"));
			if(gl_id <= 0) {
				cerr << "jit.gl.nozzle.receive: internal texture has no valid GL name" << endl;
				nozzle_frame_release(frame);
				return;
			}

			err = nozzle_frame_copy_to_gl_texture(
				frame,
				static_cast<uint32_t>(gl_id),
				0x0DE1,  // GL_TEXTURE_2D
				w,
				h,
				NOZZLE_FORMAT_RGBA8_UNORM
			);

			if(err != NOZZLE_OK) {
				cerr << "jit.gl.nozzle.receive: copy to GL texture failed (error " << err << ")" << endl;
			}
		}

		frame_count_ = finfo.frame_index;

		nozzle_frame_release(frame);

		if(output_tex_obj_) {
			texture_out.send("jit_gl_texture", c74::min::symbol(attr_to_string(out_name).c_str()));
		}
	}
};

MIN_EXTERNAL(jit_gl_bbb_nozzle_receive);
