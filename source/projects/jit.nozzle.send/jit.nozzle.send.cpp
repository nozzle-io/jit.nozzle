#include "c74_min.h"

extern "C" {
#include <nozzle/nozzle_c.h>
}

#include "jit_nozzle_format_mapping.hpp"
#include "jit_nozzle_matrix_copy.hpp"

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
	jit_nozzle::jitter_type type;
	NozzleTextureFormat nozzle_fmt;
	uint32_t source_bytes_per_pixel;
};

static bool symbol_to_jitter_type(c74::max::t_symbol *sym, jit_nozzle::jitter_type &out) {
	using namespace c74::max;
	if(sym == _jit_sym_char) { out = jit_nozzle::jitter_type::char_type; return true; }
	if(sym == _jit_sym_float32) { out = jit_nozzle::jitter_type::float32_type; return true; }
	if(sym == _jit_sym_long) { out = jit_nozzle::jitter_type::long_type; return true; }
	return false;
}

static bool jitter_to_nozzle_format(
	c74::max::t_symbol *type, int planecount, jitter_matrix_format &out
) {
	jit_nozzle::jitter_type jtype{};
	if(!symbol_to_jitter_type(type, jtype)) return false;
	jit_nozzle::send_format_mapping result{};
	if(!jit_nozzle::jitter_to_nozzle_format(jtype, planecount, result)) return false;
	out = {jtype, result.nozzle_fmt, result.source_bytes_per_pixel};
	return true;
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
    desc.fallback_flags_valid = 1;
    desc.fallback_flags = NOZZLE_FALLBACK_SAFE_DEFAULTS;

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

			NozzleResolvedTextureFormat resolved{};
			err = nozzle_frame_get_resolved_format(frame, &resolved);
			if (err != NOZZLE_OK) {
				cerr << "jit.nozzle.send: get resolved format failed (error " << err << ")" << endl;
				nozzle_frame_unlock_writable_pixels(frame);
				nozzle_frame_release(frame);
				jit_object_method(matrix_obj, _jit_sym_lock, savelock);
				return;
			}

			auto *src = static_cast<const unsigned char *>(bp);
			auto *dst = static_cast<unsigned char *>(mapped.data);
			uint32_t source_bytes_per_pixel = jfmt.source_bytes_per_pixel;
			jit_nozzle::matrix_copy_request copy_request{};
			copy_request.type = jfmt.type;
			copy_request.planecount = minfo.planecount;
			copy_request.requested_format = jfmt.nozzle_fmt;
			copy_request.mapped_format = mapped.format;
			copy_request.resolved = resolved;
			copy_request.src_bpp = source_bytes_per_pixel;

			auto dispatch = jit_nozzle::choose_matrix_copy_path(copy_request);
			if (dispatch.path == jit_nozzle::matrix_copy_path::invalid) {
				cerr << "jit.nozzle.send: copy dispatch failed: " << dispatch.error << endl;
				nozzle_frame_unlock_writable_pixels(frame);
				nozzle_frame_release(frame);
				jit_object_method(matrix_obj, _jit_sym_lock, savelock);
				return;
			}

			if (dispatch.path == jit_nozzle::matrix_copy_path::argb_swizzle) {
				bool is_bgra = (mapped.format == NOZZLE_FORMAT_BGRA8_UNORM ||
				                mapped.format == NOZZLE_FORMAT_BGRA8_SRGB);
				uint8_t permute_map[4];
				if (is_bgra) {
					permute_map[0] = 3; permute_map[1] = 2;
					permute_map[2] = 1; permute_map[3] = 0;
				} else {
					permute_map[0] = 1; permute_map[1] = 2;
					permute_map[2] = 3; permute_map[3] = 0;
				}

				NozzleTextureFormat swiz_fmt;
				if (jfmt.type == jit_nozzle::jitter_type::long_type) {
					swiz_fmt = NOZZLE_FORMAT_RGBA32_UINT;
				} else if (jfmt.type == jit_nozzle::jitter_type::float32_type) {
					swiz_fmt = NOZZLE_FORMAT_RGBA32_FLOAT;
				} else if (is_bgra) {
					swiz_fmt = NOZZLE_FORMAT_BGRA8_UNORM;
				} else {
					swiz_fmt = NOZZLE_FORMAT_RGBA8_UNORM;
				}

				NozzleErrorCode swiz_err = nozzle_swizzle_channels(
					src, dst, w, h,
					matrix_row_bytes, mapped.row_stride_bytes,
					swiz_fmt, permute_map);
				if (swiz_err != NOZZLE_OK) {
					cerr << "jit.nozzle.send: swizzle failed (error " << swiz_err << ")" << endl;
					nozzle_frame_unlock_writable_pixels(frame);
					nozzle_frame_release(frame);
					jit_object_method(matrix_obj, _jit_sym_lock, savelock);
					return;
				}
			} else if (dispatch.path == jit_nozzle::matrix_copy_path::rgb3_to_storage) {
				uint32_t src_bpp = source_bytes_per_pixel;
				uint32_t dst_bpp = resolved.bytes_per_pixel;

				auto copy_result = jit_nozzle::copy_3plane_to_storage(
					src, dst, w, h, matrix_row_bytes,
					mapped.row_stride_bytes, src_bpp, dst_bpp,
					resolved.storage_format);
				if (!copy_result.ok) {
					cerr << "jit.nozzle.send: 3-plane copy failed: " << copy_result.error << endl;
					nozzle_frame_unlock_writable_pixels(frame);
					nozzle_frame_release(frame);
					jit_object_method(matrix_obj, _jit_sym_lock, savelock);
					return;
				}

				err = nozzle_fill_opaque_alpha_channel(
					mapped.data, w, h, mapped.row_stride_bytes, mapped.format);
				if (err != NOZZLE_OK) {
					cerr << "jit.nozzle.send: alpha fill failed (error " << err << ")" << endl;
					nozzle_frame_unlock_writable_pixels(frame);
					nozzle_frame_release(frame);
					jit_object_method(matrix_obj, _jit_sym_lock, savelock);
					return;
				}
			} else if (dispatch.path == jit_nozzle::matrix_copy_path::long2_to_rgba32_uint) {
				auto copy_result = jit_nozzle::copy_2plane_long_to_rgba32_uint(
					src, dst, w, h, matrix_row_bytes,
					mapped.row_stride_bytes, source_bytes_per_pixel,
					resolved.bytes_per_pixel, resolved.storage_format);
				if (!copy_result.ok) {
					cerr << "jit.nozzle.send: 2-plane long expansion failed: "
					     << copy_result.error << endl;
					nozzle_frame_unlock_writable_pixels(frame);
					nozzle_frame_release(frame);
					jit_object_method(matrix_obj, _jit_sym_lock, savelock);
					return;
				}
			} else {
				auto copy_result = jit_nozzle::copy_direct_rows(
					src, dst, w, h, matrix_row_bytes,
					mapped.row_stride_bytes, source_bytes_per_pixel);
				if (!copy_result.ok) {
					cerr << "jit.nozzle.send: direct copy failed: " << copy_result.error << endl;
					nozzle_frame_unlock_writable_pixels(frame);
					nozzle_frame_release(frame);
					jit_object_method(matrix_obj, _jit_sym_lock, savelock);
					return;
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
