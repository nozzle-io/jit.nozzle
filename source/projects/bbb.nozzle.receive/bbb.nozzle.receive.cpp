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

class bbb_nozzle_receive : public object<bbb_nozzle_receive> {
public:
	MIN_DESCRIPTION{"Receive textures via nozzle (GPU texture sharing)"};
	MIN_TAGS{"nozzle, texture, sharing, gpu"};
	MIN_AUTHOR{"ISHII 2bit"};

	inlet<> input{this, "(bang) poll for new frame"};
	outlet<> frame_out{this, "(list) width height frame_index on new frame"};
	outlet<> info_out{this, "(anything) sender info events"};

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
				cout << "bbb.nozzle.receive: not connected" << endl;
				return {};
			}
			NozzleConnectedSenderInfo info{};
			NozzleErrorCode err = nozzle_receiver_get_connected_info(receiver_, &info);
			if(err == NOZZLE_OK) {
				cout << "bbb.nozzle.receive connected to:" << endl;
				cout << "  name: " << (info.name ? info.name : "(null)") << endl;
				cout << "  app:  " << (info.application_name ? info.application_name : "(null)") << endl;
				cout << "  size: " << info.width << " x " << info.height << endl;
				cout << "  frames: " << info.frame_counter << endl;
			} else {
				cout << "bbb.nozzle.receive: not connected (error " << err << ")" << endl;
			}
			return {};
		}
	};

	bbb_nozzle_receive() {}
	~bbb_nozzle_receive() {
		if(receiver_) {
			nozzle_receiver_destroy(receiver_);
			receiver_ = nullptr;
		}
	}

private:
	NozzleReceiver *receiver_{nullptr};

	void setup_receiver(const std::string& name) {
		if(receiver_) {
			nozzle_receiver_destroy(receiver_);
			receiver_ = nullptr;
		}

		if(name.empty()) return;

		NozzleReceiverDesc desc{};
		desc.name = name.c_str();
		desc.application_name = "bbb.nozzle.receive";
		desc.receive_mode = NOZZLE_RECEIVE_LATEST_ONLY;

		NozzleErrorCode err = nozzle_receiver_create(&desc, &receiver_);
		if(err != NOZZLE_OK) {
			cerr << "bbb.nozzle.receive: failed to connect to '" << name << "' (error " << err << ")" << endl;
			receiver_ = nullptr;
		} else {
			cout << "bbb.nozzle.receive: connected to '" << name << "'" << endl;
		}
	}

	void poll_frame() {
		if(!receiver_) {
			setup_receiver(attr_to_string(name_attr));
		}
		if(!receiver_) return;

		NozzleAcquireDesc acq{};
		acq.timeout_ms = static_cast<uint64_t>(timeout);

		NozzleFrame *frame = nullptr;
		NozzleErrorCode err = nozzle_receiver_acquire_frame(receiver_, &acq, &frame);

		if(err == NOZZLE_OK && frame) {
			NozzleFrameInfo info{};
			nozzle_frame_get_info(frame, &info);
			nozzle_frame_release(frame);

			atoms a;
			a.push_back(static_cast<int>(info.width));
			a.push_back(static_cast<int>(info.height));
			a.push_back(static_cast<long long>(info.frame_index));
			frame_out.send(a);
		}
	}
};

MIN_EXTERNAL(bbb_nozzle_receive);
