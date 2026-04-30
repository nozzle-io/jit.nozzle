#include "c74_min.h"

extern "C" {
#include <nozzle/nozzle_c.h>
}

#include <cstring>
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

class jit_nozzle_receive : public object<jit_nozzle_receive> {
public:
	MIN_DESCRIPTION{"Receive jit.matrix data via nozzle (inter-process texture sharing)"};
	MIN_TAGS{"nozzle, matrix, sharing, jit"};
	MIN_AUTHOR{"ISHII 2bit"};

	inlet<> input{this, "(bang) poll for new frame"};
	outlet<> matrix_out{this, "(jit_matrix) output matrix with received data"};
	outlet<> info_out{this, "(anything) sender info events"};

private:
	NozzleReceiver *receiver_{nullptr};

public:
	attribute<symbol> name_attr{this, "name", "nozzle_sender",
		description{"Sender name to connect to"},
		setter{[this](const atoms& args, int) -> atoms {
			if(args.size() > 0) {
				setup_receiver(std::string(symbol(args[0]).c_str()));
			}
			return args;
		}}
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

	message<> connect_msg{this, "connect", "Reconnect to sender",
		MIN_FUNCTION {
			setup_receiver(attr_to_string(name_attr));
			return {};
		}
	};

	message<> info_msg{this, "info", "Print sender info",
		MIN_FUNCTION {
			if(!receiver_) {
				cout << "jit.nozzle.receive: not connected" << endl;
				return {};
			}
			NozzleConnectedSenderInfo info{};
			NozzleErrorCode err = nozzle_receiver_get_connected_info(receiver_, &info);
			if(err == NOZZLE_OK) {
				cout << "jit.nozzle.receive connected to:" << endl;
				cout << "  name: " << (info.name ? info.name : "(null)") << endl;
				cout << "  app:  " << (info.application_name ? info.application_name : "(null)") << endl;
				cout << "  size: " << info.width << " x " << info.height << endl;
				cout << "  frames: " << info.frame_counter << endl;
			} else {
				cout << "jit.nozzle.receive: not connected (error " << err << ")" << endl;
			}
			return {};
		}
	};

	jit_nozzle_receive() {}
	~jit_nozzle_receive() {
		std::lock_guard<std::mutex> lock(mutex_);
		if(receiver_) {
			nozzle_receiver_destroy(receiver_);
			receiver_ = nullptr;
		}
		if(output_matrix_) {
			c74::max::jit_object_free(output_matrix_);
			output_matrix_ = nullptr;
			matrix_name_ = nullptr;
		}
	}

private:
	c74::max::t_object *output_matrix_{nullptr};
	c74::max::t_symbol *matrix_name_{nullptr};
	std::mutex mutex_;
	uint64_t frame_count_{0};
	int acquire_log_throttle_{0};

	void setup_receiver(const std::string& name) {
		if(name.empty()) return;

		if(receiver_) {
			nozzle_receiver_destroy(receiver_);
			receiver_ = nullptr;
		}

		NozzleReceiverDesc desc{};
		desc.name = name.c_str();
		desc.application_name = "jit.nozzle.receive";
		desc.receive_mode = NOZZLE_RECEIVE_LATEST_ONLY;

		NozzleErrorCode err = nozzle_receiver_create(&desc, &receiver_);
		if(err != NOZZLE_OK) {
			cerr << "jit.nozzle.receive: failed to connect to '" << name
			     << "' (error " << err << ")" << endl;
			receiver_ = nullptr;
		} else {
			cout << "jit.nozzle.receive: connected to '" << name << "'" << endl;
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

		if(err != NOZZLE_OK || !frame) {
			if(acquire_log_throttle_ <= 0) {
				NozzleConnectedSenderInfo dbg_info{};
				NozzleErrorCode dbg_err = nozzle_receiver_get_connected_info(receiver_, &dbg_info);
				if(dbg_err == NOZZLE_OK) {
					cerr << "jit.nozzle.receive: acquire error " << err
					     << " sender_frames=" << dbg_info.frame_counter
					     << " size=" << dbg_info.width << "x" << dbg_info.height << endl;
				} else {
					cerr << "jit.nozzle.receive: acquire error " << err
					     << " get_info also failed (" << dbg_err << ")" << endl;
				}
				acquire_log_throttle_ = 30;
			} else {
				acquire_log_throttle_--;
			}
			return;
		}
		acquire_log_throttle_ = 0;

		NozzleFrameInfo finfo{};
		nozzle_frame_get_info(frame, &finfo);

		uint32_t w = finfo.width;
		uint32_t h = finfo.height;

		NozzleMappedPixels mapped{};
		err = nozzle_frame_lock_pixels(frame, &mapped);
		if(err != NOZZLE_OK) {
			cerr << "jit.nozzle.receive: lock pixels failed (error " << err << ")" << endl;
			nozzle_frame_release(frame);
			return;
		}

		bool has_data = false;
		{
			std::lock_guard<std::mutex> lock(mutex_);

			bool need_recreate = !output_matrix_;
			if(output_matrix_) {
				t_jit_matrix_info existing_info{};
				jit_object_method(output_matrix_, _jit_sym_getinfo, &existing_info);
				if(existing_info.dim[0] != static_cast<long>(w) ||
				   existing_info.dim[1] != static_cast<long>(h) ||
				   existing_info.planecount != 4 ||
				   existing_info.type != _jit_sym_char) {
					need_recreate = true;
				}
			}

			if(need_recreate) {
				if(output_matrix_) {
					jit_object_free(output_matrix_);
					output_matrix_ = nullptr;
					matrix_name_ = nullptr;
				}
				output_matrix_ = (t_object *)jit_object_new(_jit_sym_jit_matrix);
				if(!output_matrix_) {
					cerr << "jit.nozzle.receive: failed to create output matrix" << endl;
					matrix_name_ = nullptr;
					nozzle_frame_unlock_pixels(frame);
					nozzle_frame_release(frame);
					return;
				}

				t_jit_matrix_info new_info{};
				jit_matrix_info_default(&new_info);
				new_info.dimcount = 2;
				new_info.dim[0] = w;
				new_info.dim[1] = h;
				new_info.planecount = 4;
				new_info.type = _jit_sym_char;
				jit_object_method(output_matrix_, _jit_sym_setinfo, &new_info);
				jit_object_method(output_matrix_, _jit_sym_clear);

				matrix_name_ = jit_symbol_unique();
				jit_object_method(output_matrix_, _jit_sym_register, matrix_name_);
			}

			void *out_savelock = jit_object_method(output_matrix_, _jit_sym_lock, (void *)1);
			unsigned char *out_bp = nullptr;
			jit_object_method(output_matrix_, _jit_sym_getdata, &out_bp);

			if(out_bp) {
				has_data = true;
				t_jit_matrix_info out_info{};
				jit_object_method(output_matrix_, _jit_sym_getinfo, &out_info);
				uint32_t src_row_bytes = mapped.row_bytes;
				uint32_t dst_row_bytes = static_cast<uint32_t>(out_info.dimstride[1]);
				uint32_t copy_bytes = std::min(src_row_bytes, dst_row_bytes);
				copy_bytes = std::min(copy_bytes, w * 4u);

				auto *src = static_cast<const unsigned char *>(mapped.data);
				for(uint32_t y = 0; y < h; y++) {
					std::memcpy(out_bp + y * dst_row_bytes, src + y * src_row_bytes, copy_bytes);
				}
			}

			jit_object_method(output_matrix_, _jit_sym_lock, out_savelock);
		}

		nozzle_frame_unlock_pixels(frame);
		nozzle_frame_release(frame);

		frame_count_ = finfo.frame_index;

		if(output_matrix_ && matrix_name_) {
			cout << "DEBUG: acquire OK frame=" << finfo.frame_index
			     << " w=" << finfo.width << " h=" << finfo.height
			     << " has_data=" << has_data
			     << " matrix=" << matrix_name_->s_name << endl;
			matrix_out.send("jit_matrix", c74::min::symbol(matrix_name_->s_name));
		} else {
			cerr << "DEBUG: output_matrix_=" << (output_matrix_ ? "ok" : "null")
			     << " matrix_name_=" << (matrix_name_ ? matrix_name_->s_name : "null") << endl;
		}
	}
};

MIN_EXTERNAL(jit_nozzle_receive);
