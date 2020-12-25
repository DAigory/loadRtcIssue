#pragma once

//#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "common.h"

using namespace std;

class WebSocketObserver {
public:
	virtual ~WebSocketObserver() {};
	virtual void OnMessageWS(const std::string& Msg, const std::string& value) { std::cout << "OnMessageWS unimplemented"; };
	virtual void OnAnswerWS(const std::string&) { std::cout << "OnAnswerWS unimplemented"; };
	virtual void OnOfferWS(const std::string&) { std::cout << "OnOfferWS unimplemented"; };
	virtual void OnIceWS(const std::string&) { std::cout << "OnIceWS unimplemented"; };
	virtual void OnConnectedWS() { std::cout << "v unimplemented"; };
	virtual void OnClosed() { std::cout << "OnClosed unimplemented"; };
	virtual void OnConnectionError() { std::cout << "OnConnectionError unimplemented"; };
	virtual void SetWebsocket_connection_id(int connectionId) { std::cout << "SetWebsocket_connection_id unimplemented"; };
};

//typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::client<websocketpp::config::asio_client> client;
//typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

class connection_metadata {
public:
	typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

	connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri, WebSocketObserver& Observer)
		: m_id(id)
		, m_hdl(hdl)
		, m_status("Connecting")
		, m_uri(uri)
		, m_server("N/A")
		, Observer(Observer)
	{}

	void on_open(client* c, websocketpp::connection_hdl hdl);
	void on_fail(client* c, websocketpp::connection_hdl hdl);
	void on_close(client* c, websocketpp::connection_hdl hdl);
	void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg);
	string getSummary();

	websocketpp::connection_hdl get_hdl() const { return m_hdl; }
	int get_id() const { return m_id; }
	std::string get_status() const { return m_status; }
	void record_sent_message(std::string message) {
		m_messages.push_back(">> " + message);
	}

	friend std::ostream& operator<< (std::ostream& out, connection_metadata const& data);
private:
	WebSocketObserver& Observer;
	int m_id;
	websocketpp::connection_hdl m_hdl;
	std::string m_status;
	std::string m_uri;
	std::string m_server;
	std::string m_error_reason;
	std::vector<std::string> m_messages;
	void OnMessageObserverHandler(client::message_ptr msg);
};


class websocket_endpoint {
public:
	websocket_endpoint(WebSocketObserver& Observer);
	~websocket_endpoint();

	int connect(std::string const& uri);
	void close(int id, websocketpp::close::status::value code);
	void send(int id, std::string message);
	void SendIceCandidate(int onnectionId, string PlayerId, const webrtc::IceCandidateInterface& IceCandidate);
	void SendAnswer(int id, std::string PlayerId, const webrtc::SessionDescriptionInterface* SDP);

	connection_metadata::ptr get_metadata(int id) const {
		con_list::const_iterator metadata_it = m_connection_list.find(id);
		if (metadata_it == m_connection_list.end()) {
			return connection_metadata::ptr();
		}
		else {
			return metadata_it->second;
		}
	}
private:
	WebSocketObserver& Observer;
	
	typedef std::map<int, connection_metadata::ptr> con_list;
	
	client m_endpoint;
	websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

	con_list m_connection_list;
	int m_next_id;
};
