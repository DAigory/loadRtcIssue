#pragma once
#include <api/peer_connection_interface.h>
#include "api/video_codecs/video_decoder.h"
#include "api/video_codecs/sdp_video_format.h"
#include "api/video/video_frame.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/i420_buffer.h"
#include "api/video/video_sink_interface.h"

#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "api/audio_codecs/audio_format.h"
#include "api/audio_codecs/audio_decoder_factory_template.h"
#include "api/audio_codecs/audio_encoder_factory_template.h"
#include "api/audio_codecs/opus/audio_decoder_opus.h"
#include "api/audio_codecs/opus/audio_encoder_opus.h"
//#include "api/test/fake_constraints.h"
#include "api/video_codecs/video_decoder.h"
#include "api/video_codecs/sdp_video_format.h"
#include "api/video/video_frame.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/i420_buffer.h"
#include "api/video/video_sink_interface.h"

#include "rtc_base/thread.h"
#include "rtc_base/logging.h"
//#include "rtc_base/flags.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/net_helpers.h"
#include "rtc_base/string_utils.h"
#include "rtc_base/signal_thread.h"

#include "pc/session_description.h"

//#include "sdk/objc/native/api/video_capturer.h"

#include "modules/video_capture/video_capture_factory.h"

#include "media/engine/internal_decoder_factory.h"
#include "media/engine/internal_encoder_factory.h"
#include "media/base/h264_profile_level_id.h"
//#include "sdk/objc/base/RTCVideoEncoderFactory.h"
#include "media/base/adapted_video_track_source.h"
#include "media/base/media_channel.h"
#include "media/base/video_common.h"

#include "modules/video_capture/video_capture_factory.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/audio_device_buffer.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/video_coding/codecs/h264/include/h264.h"

#include "common_video/h264/h264_bitstream_parser.h"
#include "common_video/h264/h264_common.h"

#include "media/base/video_broadcaster.h"

class FVideoDecoderDummy : public webrtc::VideoDecoder
{
	int32_t InitDecode(const webrtc::VideoCodec* CodecSettings, int32_t /*NumberOfCores*/) override { return WEBRTC_VIDEO_CODEC_OK; }
	int32_t Release() override { return WEBRTC_VIDEO_CODEC_OK; };

	int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* Callback) override { return WEBRTC_VIDEO_CODEC_OK; };

	int32_t Decode(const webrtc::EncodedImage& InputImage, bool MissingFrames, int64_t RenderTimeMs) override { return WEBRTC_VIDEO_CODEC_OK; };

	bool PrefersLateDecoding() const override
	{
		return true; // means that the decoder can provide a limited number of output samples
	}

	const char* ImplementationName() const override
	{
		return "FVideoDecoderDummy";
	}
};


inline webrtc::SdpVideoFormat CreateH264Format(webrtc::H264::Profile profile, webrtc::H264::Level level)
{
	const absl::optional<std::string> profile_string =
		webrtc::H264::ProfileLevelIdToString(webrtc::H264::ProfileLevelId(profile, level));
	
	return webrtc::SdpVideoFormat
	(
		cricket::kH264CodecName,
		{
			{cricket::kH264FmtpProfileLevelId, *profile_string},
			{cricket::kH264FmtpLevelAsymmetryAllowed, "1"},
			{cricket::kH264FmtpPacketizationMode, "1"}
		}
	);
}

class FDummyVideoDecoderFactory : public webrtc::VideoDecoderFactory
{
public:
	std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override
	{
		return { CreateH264Format(webrtc::H264::kProfileConstrainedBaseline, webrtc::H264::kLevel5_2) };
	}
	std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(const webrtc::SdpVideoFormat& format) override
	{
		return std::make_unique<FVideoDecoderDummy>();
	}
};
