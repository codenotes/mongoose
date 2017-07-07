#pragma once
#include "..\..\mongoose.h"
#include <thread>
#include <mutex>
#include <iostream>
#include <boost/circular_buffer.hpp>
#include <string>
#include <condition_variable>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <map>
#include <vector>

typedef boost::function<void(struct mg_connection *nc , struct http_message *)> _requestFunction;

class GregWebServer
{
	static std::string s_http_port;
	static struct mg_serve_http_opts s_http_server_opts;
	static std::unique_lock<std::mutex> lk;
	static std::mutex m;
	static boost::circular_buffer<std::string> inbound_webrequest;
	static boost::circular_buffer<std::string> outbound_webrequest;
	static int gameThreadTimePause ;
	static std::condition_variable cv;
	static struct mg_mgr mgr;
	static struct mg_connection *mNc;
	static bool bShutdown;

	static std::vector<std::string> methods;
	static std::map<std::string, _requestFunction> apiToFunction;
	static std::map<std::string, std::string > getGetValues(std::vector<std::string>  & invec, http_message * hm)
	{
		char value[512];
		struct mg_str *body;
		//std::pair<std::string, std::string> pr;
		std::map<std::string, std::string>  temp;

		for (auto v : invec)
		{
			body=hm->query_string.len > 0 ? &hm->query_string : &hm->body;
			mg_get_http_var(body, v.c_str(), value, sizeof(value));
			temp[v]= value;
		}

		return temp;

	}



	static std::string getOneGet(std::string name , http_message * hm)
	{
		std::vector<std::string> v;
		v.push_back(name);
		auto vv = getGetValues(v, hm);
		return vv[name];

	}

	static void what_thread()
	{
		std::thread::id this_id = std::this_thread::get_id();
		std::cout << __FUNCTION__ << "***" << this_id << std::endl;

	}

	static bool dispatchMethod(std::string methodAsUri, struct mg_connection * nc, http_message * hm)
	{
		if (apiToFunction.find(methodAsUri) == apiToFunction.end()) return false;
		auto f = apiToFunction[methodAsUri];

		f(nc,hm); //call the correct function for this API/URI
		return true;

	}

	//"main" web thread intended to be separate from the game or host application, this runs on thread 1, the web server thread.
	static void web_poll(struct mg_mgr & mgr)
	{
		//what_thread();
		for (;;) 
		{
			mg_mgr_poll(&mgr, 1000);
			if (bShutdown)
				return;
		}
	}


	static void FN_MG_EV_HTTP_REQUEST(struct mg_connection *c,  void *p)
	{
		struct http_message *hm = (struct http_message *) p;
		//#define OLD_WAY
#ifdef OLD_WAY
		inbound_webrequest.push_back("requestmade");
		cv.wait(lk);
		cout << "----->AWAKE!, ev handler called...thread:" << std::this_thread::get_id() << endl;


		mg_send_head(c, 200, hm->message.len, "Content-Type: text/plain");
		//mg_send(c, "test", 4);
		//mg_printf(c, "%.*s", hm->message.len, hm->message.p);//works
		cout << "len:" << hm->message.len << endl; // " " << hm.message.
		mg_printf(c, "%s", hm->message.p); //works

#elif USE_CHUNK //works


		mg_printf(c, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
		mg_printf_http_chunk(c, "{ \"result\": %d }", 22);
		mg_send_http_chunk(c, "", 0); /* Send empty chunk, the end of response */
#else //WORKS!
		inbound_webrequest.push_back("requestmade");
		cv.wait(lk);
		std::string s = outbound_webrequest[0];
		outbound_webrequest.pop_front();


		char value[200];
		const struct mg_str *body =
			hm->query_string.len > 0 ? &hm->query_string : &hm->body;
		
		mg_get_http_var(body, "greg", value, sizeof(value));
		std::cout << "!" << value << std::endl;

		size_t len = (size_t)hm->uri.len;
		memcpy(value, hm->uri.p, len);
		value[len] = 0;

		std::cout << "uri:" << value << std::endl;
		//printf("len:%d uri:%.*s\n", hm->uri.len,hm->uri.len, hm->uri.p);


		// if (mg_vcmp(&hm->uri, "/api/v1/sum") == 0)

		//mg_send_head(c, 200, hm->message.len, "Content-Type: text/plain");
		//mg_printf(c, "%s", "test"); 
		mg_printf(c, "HTTP/1.0 200 OK\r\n\r\n%s", s.c_str());
		c->flags |= MG_F_SEND_AND_CLOSE;
#endif
	}


	static void FN_MG_EV_ACCEPT(struct mg_connection * nc)
	{
		char addr[32];
		mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
			MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
		printf("%p: Connection from %s\r\n", nc, addr);
		
	}

	static void FN_MG_EV_CLOSE(struct mg_connection * nc)
	{

		printf("%p: Connection closed\r4\n", nc);

	}

	//reference function pulled from wince sample, probably does hello correctly. I since figured all this out. 
	 static void FN_MG_EV_HTTP_REQUEST2(struct mg_connection *nc, void *p)
	 {		 struct http_message *hm = (struct http_message *) p;
		 char addr[32];

		 mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
			 MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);

	

		 printf("%p: %.*s %.*s\r\n", nc, (int)hm->method.len, hm->method.p,
			 (int)hm->uri.len, hm->uri.p);

		 char uri[255];
		 sprintf(uri, "%.*s", (int)hm->uri.len, hm->uri.p);
		 std::cout << "uri:" << uri << std::endl;

		/* if (strcmp(uri, "/api")==0)
		 {


			 char value[200];
			 const struct mg_str *body =
				 hm->query_string.len > 0 ? &hm->query_string : &hm->body;

			 mg_get_http_var(body, "greg", value, sizeof(value));
			 std::cout << "!" << value << std::endl;


			 mg_send_response_line(nc, 200,
				 "Content-Type: text/html\r\n"
				 "Connection: close");
			 mg_printf(nc,
				 "\r\n<h1>API CALL, %s!</h1>\r\n"
				 "You asked for %.*s, arg was:%s\r\n",
				 addr, (int)hm->uri.len, hm->uri.p, value);
			 
			 
			 
			 
			 nc->flags |= MG_F_SEND_AND_CLOSE;

		 }*/
		 if (!dispatchMethod(uri,nc,hm))//returns true if there was a handler, false if there wasn't
		 {
			 printf("seems to be no handler for %s, just send the main page\n", uri);
			  mg_http_serve_file(nc, hm, "c:\\temp\\test.html",
			 	 mg_mk_str("text/html"), mg_mk_str(""));
			  nc->flags |= MG_F_SEND_AND_CLOSE;
			 return;

		 }

		 //else //return just the base page
		 //{

			// mg_http_serve_file(nc, hm, "c:\\temp\\test.html",
			//	 mg_mk_str("text/html"), mg_mk_str(""));
			// nc->flags |= MG_F_SEND_AND_CLOSE;


		 //}




		
	 }

	 

	static void ev_handler(struct mg_connection *nc, int ev, void *p)
	{
		struct mbuf *io = &nc->recv_mbuf;
		struct mg_connection *c;
		switch (ev)
		{
		case MG_EV_HTTP_REQUEST:
			FN_MG_EV_HTTP_REQUEST2(nc, p);
			break;
		case MG_EV_ACCEPT: 
			FN_MG_EV_ACCEPT(nc);
			break;
		case MG_EV_CLOSE:
			FN_MG_EV_CLOSE(nc);
			break;
	/*	case MG_EV_RECV:

			printf("I got %*.s\n", io->len, io);
			mg_send(nc, "boob", 4);
			mbuf_remove(io, io->len);
			break;

		case MG_EV_SEND:
			printf("EV_SEND?\n");
			break;*/

		default:
			if(ev)
				printf("no case for:%d\n", ev);
			;
		}
	}

	


	static void gamethread()
	{

		for (;;)
		{
			//	cout << "checking..." << endl;
			if (bShutdown)
				return;

			//#event here: this would be replaced by an event that would be fired I think?  We need to pass info back to the caller.
			if (!inbound_webrequest.empty())
			{
				std::cout << "FOUND (should see before awake):" << inbound_webrequest[0] << "threadid:" << std::this_thread::get_id() << std::endl;
				inbound_webrequest.pop_front();
				outbound_webrequest.push_back("outbound data");
				cv.notify_one();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(gameThreadTimePause));
		

		}
	}


public:

	static void addApiURIHandler(std::string methodName, _requestFunction rf)
	{
		apiToFunction[methodName] = rf;
	}


	


	GregWebServer::GregWebServer()
	{

	}


	GregWebServer::~GregWebServer()
	{
	}

	static void Stop()
	{
		bShutdown = true;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		mg_mgr_free(&mgr);
	}

	static int Start(std::string port, int gameThreadPollTime, bool doThreadJoin=false)
	{
		s_http_port = port;
		gameThreadTimePause = gameThreadPollTime;

		mg_mgr_init(&mgr, NULL);
		printf("Starting web server on port %s\n", s_http_port.c_str());

		mNc = mg_bind(&mgr, s_http_port.c_str(), ev_handler);


		if (mNc == NULL) 
		{
			printf("Failed to create listener\n");
			return 1;
		}

		// Set up HTTP server parameters
		mg_set_protocol_http_websocket(mNc);
		//s_http_server_opts.document_root = ".";  // Serve current directory
		//s_http_server_opts.enable_directory_listing = "yes";

		//spin off threads
		std::thread gthread(gamethread);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		std::thread t1(web_poll, mgr);

		if (doThreadJoin)
		{
			t1.join();
			mg_mgr_free(&mgr);
		}

		//else, we are hosted by Unreal or some other environment that is running on its own thread
		//so we don't need to pause our execution to prevent program exist like we would with a console application or something

	}

};

#define GREGWEBSERVER_INIT(QSIZE) \
std::mutex GregWebServer::m; \
std::unique_lock<std::mutex> GregWebServer::lk(GregWebServer::m); \
boost::circular_buffer<std::string> GregWebServer::inbound_webrequest(QSIZE); \
boost::circular_buffer<std::string> GregWebServer::outbound_webrequest(QSIZE); \
std::string GregWebServer::s_http_port = "8000"; \
int GregWebServer::gameThreadTimePause = 1; \
std::condition_variable GregWebServer::cv; \
struct mg_mgr GregWebServer::mgr; \
struct mg_connection *GregWebServer::mNc; \
bool GregWebServer::bShutdown=false; \
std::map<std::string, _requestFunction> GregWebServer::apiToFunction;\
std::vector<std::string> GregWebServer::methods;


