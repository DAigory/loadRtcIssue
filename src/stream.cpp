#include "stream.h"
#include "streams.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include "DescriptinObserver.h"

Stream::Stream(Streams& streams_in) :
	streams(streams_in)
{
	std::cout << "********************* stream created";
}

Stream::~Stream()
{
	std::cout << "********************* stream destructor";
	if (PeerConnection != nullptr)
	{
		PeerConnection->Close();
		PeerConnection = nullptr;
	}	
	if (DataChannel != nullptr)
	{
		DataChannel->UnregisterObserver();
		DataChannel = nullptr;
	}
}


bool Stream::AddIceCandidate(std::unique_ptr<webrtc::IceCandidateInterface> candidate)
{
	if (!PeerConnection->AddIceCandidate(candidate.release()))
	{
		//std::cout << "Failed to apply remote ICE Candidate " << endl;
		return false;
	}
	else
	{
		//std::cout << "**** ice applyed " << endl;
	}
	return true;
}

//ws messages observer

void Stream::OnRemoteIceCandidate(std::unique_ptr<webrtc::IceCandidateInterface> candidate)
{
	if (answerGenerated == false)
	{		
		ices.push_back(std::move(candidate));
	}
	else
	{
		AddIceCandidate(std::unique_ptr<webrtc::IceCandidateInterface>(candidate.release()));
		for (int i = 0; i < ices.size(); i++)
		{
			AddIceCandidate(std::unique_ptr<webrtc::IceCandidateInterface>(ices[i].release()));
		}
		ices.empty();
	}
}

void Stream::HandleOffer(std::unique_ptr<webrtc::SessionDescriptionInterface> sdp)
{
	MySetSessionDescriptionObserver* SetLocalDescriptionObserver = MySetSessionDescriptionObserver::Create
	(
		[this]() // on success
	{
		std::cout << "answer created " << endl;
		streams.webSocketConnection->SendAnswer(
			streams.websocket_connection_id,
			sessionSocketId, 
			PeerConnection->local_description());
		answerGenerated = true;

		for (int i = 0; i < ices.size(); i++)
		{
			AddIceCandidate(std::unique_ptr<webrtc::IceCandidateInterface>(ices[i].release()));
		}
		ices.empty();
	},
		[this](const string& Error) // on failure
	{
		std::cout << "Failed to set local description " << Error << endl;
	}
	);

	auto OnCreateAnswerSuccess = [this, SetLocalDescriptionObserver](webrtc::SessionDescriptionInterface* SDP)
	{
		PeerConnection->SetLocalDescription(SetLocalDescriptionObserver, SDP);
	};

	MyCreateSessionDescriptionObserver* CreateAnswerObserver = MyCreateSessionDescriptionObserver::Create
	(
		OnCreateAnswerSuccess,
		[this](RTCError Error)
	{
		std::cout << "Failed to create answer " << Error.message() << endl;
	}
	);

	auto OnSetRemoteDescriptionSuccess = [this, CreateAnswerObserver, OnCreateAnswerSuccess = OnCreateAnswerSuccess]()
	{
		webrtc::PeerConnectionInterface::RTCOfferAnswerOptions AnswerOption{ 0, 0, true, true, true };
		PeerConnection->CreateAnswer(CreateAnswerObserver, AnswerOption);
	};

	MySetSessionDescriptionObserver* SetRemoteDescriptionObserver = MySetSessionDescriptionObserver::Create(
		OnSetRemoteDescriptionSuccess,
		[this](const string& Error) // on failure
	{
		std::cout << "Failed to set remote description " << Error << endl;
	}
	);

	PeerConnection->SetRemoteDescription(SetRemoteDescriptionObserver, sdp.release());
}

//peer observer
void Stream::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state)
{
	std::cout << "OnSignalingChange: " << new_state << endl;
};

void Stream::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
	std::cout << "OnAddStream" << endl;
};

void Stream::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
	std::cout << "OnRemoveStream" << endl;
};

void Stream::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel)
{
	std::cout << "OnDataChannel" << endl;
	DataChannel = data_channel;
	DataChannel->RegisterObserver(this);
};

void Stream::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state)
{
	std::cout << "PeerConnectionState changed" << endl;
	
	if (new_state == webrtc::PeerConnectionInterface::PeerConnectionState::kDisconnected //looks like ws alive (because stream deleted if ws closed but we here) and websocket on uer side was removed (it is erro or logout user out admin logout user without close ws)
		|| new_state == webrtc::PeerConnectionInterface::PeerConnectionState::kClosed
		|| new_state == webrtc::PeerConnectionInterface::PeerConnectionState::kFailed)  //it is reload page...destroy with socket, but i'm not sure
	{
		streams.RemoveStream(this->GetSessionId());
	}
}

void Stream::OnRenegotiationNeeded()
{
	std::cout << "OnRenegotiationNeeded" << endl;
};

void Stream::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state)
{
	if (new_state == webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionConnected)
	{
		std::cout << "********* ice CONNECTED: " << new_state << endl;
		std::cout << "********* streams we have: " << streams.streams.size() << endl;
	}
	else
	{
		std::cout << "********* ice state: " << new_state << endl;
	}
	
};

void Stream::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
{
	std::cout << "OnIceGatheringChange: " << new_state << endl;
};

void Stream::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
	std::cout << "ice generated: " << endl;
	streams.webSocketConnection->SendIceCandidate(streams.websocket_connection_id, sessionSocketId, *candidate);
};

void Stream::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
	std::cout << "****** track added (vide/audio)" << endl;
}

void Stream::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
	if (receiver->media_type() == cricket::MEDIA_TYPE_VIDEO)
	{

	}
}


//data observer

void Stream::OnStateChange()
{
	std::cout << "OnStateChange";
};

void Stream::OnMessage(const webrtc::DataBuffer& buffer)
{
	auto mes = std::string(buffer.data.data<char>(), buffer.data.size());
	std::cout << "byte data income";

};

void Stream::OnBufferedAmountChange(uint64_t previous_amount)
{

};
