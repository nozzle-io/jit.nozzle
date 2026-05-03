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

struct jitter_matrix_format {
	NozzleTextureFormat nozzle_fmt;
	uint32_t bytes_per_pixel;
};

static bool jitter_to_nozzle_format(
	c74::max::t_symbol *type, int planecount, jitter_matrix_format &out
) {
	using namespace c74::max;
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
		// IOSurface doesn't support UINT formats, treat as float32 (same byte width)
		switch(planecount) {
			case 1: out = {NOZZLE_FORMAT_R32_FLOAT, 4}; return true;
			case 2: out = {NOZZLE_FORMAT_RG32_FLOAT, 8}; return true;
			case 3: out = {NOZZLE_FORMAT_RGBA32_FLOAT, 12}; return true;
			case 4: out = {NOZZLE_FORMAT_RGBA32_FLOAT, 16}; return true;
		}
	}
	return false;
}

class jit_nozzle_send : public object<jit_nozzle_send> {
public:
	MIN_DESCRIPTION{"Publish jit.matrix data via nozzle (inter-process texture sharing)"};
	MIN_TAGS{"nozzle, matrix, sharing, jit"};
	MIN_AUTHOR{"ISHII 2bit"};

	inlet<> input{this, "(jit_matrix) input matrix to send"};
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
			cout << "jit.nozzle.send status:" << endl;
			cout << "  name: " << attr_to_string(name_attr) << endl;
			cout << "  sender: " << (sender_ ? "active" : "inactive") << endl;
			cout << "  frames sent: " << frame_count_ << endl;
			return {};
		}
	};

	jit_nozzle_send() {}
	~jit_nozzle_send() {
		std::lock_guard<std::mutex> lock(mutex_);
		if(sender_) {
			nozzle_sender_destroy(sender_);
			sender_ = nullptr;
		}
	}

private:
	std::mutex mutex_;
	uint64_t frame_count_{0};

	void setup_sender(const std::string& name) {
		if(name.empty()) return;

		if(sender_) {
			nozzle_sender_destroy(sender_);
			sender_ = nullptr;
		}

		NozzleSenderDesc desc{};
		desc.name = name.c_str();
		desc.application_name = "jit.nozzle.send";
		desc.ring_buffer_size = 3;

		NozzleErrorCode err = nozzle_sender_create(&desc, &sender_);
		if(err != NOZZLE_OK) {
			cerr << "jit.nozzle.send: failed to create sender '" << name
			     << "' (error " << err << ")" << endl;
			sender_ = nullptr;
		}
	}

	void send_matrix(const symbol &matrix_name) {
		using namespace c74::max;

		// find the named jit.matrix
		void *matrix_obj = jit_object_findregistered((t_symbol *)matrix_name);
		if(!matrix_obj) {
			cerr << "jit.nozzle.send: matrix '" << matrix_name << "' not found" << endl;
			return;
		}
		if(jit_object_classname(matrix_obj) != _jit_sym_jit_matrix) {
			cerr << "jit.nozzle.send: object '" << matrix_name << "' is not a jit.matrix" << endl;
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
			cerr << "jit.nozzle.send: matrix has no data" << endl;
			jit_object_method(matrix_obj, _jit_sym_lock, savelock);
			return;
		}

		uint32_t w = static_cast<uint32_t>(minfo.dim[0]);
		uint32_t h = static_cast<uint32_t>(minfo.dim[1]);
		uint32_t matrix_row_bytes = static_cast<uint32_t>(minfo.dimstride[1]);

		jitter_matrix_format jfmt{};
		if(!jitter_to_nozzle_format(minfo.type, minfo.planecount, jfmt)) {
			cerr << "jit.nozzle.send: unsupported matrix type "
			     << minfo.type->s_name << " planecount=" << minfo.planecount << endl;
			jit_object_method(matrix_obj, _jit_sym_lock, savelock);
			return;
		}

		{
			std::lock_guard<std::mutex> lock(mutex_);

			// acquire writable frame from nozzle
			NozzleFrame *frame = nullptr;
			NozzleErrorCode err = nozzle_sender_acquire_writable_frame(
				sender_, w, h, jfmt.nozzle_fmt, &frame
			);
			if(err != NOZZLE_OK || !frame) {
				cerr << "jit.nozzle.send: acquire failed (error " << err << ")" << endl;
				jit_object_method(matrix_obj, _jit_sym_lock, savelock);
				return;
			}

			// lock writable pixels on nozzle frame
			NozzleMappedPixels mapped{};
			err = nozzle_frame_lock_writable_pixels_with_origin(
				frame, NOZZLE_ORIGIN_TOP_LEFT, &mapped);
			if(err != NOZZLE_OK) {
				cerr << "jit.nozzle.send: lock writable pixels failed (error " << err << ")" << endl;
				nozzle_frame_release(frame);
				jit_object_method(matrix_obj, _jit_sym_lock, savelock);
				return;
			}

				auto *src = static_cast<const unsigned char *>(bp);
			auto *dst = static_cast<unsigned char *>(mapped.data);
			bool need_argb_swizzle = (minfo.planecount == 4);
			uint32_t pixel_bytes = jfmt.bytes_per_pixel;
			uint32_t copy_bytes = std::min(matrix_row_bytes, w * pixel_bytes);

			bool is_bgra = (pixel_bytes == 4 && minfo.type == _jit_sym_char
 #if NOZZLE_PLATFORM_MACOS
			                // macOS IOSurface normalizes all 8-bit 4-channel to BGRA8
			                && true
#else
			                && (jfmt.nozzle_fmt == NOZZLE_FORMAT_BGRA8_UNORM ||
			                    jfmt.nozzle_fmt == NOZZLE_FORMAT_BGRA8_SRGB)
#endif
			);

			for(uint32_t y = 0; y < h; y++) {
				const unsigned char *src_row = src + y * matrix_row_bytes;
				unsigned char *dst_row = dst + static_cast<int64_t>(y) * mapped.row_stride_bytes;
				if(need_argb_swizzle) {
					for(uint32_t x = 0; x < w; x++) {
						const unsigned char *sp = src_row + x * pixel_bytes;
						unsigned char *dp = dst_row + x * pixel_bytes;
						uint32_t cs = pixel_bytes / 4;
						if(is_bgra) {
							// ARGB → BGRA: B=sp[3], G=sp[2], R=sp[1], A=sp[0]
							std::memcpy(dp + 0 * cs, sp + 3 * cs, cs); // B
							std::memcpy(dp + 1 * cs, sp + 2 * cs, cs); // G
							std::memcpy(dp + 2 * cs, sp + 1 * cs, cs); // R
							std::memcpy(dp + 3 * cs, sp + 0, cs);      // A
						} else {
							// ARGB → RGBA: R=sp[1], G=sp[2], B=sp[3], A=sp[0]
							std::memcpy(dp + 0 * cs, sp + 1 * cs, cs); // R
							std::memcpy(dp + 1 * cs, sp + 2 * cs, cs); // G
							std::memcpy(dp + 2 * cs, sp + 3 * cs, cs); // B
							std::memcpy(dp + 3 * cs, sp + 0, cs);      // A
						}
					}
				} else {
					std::memcpy(dst_row, src_row, copy_bytes);
				}
			}

			nozzle_frame_unlock_writable_pixels(frame);

			// commit frame
			err = nozzle_sender_commit_frame(sender_, frame);
			if(err != NOZZLE_OK) {
				cerr << "jit.nozzle.send: commit failed (error " << err << ")" << endl;
			} else {
				frame_count_++;
			}
		}

		// unlock matrix
		jit_object_method(matrix_obj, _jit_sym_lock, savelock);

		// output frame info
		frame_out.send({static_cast<int>(w), static_cast<int>(h), static_cast<long long>(frame_count_)});
	}
};

MIN_EXTERNAL(jit_nozzle_send);
