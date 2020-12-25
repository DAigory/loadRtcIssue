#pragma once
#include <iostream>
#include "api/peer_connection_interface.h"
#include "api/create_peerconnection_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"


using namespace std;
using namespace rtc;
using namespace webrtc;

class Streams;

class Stream : 
	public webrtc::PeerConnectionObserver,
	public webrtc::DataChannelObserver
{
public:
	Stream(Streams& Outer);
	~Stream();

	void SetPeerConnection(rtc::scoped_refptr<webrtc::PeerConnectionInterface> peerConnection) { PeerConnection = peerConnection; }
	void SetDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel) { DataChannel = dataChannel; }
	void SetStreamId(string id) { std::cout << "userId" << id; sessionSocketId = id; }
	string GetSessionId() { return sessionSocketId; }
	//handle ws messages
	void OnRemoteIceCandidate(std::unique_ptr<webrtc::IceCandidateInterface>);
	void HandleOffer(std::unique_ptr<webrtc::SessionDescriptionInterface>);
private:
	Streams& streams;
	string sessionSocketId;

	std::vector<std::unique_ptr<webrtc::IceCandidateInterface>> ices;
	bool answerGenerated = false;	
	bool AddIceCandidate(std::unique_ptr<webrtc::IceCandidateInterface>);
	
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> PeerConnection = nullptr;	
	rtc::scoped_refptr<webrtc::DataChannelInterface> DataChannel = nullptr;		
	//peer observer
	void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state) override;
	void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;
	void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
	void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
	void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;
	void OnRenegotiationNeeded() override;
	void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
	void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
	void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
	void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;
	void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;

	//data observer
	void OnStateChange() override;
	void OnMessage(const webrtc::DataBuffer& buffer) override;
	void OnBufferedAmountChange(uint64_t previous_amount) override;
};

