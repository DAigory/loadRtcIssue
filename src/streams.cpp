#include "streams.h"
#include <iostream>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <api/peer_connection_interface.h>

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/create_peerconnection_factory.h"
#include "api/peer_connection_interface.h"

#include "rtc_base/ssl_adapter.h"
#include <thread>

#include "CustomDecoder.h"

using namespace std;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;


Streams::Streams()
{
	webrtcThreadHandler = std::thread(&Streams::CreateFactory, this);
	webSocketConnection = new websocket_endpoint(*this);
}

Streams::~Streams()
{
	webrtc_thread.join();
	delete(webSocketConnection);
}

//webrtc thread implementation
void Streams::CreateFactory()
{
	webrtcThreadId = GetCurrentThreadId();
	rtc::InitializeSSL();

	network_thread = rtc::Thread::CreateWithSocketServer();
	network_thread->Start();

	worker_thread = rtc::Thread::Create();
	worker_thread->Start();
	signaling_thread = rtc::Thread::Create();
	signaling_thread->Start();

	auto VideoDecoderFactoryStrong = std::make_unique<FDummyVideoDecoderFactory>();

	PeerConnectionFactory = webrtc::CreatePeerConnectionFactory(
		network_thread.get(),
		worker_thread.get(),
		signaling_thread.get(),
		nullptr /* default_adm */,
		webrtc::CreateAudioEncoderFactory<webrtc::AudioEncoderOpus>(),
		webrtc::CreateAudioDecoderFactory<webrtc::AudioDecoderOpus>(),
		webrtc::CreateBuiltinVideoEncoderFactory(),
		std::move(VideoDecoderFactoryStrong),
		nullptr /* audio_mixer */, 
		nullptr /* audio_processing */);	

	MSG msg;
	while (::GetMessage(&msg, nullptr, 0, 0))
	{		
		//std::cout << "********************* WEBRTC THREAD WAS FINISHED*****************";
		if(msg.message == ET_WEBRTC_EVENT)
		{
			auto msgType = (std::string*)msg.lParam;
			auto msgData = (std::string*)msg.wParam;
			if (*msgType == "onOffer")
			{				
				OnOfferWs_WebrtcThread(*msgData);
			}
			else if (*msgType == "onIce")
			{
				OnIce_webrtcThread(*msgData);
			}
			delete(msgType);
			delete(msgData);			
		}
		else if (msg.message == ET_QUITLOOP)
		{
			break;
		}
		else
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
	
	std::cout << "********************* WEBRTC THREAD WAS FINISHED*****************";
}

void Streams::OnOfferWs_WebrtcThread(const string& offer)
{
	rapidjson::Document message_object;
	message_object.Parse(offer.c_str());

	std::string userId = message_object["userId"].GetString();

	webrtc::DataChannelInit data_channel_config;
	data_channel_config.ordered = false;
	data_channel_config.maxRetransmits = 0;

	std::string sdp = message_object["sdp"].GetString();
	webrtc::SdpParseError error;

	std::unique_ptr<webrtc::SessionDescriptionInterface> SessionDesc =
		webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, sdp, &error);
	webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;

	std::shared_ptr<Stream> stream = std::make_shared<Stream>(*this);

	webrtc::PeerConnectionInterface::RTCConfiguration configuration;
	configuration.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
	webrtc::PeerConnectionInterface::IceServer ice_server;
	ice_server.uri = "stun:stun.l.google.com:19302";
	configuration.servers.push_back(ice_server);

	stream->SetStreamId(userId);
	auto peer_connection =
		PeerConnectionFactory->CreatePeerConnection(configuration, webrtc::PeerConnectionDependencies{ stream.get() });

	stream->SetPeerConnection(peer_connection);
	webrtc::DataChannelInit config;
	// Configuring DataChannel.
	auto data_channel = peer_connection->CreateDataChannel("data_channel", &config);
	data_channel->RegisterObserver(stream.get());
	stream->SetDataChannel(data_channel);
	streams.insert(std::pair<string, std::shared_ptr<Stream>>(userId, stream));
	stream->HandleOffer(std::unique_ptr<webrtc::SessionDescriptionInterface>(SessionDesc.release()));
}

void Streams::OnIce_webrtcThread(const string& mes)
{
	
	rapidjson::Document message_object;
	auto msgString = mes;
	message_object.Parse(msgString.c_str());

	std::string userId = message_object["userId"].GetString();
	auto sdp = message_object["ice"].GetObjectW();

	std::string SdpMid = sdp["sdpMid"].GetString();
	int SdpMLineIndex = sdp["sdpMlineIndex"].GetInt();
	std::string candidate = sdp["candidate"].GetString();

	webrtc::SdpParseError Error;
	std::unique_ptr<webrtc::IceCandidateInterface> Candidate(webrtc::CreateIceCandidate(SdpMid, SdpMLineIndex, candidate, &Error));
	if (!Candidate)
	{
		std::cout << "OnIceWS error " << mes << endl;
	}

	if (streams.find(userId) != streams.end())
	{
		//std::cout << "ws ice recieved OnRemoteIceCandidate " <<  endl;
		streams[userId]->OnRemoteIceCandidate(std::unique_ptr<webrtc::IceCandidateInterface>(Candidate.release()));
	}
	else
	{
		std::cout << "ot ice but stream not exist " << endl;
	}
}

//streams thread
void Streams::RemoveStream(const string& sessionId)
{
	//streams.erase(sessionId); //todo shared ptr should call destructor I hope
	std::cout << "********* RemoveStream streams we have: " << streams.size() << endl;
}

//WebSocketObserver implementation
void Streams::OnMessageWS(const string& MsgType, const string& value)
{
	if (MsgType == "config") { //todo remove after serverController will created		
		rapidjson::Document documentMain;
		documentMain.SetObject();
		rapidjson::Value array(rapidjson::kArrayType);

		rapidjson::Document documentArray;
		documentArray.SetArray();

		rapidjson::Document message_object;
		message_object.SetObject();
		rapidjson::Value message_payload;
		message_payload.SetObject();
		message_payload.AddMember("name", "main", message_object.GetAllocator());
		message_payload.AddMember("selectedCameraIndex", 0, message_object.GetAllocator());
		array.PushBack(message_payload, message_object.GetAllocator());


		documentMain.AddMember("type", "registerInstanceName", message_object.GetAllocator());
		documentMain.AddMember("namesList", array, message_object.GetAllocator());

		rapidjson::StringBuffer strbuf;
		rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
		documentMain.Accept(writer);
		std::string payload = strbuf.GetString();
		webSocketConnection->send(websocket_connection_id, payload);
	}
	else if (MsgType == "registerInstanceName")
	{
		rapidjson::Document message_object;
		message_object.Parse(value.c_str());
		std::string NameServer = message_object["name"].GetString();
		std::cout << "registerInstanceName " << NameServer << endl;
	}
	else if (MsgType == "userWebSocketClosed") //todo temporary ignored in wevrtcHandler. stream delete him by himself..use it for increase deleted speed or remove it
	{
		std::cout << " userWebSocketClosed ";
	}
	else if (MsgType == "isWebCameraEnable") //todo temropary disable. Maybe it cause some freez video. Consider to change this to negotiation sdp
	{
		std::cout << "isWebCameraEnable ";
	}
	else if (MsgType == "isMicrophoneEnable")
	{
		std::cout << "isMicrophoneEnable ";
	}
	else if (MsgType == "toScreenButtonEvent")
	{
		std::cout << "toScreenButtonEvent ";
	}
	else if (MsgType == "registerSendStat")
	{
		std::cout << "registerSendStat ";
	}
	else
	{
		std::cout << "Unsupported message " << MsgType;
	}
}

void Streams::OnOfferWS(const string& offer)
{
	PostThreadMessage(webrtcThreadId, ET_WEBRTC_EVENT, (WPARAM)new std::string(offer), (WPARAM)new std::string("onOffer"));
}

void Streams::OnAnswerWS(const string& answer)
{
	std::cout << "error OnAnswerWS shouldn't call" << endl;
}

void Streams::OnIceWS(const string& mes)
{
	PostThreadMessage(webrtcThreadId, ET_WEBRTC_EVENT, (WPARAM)new std::string(mes), (WPARAM)new std::string("onIce"));
}

void Streams::OnConnectedWS()
{
	std::cout << "OnConnectedWS" << endl;
}

void Streams::OnClosed()
{
	std::cout << "on closed ws detected" << endl;
	streams.empty();
	Connect();
}

void Streams::OnConnectionError()
{
	std::cout << "OnConnectionError" << endl;
	streams.empty();
	Connect();
}

void Streams::Connect()
{
	webSocketConnection->connect("ws://localhost:80/ueVideo");
}