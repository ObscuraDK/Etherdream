/// @file
///	@copyright	Copyright 2024 VERTIGO. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#include <iostream>
#include <thread>
#include <vector>
#include <cmath>
#include <cassert>
#include "c74_min.h"
#include "../shared/signal_routing_objects.h"
#include "c74_min_api.h"
#include "netherdream.h"

//struct EAD_Pnt_s dot[DOT_POINTS];

#ifndef M_PI // M_PI not defined on Windows
    #define M_PI 3.14159265358979323846
#endif

#define CIRCLE_POINTS						960
struct EAD_Pnt_s circle[CIRCLE_POINTS];


class etherdream : public signal_routing_base<etherdream>, public sample_operator<6, 1> {

public:
	MIN_DESCRIPTION {"ETHERDREAM"};
	MIN_TAGS {"audio, routing"};
	MIN_AUTHOR {"Cycling '74"};
	MIN_RELATED {"panner~, matrix~"};

	inlet<>  X {this, "(signal) Input 1"};
	inlet<>  Y{this, "(signal) Input 2"};
	inlet<>  R{ this, "(signal) Input 3"};
	inlet<>  G{ this, "(signal) Input 4" };
	inlet<>  B{ this, "(signal) Input 5" };
    inlet<>  S{ this, "(signal) Input 6" };
	outlet<> out1{this, "(anything) dac names"};
    outlet<> out2{ this, "(anything) failures" };
    attribute<int> ndac{ this, "devicenr", 1};
    attribute<int> dSample{ this, "downsample", 1 };

	void add(sample x, sample y, sample r, sample g, sample b);
	void sendFrame();
        
    message<> setup{ this, "setup",
        MIN_FUNCTION {
            if (!netherdream::LoadEtherDreamLib()) {
                cout << "cant start Etherdream" << endl;
            }
            for (uint8_t i = 0; i < netherdream::GetNumberOfDevices(); i++) {
                out1.send(netherdream::GetDeviceName(i));
            }
            return {};
        }
    };

    message<> getdevices{ this, "getdevices", "get etherdreams",
    MIN_FUNCTION {
        for (uint8_t i = 0; i < netherdream::GetNumberOfDevices(); i++) {
                out1.send(netherdream::GetDeviceName(i));
        }
        return {};
        }
    };

    message<> start{ this, "start", "starts etherdream",
        MIN_FUNCTION {
            if (!netherdream::Close()) {
              out2.send("Etherdream cant close");
            }
            if (!netherdream::OpenDevice(ndac)) {
              cout << "Etherdream cant open" << endl;
              out2.send("Etherdream cant open");
              deviceOpen = false;
            }
            else{
                deviceOpen = true;
                out2.send("Etherdream started");
            }
            return {};
        }
    };

    message<> bang{ this, "bang", "Post the greeting.",
        MIN_FUNCTION {
            //cout << netherdream::GetDeviceName(ndac) << endl;    // post to the max console
            cout << numberPoints << endl;
        return {};
        }
    };

	samples<1> operator()(sample in1, sample in2, sample in3, sample in4, sample in5, sample in6) { //bliver kørt 1 gang for hvert input
        if (S.has_signal_connection()) {
            if (downSample == dSample) {
                add(in1, in2, in3, in4, in5);
                downSample = 0;
            }
            else {
                downSample++;
            }
            if (in6 > 0.9 && rising == true) {
                sendFrame();
                rising = false;
                downSample = 0;
            }
            else if (in6 < 0.2 && rising == false) {
                rising = true;
            }
        }
		return {};
	}
    private:

        uint16_t point = 0;
        uint8_t downSample = 0;
        bool rising = true;
        uint16_t numberPoints = 0;
        bool deviceOpen = false;
};
MIN_EXTERNAL(etherdream);

void etherdream::add(sample x, sample y, sample r, sample g, sample b) {
    if (point > 960) {
        point = 0;
    }
    struct EAD_Pnt_s* pt = &circle[point];
    pt->X = x* std::numeric_limits<int16_t>::max();
    pt->Y = y* std::numeric_limits<int16_t>::max();
    pt->R = r* std::numeric_limits<int16_t>::max();
    pt->G = g* std::numeric_limits<int16_t>::max();
    pt->B = b* std::numeric_limits<int16_t>::max();
    pt->I = std::numeric_limits<int16_t>::max();
    point += 1;
}

void etherdream::sendFrame() { //netherdream::GetStatus(ndac) == netherdream::Status::Ready
    if (deviceOpen) {
        if (point > 0) {
            bool write = netherdream::WriteFrame(ndac, circle, sizeof(EAD_Pnt_s) * point, 48000, 1);
            numberPoints = point;
        }else{
            numberPoints = 0;
        }
    }
    point = 0;
}
