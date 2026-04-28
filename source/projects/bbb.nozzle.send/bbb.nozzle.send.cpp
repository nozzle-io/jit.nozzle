#include "c74_min.h"

extern "C" {
#include <bbb/nozzle/nozzle_c.h>
}

#include <cstring>
#include <string>

using namespace c74::min;

static std::string to_string(const symbol &s) {
	return std::string((const char *)s);
}

static std::string attr_to_string(const attribute<symbol> &a) {
	const symbol &s = a;
	return to_string(s);
}

class bbb_nozzle_send : public object<bbb_nozzle_send> {
public:
	MIN_DESCRIPTION{"Publish textures via nozzle (GPU texture sharing)"};
	MIN_TAGS{"nozzle, texture, sharing, gpu"};
	MIN_AUTHOR{"ISHII 2bit"};

	inlet<> input{this, "(bang/list) publish texture or set size"};

	attribute<symbol> name_attr{this, "name", "nozzle_sender",
		description{"Sender name (used for discovery by receivers)"},
		setter{[this](const atoms& args, int) -> atoms {
			if(args.size() > 0) {
				setup_sender(to_string(symbol(args[0])), static_cast<int>(width), static_cast<int>(height));
			}
			return args;
		}}
	};

	attribute<int> width{this, "width", 640,
		description{"Texture width"},
		setter{[this](const atoms& args, int) -> atoms {
			if(args.size() > 0) {
				setup_sender(attr_to_string(name_attr), static_cast<int>(args[0]), static_cast<int>(height));
			}
			return args;
		}}
	};

	attribute<int> height{this, "height", 480,
		description{"Texture height"},
		setter{[this](const atoms& args, int) -> atoms {
			if(args.size() > 0) {
				setup_sender(attr_to_string(name_attr), static_cast<int>(width), static_cast<int>(args[0]));
			}
			return args;
		}}
	};

	message<> bang_msg{this, "bang", "Publish a frame",
		MIN_FUNCTION {
			publish_frame();
			return {};
		}
	};

	message<> list_msg{this, "list", "Set width and height: w h",
		MIN_FUNCTION {
			if(args.size() >= 2) {
				int w = static_cast<int>(args[0]);
				int h = static_cast<int>(args[1]);
				width = w;
				height = h;
				setup_sender(attr_to_string(name_attr), w, h);
			}
			return {};
		}
	};

	message<> dump_msg{this, "dump", "Print status",
		MIN_FUNCTION {
			cout << "bbb.nozzle.send status:" << endl;
			cout << "  name: " << attr_to_string(name_attr) << endl;
			cout << "  size: " << static_cast<int>(width) << " x " << static_cast<int>(height) << endl;
			cout << "  sender: " << (sender_ ? "active" : "inactive") << endl;
			return {};
		}
	};

	bbb_nozzle_send() {}
	~bbb_nozzle_send() {
		if(sender_) {
			nozzle_sender_destroy(sender_);
			sender_ = nullptr;
		}
	}

private:
	NozzleSender *sender_{nullptr};

	void setup_sender(const std::string& name, int w, int h) {
		if(sender_) {
			nozzle_sender_destroy(sender_);
			sender_ = nullptr;
		}

		if(w <= 0 || h <= 0 || name.empty()) return;

		NozzleSenderDesc desc{};
		desc.name = name.c_str();
		desc.application_name = "bbb.nozzle.send";
		desc.ring_buffer_size = 3;

		NozzleErrorCode err = nozzle_sender_create(&desc, &sender_);
		if(err != NOZZLE_OK) {
			cerr << "bbb.nozzle.send: failed to create sender (error " << err << ")" << endl;
			sender_ = nullptr;
		}
	}

	void publish_frame() {
		if(!sender_) {
			setup_sender(attr_to_string(name_attr), static_cast<int>(width), static_cast<int>(height));
		}
		if(!sender_) return;

		NozzleFrame *frame = nullptr;
		NozzleErrorCode err = nozzle_sender_acquire_writable_frame(
			sender_,
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height),
			NOZZLE_FORMAT_RGBA8_UNORM,
			&frame
		);
		if(err != NOZZLE_OK || !frame) {
			cerr << "bbb.nozzle.send: acquire failed (error " << err << ")" << endl;
			return;
		}

		err = nozzle_sender_commit_frame(sender_, frame);
		if(err != NOZZLE_OK) {
			cerr << "bbb.nozzle.send: commit failed (error " << err << ")" << endl;
		}
	}
};

MIN_EXTERNAL(bbb_nozzle_send);
