#pragma once

#include "webSocketConnection.h"
#include "stream.h"
#include <map>

class Streams : public WebSocketObserver
{
#define ET_WEBRTC_EVENT 7854
#define ET_QUITLOOP 6581
public:
	Streams();
	~Streams();
	void Connect();
	std::map<string, std::shared_ptr<Stream>> streams;
	//webrtc thread
	void CreateFactory();
	DWORD webrtcThreadId;
	std::thread webrtcThreadHandler;

	std::unique_ptr<rtc::Thread> network_thread;
	std::unique_ptr<rtc::Thread> worker_thread;
	std::unique_ptr<rtc::Thread> signaling_thread;

	void OnOfferWs_WebrtcThread(const string&);
	void OnIce_webrtcThread(const string&);


	void RemoveStream(const string& sessionId);
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> PeerConnectionFactory;
	std::thread webrtc_thread;
	//WebSocketObserver declaration
	int websocket_connection_id;
	websocket_endpoint* webSocketConnection;
	//thread WebSocketSignallingThread; //maybe move to separate thread
	void OnMessageWS(const string&, const string&) override;
	void OnAnswerWS(const string&) override;
	void OnOfferWS(const string&) override;
	void OnIceWS(const string&) override;
	void OnConnectedWS() override;
	void OnClosed() override;
	void OnConnectionError() override;
	void SetWebsocket_connection_id(int id) override { this->websocket_connection_id = id; };
};

