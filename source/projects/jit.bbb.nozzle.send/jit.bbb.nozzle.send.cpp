#include "c74_min.h"

extern "C" {
#include <bbb/nozzle/nozzle_c.h>
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

class jit_bbb_nozzle_send : public object<jit_bbb_nozzle_send> {
public:
	MIN_DESCRIPTION{"Publish jit.matrix data via nozzle (inter-process texture sharing)"};
	MIN_TAGS{"nozzle, matrix, sharing, jit"};
	MIN_AUTHOR{"ISHII 2bit"};

	inlet<> input{this, "(jit_matrix) input matrix to send"};
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

	message<> jit_matrix_msg{this, "jit_matrix", "Receive and publish jit.matrix data",
		MIN_FUNCTION {
			if(args.size() < 1) return {};
			symbol matrix_name = args[0];
			send_matrix(matrix_name);
			return {};
		}
	};

	message<> dump_msg{this, "dump", "Print status",
		MIN_FUNCTION {
			cout << "jit.bbb.nozzle.send status:" << endl;
			cout << "  name: " << attr_to_string(name_attr) << endl;
			cout << "  sender: " << (sender_ ? "active" : "inactive") << endl;
			cout << "  frames sent: " << frame_count_ << endl;
			return {};
		}
	};

	jit_bbb_nozzle_send() {}
	~jit_bbb_nozzle_send() {
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

	void setup_sender(const std::string& name) {
		if(name.empty()) return;

		NozzleSenderDesc desc{};
		desc.name = name.c_str();
		desc.application_name = "jit.bbb.nozzle.send";
		desc.ring_buffer_size = 3;

		NozzleErrorCode err = nozzle_sender_create(&desc, &sender_);
		if(err != NOZZLE_OK) {
			cerr << "jit.bbb.nozzle.send: failed to create sender '" << name
			     << "' (error " << err << ")" << endl;
			sender_ = nullptr;
		}
	}

	void send_matrix(const symbol &matrix_name) {
		using namespace c74::max;

		// find the named jit.matrix
		void *matrix_obj = jit_object_findregistered((t_symbol *)matrix_name);
		if(!matrix_obj) {
			cerr << "jit.bbb.nozzle.send: matrix '" << matrix_name << "' not found" << endl;
			return;
		}

		// ensure sender exists
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if(!sender_) {
				setup_sender(attr_to_string(name_attr));
			}
		}
		if(!sender_) return;

		// lock matrix and get info + data
		void *savelock = jit_object_method(matrix_obj, _jit_sym_lock, (void *)1);
		t_jit_matrix_info minfo{};
		jit_object_method(matrix_obj, _jit_sym_getinfo, &minfo);
		unsigned char *bp = nullptr;
		jit_object_method(matrix_obj, _jit_sym_getdata, &bp);

		if(!bp || minfo.dim[0] <= 0 || minfo.dim[1] <= 0) {
			cerr << "jit.bbb.nozzle.send: matrix has no data" << endl;
			jit_object_method(matrix_obj, _jit_sym_lock, savelock);
			return;
		}

		uint32_t w = static_cast<uint32_t>(minfo.dim[0]);
		uint32_t h = static_cast<uint32_t>(minfo.dim[1]);
		uint32_t matrix_row_bytes = static_cast<uint32_t>(minfo.dimstride[1]);

		{
			std::lock_guard<std::mutex> lock(mutex_);

			// acquire writable frame from nozzle
			NozzleFrame *frame = nullptr;
			NozzleErrorCode err = nozzle_sender_acquire_writable_frame(
				sender_, w, h, NOZZLE_FORMAT_RGBA8_UNORM, &frame
			);
			if(err != NOZZLE_OK || !frame) {
				cerr << "jit.bbb.nozzle.send: acquire failed (error " << err << ")" << endl;
				jit_object_method(matrix_obj, _jit_sym_lock, savelock);
				return;
			}

			// lock writable pixels on nozzle frame
			NozzleMappedPixels mapped{};
			err = nozzle_frame_lock_writable_pixels(frame, &mapped);
			if(err != NOZZLE_OK) {
				cerr << "jit.bbb.nozzle.send: lock writable pixels failed (error " << err << ")" << endl;
				nozzle_frame_release(frame);
				jit_object_method(matrix_obj, _jit_sym_lock, savelock);
				return;
			}

			// copy matrix data row by row
			uint32_t src_row_bytes = matrix_row_bytes;
			uint32_t dst_row_bytes = mapped.row_bytes;
			uint32_t copy_bytes = std::min(src_row_bytes, dst_row_bytes);
			// RGBA8 = 4 bytes per pixel, cap at w * 4
			copy_bytes = std::min(copy_bytes, w * 4u);

			auto *src = static_cast<const unsigned char *>(bp);
			auto *dst = static_cast<unsigned char *>(mapped.data);
			for(uint32_t y = 0; y < h; y++) {
				std::memcpy(dst + y * dst_row_bytes, src + y * src_row_bytes, copy_bytes);
			}

			nozzle_frame_unlock_writable_pixels(frame);

			// commit frame
			err = nozzle_sender_commit_frame(sender_, frame);
			if(err != NOZZLE_OK) {
				cerr << "jit.bbb.nozzle.send: commit failed (error " << err << ")" << endl;
			}

			// get frame info for output
			if(err == NOZZLE_OK) {
				NozzleFrameInfo finfo{};
				nozzle_frame_get_info(frame, &finfo);
				frame_count_ = finfo.frame_index;
			}
		}

		// unlock matrix
		jit_object_method(matrix_obj, _jit_sym_lock, savelock);

		// output frame info
		frame_out.send({static_cast<int>(w), static_cast<int>(h), static_cast<long long>(frame_count_)});
	}
};

MIN_EXTERNAL(jit_bbb_nozzle_send);
