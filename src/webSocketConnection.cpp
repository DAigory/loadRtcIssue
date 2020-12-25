#include "webSocketConnection.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

using namespace std;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

void connection_metadata::on_open(client* c, websocketpp::connection_hdl hdl) {
	m_status = "Open";
	//Observer.SetWebsocket_connection_handler(hdl); //ability send message in feature
	client::connection_ptr con = c->get_con_from_hdl(hdl);
	m_server = con->get_response_header("Server");
	Observer.OnConnectedWS();
}

void connection_metadata::on_fail(client* c, websocketpp::connection_hdl hdl) {
	m_status = "Failed";

	client::connection_ptr con = c->get_con_from_hdl(hdl);
	m_server = con->get_response_header("Server");
	m_error_reason = con->get_ec().message();
	Observer.OnConnectionError();
}

void connection_metadata::on_close(client* c, websocketpp::connection_hdl hdl) {
	m_status = "Closed";
	client::connection_ptr con = c->get_con_from_hdl(hdl);
	std::stringstream s;
	s << "close code: " << con->get_remote_close_code() << " ("
		<< websocketpp::close::status::get_string(con->get_remote_close_code())
		<< "), close reason: " << con->get_remote_close_reason();
	m_error_reason = s.str();
	Observer.OnClosed();
}

void connection_metadata::on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
	//std::cout << "****** on_ws_message" << std::endl;
	if (msg->get_opcode() == websocketpp::frame::opcode::text) {
		//m_messages.push_back(msg->get_payload());
		
		OnMessageObserverHandler(msg);
	}
	else {
		OnMessageObserverHandler(msg);
		//m_messages.push_back(websocketpp::utility::to_hex(msg->get_payload()));
	}
}

void connection_metadata::OnMessageObserverHandler(client::message_ptr msg) {

	rapidjson::Document message_object;
	auto msgString = msg->get_payload().c_str();
	message_object.Parse(msgString);

	if (message_object.HasMember("type") == false)
	{
		std::cout << "ws message don't have 'type' field " << msg << std::endl;;  ;
		return;
	}
	std::string MsgType = message_object["type"].GetString();	
	
	if (MsgType == "offer")
	{
		std::cout << "WS: OnOffer" << std::endl;
		Observer.OnOfferWS(msgString);
	}
	else if (MsgType == "answer")
	{		
		if (message_object.HasMember("sdp") == true) {
			std::cout << "WS: asnwer has sdp" << std::endl;
			Observer.OnAnswerWS(msgString);
		}
		else {
			std::cout << "WS error: asnwer don't vave sdp" << std::endl; ;
		}
	}
	else if (MsgType == "ice")
	{
		Observer.OnIceWS(msgString);
	}
	else
	{
		Observer.OnMessageWS(MsgType, msgString);
	}
}

string connection_metadata::getSummary() {
	stringstream out;

	out << "> URI: " << m_uri << "\n"
		<< "> Status: " << m_status << "\n"
		<< "> Remote Server: " << (m_server.empty() ? "None Specified" : m_server) << "\n"
		<< "> Error/close reason: " << (m_error_reason.empty() ? "N/A" : m_error_reason) << "\n";
	out << "> Messages Processed: (" << m_messages.size() << ") \n";

	std::vector<std::string>::const_iterator it;
	for (it = m_messages.begin(); it != m_messages.end(); ++it) {
		out << *it << "\n";
	}

	return out.str();
}

websocket_endpoint::websocket_endpoint(WebSocketObserver& Observer) : m_next_id(0),  Observer(Observer) {
	//m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
	//m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

	// Set logging to be pretty verbose (everything except message payloads)
	m_endpoint.set_access_channels(websocketpp::log::alevel::all);
	m_endpoint.clear_access_channels(websocketpp::log::alevel::frame_payload);
	m_endpoint.set_error_channels(websocketpp::log::elevel::all);

	m_endpoint.init_asio();
	m_endpoint.start_perpetual();
	m_endpoint.set_reuse_addr(true);

	m_thread.reset(new websocketpp::lib::thread(&client::run, &m_endpoint));

	
}

int websocket_endpoint::connect(std::string const& uri) {
	std::cout << "connect to signaling";
	websocketpp::lib::error_code ec;

	client::connection_ptr con = m_endpoint.get_connection(uri, ec);

	if (ec) {
		std::cout << "> Connect initialization error: " << ec.message() << std::endl;
		return -1;
	}

	int new_id = m_next_id++;
	Observer.SetWebsocket_connection_id(new_id);
	connection_metadata::ptr metadata_ptr(new connection_metadata(new_id, con->get_handle(), uri, Observer));
	m_connection_list[new_id] = metadata_ptr;

	con->set_open_handler(websocketpp::lib::bind(
		&connection_metadata::on_open,
		metadata_ptr,
		&m_endpoint,
		websocketpp::lib::placeholders::_1
	));
	con->set_fail_handler(websocketpp::lib::bind(
		&connection_metadata::on_fail,
		metadata_ptr,
		&m_endpoint,
		websocketpp::lib::placeholders::_1
	));
	con->set_close_handler(websocketpp::lib::bind(
		&connection_metadata::on_close,
		metadata_ptr,
		&m_endpoint,
		websocketpp::lib::placeholders::_1
	));
	con->set_message_handler(websocketpp::lib::bind(
		&connection_metadata::on_message,
		metadata_ptr,
		websocketpp::lib::placeholders::_1,
		websocketpp::lib::placeholders::_2
	));

	m_endpoint.connect(con);

	return new_id;
}

void websocket_endpoint::close(int id, websocketpp::close::status::value code) {
	websocketpp::lib::error_code ec;

	con_list::iterator metadata_it = m_connection_list.find(id);
	if (metadata_it == m_connection_list.end()) {
		std::cout << "> No connection found with id " << id << std::endl;
		return;
	}

	m_endpoint.close(metadata_it->second->get_hdl(), code, "", ec);
	if (ec) {
		std::cout << "> Error initiating close: " << ec.message() << std::endl;
	}
}

void websocket_endpoint::send(int id, std::string message) {
	websocketpp::lib::error_code ec;

	con_list::iterator metadata_it = m_connection_list.find(id);
	if (metadata_it == m_connection_list.end()) {
		std::cout << "> No connection found with id " << id << std::endl;
		return;
	}

	m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);
	if (ec) {
		std::cout << "> Error sending message: " << ec.message() << std::endl;
		return;
	}

	metadata_it->second->record_sent_message(message);
}

websocket_endpoint::~websocket_endpoint() {
	m_endpoint.stop_perpetual();

	for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
		if (it->second->get_status() != "Open") {
			// Only close open connections
			continue;
		}

		std::cout << "> Closing connection " << it->second->get_id() << std::endl;

		websocketpp::lib::error_code ec;
		m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
		if (ec) {
			std::cout << "> Error closing connection " << it->second->get_id() << ": "
				<< ec.message() << std::endl;
		}
	}

	m_thread->join();
}

void websocket_endpoint::SendIceCandidate(int onnectionId, string PlayerId, const webrtc::IceCandidateInterface& IceCandidate)
{	
	rapidjson::Value candidate_valueUserId;
	candidate_valueUserId.SetString(rapidjson::StringRef(PlayerId.c_str()));

	std::string candidate_str;
	IceCandidate.ToString(&candidate_str);
	rapidjson::Document message_object;
	message_object.SetObject();
	message_object.AddMember("type", "ice", message_object.GetAllocator());	
	message_object.AddMember("userId", candidate_valueUserId, message_object.GetAllocator());
	rapidjson::Value candidate_value;
	candidate_value.SetString(rapidjson::StringRef(candidate_str.c_str()));
	rapidjson::Value sdp_mid_value;
	sdp_mid_value.SetString(rapidjson::StringRef(IceCandidate.sdp_mid().c_str()));

	rapidjson::Value message_payload;
	message_payload.SetObject();
	message_payload.AddMember("candidate", candidate_value, message_object.GetAllocator());
	message_payload.AddMember("sdpMid", sdp_mid_value, message_object.GetAllocator());
	message_payload.AddMember("sdpMLineIndex", IceCandidate.sdp_mline_index(),
	message_object.GetAllocator());

	message_object.AddMember("ice", message_payload, message_object.GetAllocator());
	rapidjson::StringBuffer strbuf;
	rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
	message_object.Accept(writer);
	std::string payload = strbuf.GetString();

	send(onnectionId, payload);
}

void websocket_endpoint::SendAnswer(
	 int onnectionId,
	 std::string PlayerId, 
	 const webrtc::SessionDescriptionInterface* SDP)
{
	std::string offer_string;
	SDP->ToString(&offer_string);

	rapidjson::Value user_Idvalue;
	user_Idvalue.SetString(rapidjson::StringRef(PlayerId.c_str()));

	rapidjson::Value sdp_value;
	sdp_value.SetString(rapidjson::StringRef(offer_string.c_str()));

	rapidjson::Document message_object;
	message_object.SetObject();
	message_object.AddMember("type", "answer", message_object.GetAllocator());
	message_object.AddMember("sdp", sdp_value, message_object.GetAllocator());	
	message_object.AddMember("userId", user_Idvalue, message_object.GetAllocator());
	

	rapidjson::StringBuffer strbuf;
	rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
	message_object.Accept(writer);
	std::string payload = strbuf.GetString();
	send(onnectionId, payload);
}

