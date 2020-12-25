#pragma once
#include <iostream>

#include <api/peer_connection_interface.h>

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/create_peerconnection_factory.h"
#include "api/peer_connection_interface.h"
#include "api/stats/rtcstats_objects.h"
#include "rtc_base/strings/json.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/win32_socket_init.h"
#include "rtc_base/win32_socket_server.h"
#include "rtc_base/third_party/base64/base64.h"
#include "media/base/video_adapter.h"
#include "media/base/video_common.h"
#include "media/engine/webrtc_media_engine.h"
#include "modules/video_capture/video_capture_factory.h"
#include "third_party/libyuv/include/libyuv.h"
#include "modules/desktop_capture/win/screen_capture_utils.h"
#include "modules/desktop_capture/desktop_and_cursor_composer.h"
#include "modules/desktop_capture/cropping_window_capturer.h"
#include "modules/desktop_capture/desktop_capture_options.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "api/video/i420_buffer.h"

#include "test/vcm_capturer.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "pc/video_track_source.h"
#include "p2p/base/port_allocator.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/rtc_certificate_generator.h"
#include "rtc_base/strings/json.h"

#include <functional>

using namespace std;
using namespace rtc;
using namespace webrtc;

// Set SessionDescription events.
class MySetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
	using FSuccessCallback = std::function<void()>;
	using FFailureCallback = std::function<void(const string&)>;
public:
	static MySetSessionDescriptionObserver*
		Create(FSuccessCallback&& successCallback, FFailureCallback&& failureCallback)
	{		
		return new rtc::RefCountedObject<MySetSessionDescriptionObserver>(
			std::move(successCallback),
			std::move(failureCallback));
	}
	// Default constructor.
	MySetSessionDescriptionObserver(FSuccessCallback&& on_success, FFailureCallback&& failureCallback) :
		on_success{ std::move(on_success) },
		on_failure(std::move(failureCallback)){
		std::cout << "MySetSessionDescriptionObserver" << endl;
	}


	// Successfully set a session description.
	void OnSuccess() { 
		std::cout << "MySetSessionDescriptionObserver OnSuccess" << endl;
		on_success(); 
	}

	// Failure to set a sesion description.
	void OnFailure(const RTCError error) {
		std::cout << "MySetSessionDescriptionObserver OnFailure" << endl;
		on_failure(std::string(error.message())); 
	}

	// Unimplemented virtual function.
	void AddRef() const {}

	// Unimplemented virtual function.
	rtc::RefCountReleaseStatus Release() const { return rtc::RefCountReleaseStatus::kDroppedLastRef; }
private:
	FSuccessCallback on_success;
	FFailureCallback on_failure;
};

// Create SessionDescription events.
class MyCreateSessionDescriptionObserver : public webrtc::CreateSessionDescriptionObserver {
	using FSuccessCallback = std::function<void(webrtc::SessionDescriptionInterface*)>;
	using FFailureCallback = std::function<void(RTCError error)>;
public:
	static MyCreateSessionDescriptionObserver*
		Create(FSuccessCallback&& successCallback, FFailureCallback&& failureCallback)
	{
		return new rtc::RefCountedObject<MyCreateSessionDescriptionObserver>(
			std::move(successCallback),
			std::move(failureCallback));
	}
	// Constructor taking a callback.
	MyCreateSessionDescriptionObserver(FSuccessCallback&& on_success, FFailureCallback&& failureCallback) :
		on_success{std::move(on_success)},
		on_failure(std::move(failureCallback)){
		std::cout << "MyCreateSessionDescriptionObserver " << endl;
	}

	// Successfully created a session description.
	void OnSuccess(webrtc::SessionDescriptionInterface* desc) {
		std::cout << "MyCreateSessionDescriptionObserver OnSuccess " << endl;
		on_success(desc);
	}

	// Failure to create a session description.
	void OnFailure(const RTCError error) {
		std::cout << "MyCreateSessionDescriptionObserver OnFailure " << endl;
		on_failure(error);
	}

	// Unimplemented virtual function.
	void AddRef() const {}

	// Unimplemented virtual function.
	rtc::RefCountReleaseStatus Release() const { return rtc::RefCountReleaseStatus::kDroppedLastRef; }

private:
	FSuccessCallback on_success;
	FFailureCallback on_failure;
};


// Create SessionDescription events.
class MyCreateSessionDescriptionObserver2 : public webrtc::CreateSessionDescriptionObserver {
public:
	// Constructor taking a callback.
	MyCreateSessionDescriptionObserver2(std::function<void(webrtc::SessionDescriptionInterface*)>
		on_success) : on_success{ on_success } {}

	// Successfully created a session description.
	void OnSuccess(webrtc::SessionDescriptionInterface* desc) {
		std::cout << "OnSuccess " << endl;
		on_success(desc);
		
	}

	// Failure to create a session description.
	void OnFailure(const RTCError error) {
		std::cout << "OnFailure " << endl;
		on_failure(error);
	}

	// Unimplemented virtual function.
	void AddRef() const {}

	// Unimplemented virtual function.
	rtc::RefCountReleaseStatus Release() const { return rtc::RefCountReleaseStatus::kDroppedLastRef; }

private:
	std::function<void(webrtc::SessionDescriptionInterface*)> on_success;
	std::function<void(RTCError error)> on_failure;
};

// Set SessionDescription events.
class MySetSessionDescriptionObserver2 : public webrtc::SetSessionDescriptionObserver {
public:
	// Default constructor.
	MySetSessionDescriptionObserver2() {}

	// Successfully set a session description.
	void OnSuccess() {
		std::cout << "OnSuccess " << endl;
	}

	// Failure to set a sesion description.
	void OnFailure(const RTCError error) {
		std::cout << "OnFailure " << endl;
	}

	// Unimplemented virtual function.
	void AddRef() const {}

	// Unimplemented virtual function.
	rtc::RefCountReleaseStatus Release() const { return rtc::RefCountReleaseStatus::kDroppedLastRef; }
};


