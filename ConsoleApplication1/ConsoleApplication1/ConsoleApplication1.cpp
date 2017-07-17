// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\..\mongoose.h"
#include <thread>
#include <mutex>
#include <iostream>
#include <boost/circular_buffer.hpp>
#include <string>
#include <condition_variable>
#include <boost/algorithm/string.hpp>
#include <vector>

#include "GregWebServer.h"
using namespace std;

GREGWEBSERVER_INIT(3)

///favicon.ico

void favicon(struct mg_connection *nc, struct http_message * hm)
{
	mg_http_serve_file(nc, hm, "c:\\temp\\favicon.ico",
		mg_mk_str("image/x-icon"), mg_mk_str(""));
	nc->flags |= MG_F_SEND_AND_CLOSE;

}

void sampleDispatchFunction(struct mg_connection *nc , struct http_message * hm)
{

	printf("sampel dispatch /api2 \n");
	char addr[32];
	if (!nc)
	{
		printf("strange...nc is null...\n");
	}

	mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
		MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);

	char value[200];
	const struct mg_str *body =
		hm->query_string.len > 0 ? &hm->query_string : &hm->body;

	std::string qs = *weirdoString((char*)(hm->query_string.p), (int)hm->query_string.len);
	GLOGW("QUERYSTRING:%s", qs.c_str());
	vector<string> tokens;
	boost::split(tokens, qs, boost::is_any_of("=&"));
	int i = 0;
	for (auto beg = tokens.begin(); beg != tokens.end(); ++beg) 
	{
		if(i%2)
			GLOGW("VALUE:%s", (*beg).c_str());
		else
			GLOGW("KEY:%s", (*beg).c_str());
		//cout << *beg << endl;
		i++;
	}



	mg_get_http_var(body, "greg", value, sizeof(value));
	std::cout << "!" << value << std::endl;

	mg_send_response_line(nc, 200,
		"Content-Type: text/html\r\n"
		"Connection: close");
	mg_printf(nc,
		"\r\n<h1>API2 CALL, %s!</h1>\r\n"
		"You asked for %.*s, arg was:%s\r\n",
		addr, (int)hm->uri.len, hm->uri.p, value);


	nc->flags |= MG_F_SEND_AND_CLOSE;
}



#include <iostream>

#if 0
#define TEST(X) printf("%d\n",X);
#else	
#define TEST(X)
#endif

int main(void)
{


	GregWebServer gw;
	gw.setDefaultPage("c:\\temp\\test.html");
	gw.addApiURIHandler("/api2",sampleDispatchFunction);
	gw.addApiURIHandler("/favicon.ico", favicon);

	gw.Start("8000", 100,true,true);


	return 0;
}


//static void ev_handler(struct mg_connection *nc, int ev, void *p) {
//	if (ev == MG_EV_HTTP_REQUEST) {
//		mg_serve_http(nc, (struct http_message *) p, s_http_server_opts);
//	}
//}

#if 0
using namespace std;
static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;
std::mutex m;
std::condition_variable cv;
boost::circular_buffer<std::string> inbound_webrequest(3);
boost::circular_buffer<std::string> outbound_webrequest(3);

std::unique_lock<std::mutex> lk(m);

void gamethread()
{


	for (;;)
	{
		//	cout << "checking..." << endl;

		//#event here: this would be replaced by an event that would be fired I think?  We need to pass info back to the caller.
		if (!inbound_webrequest.empty())
		{
			cout << "FOUND (should see before awake):" << inbound_webrequest[0] << "threadid:" << std::this_thread::get_id() << endl;
			inbound_webrequest.pop_front();
			outbound_webrequest.push_back("outbound data");
			cv.notify_one();
		}

		std::this_thread::sleep_for(1s);

	}
}


void what_thread()
{
	std::thread::id this_id = std::this_thread::get_id();
	cout << __FUNCTION__ << "***" << this_id << endl;

}

static void ev_handler(struct mg_connection *c, int ev, void *p)
{
	struct http_message *hm = (struct http_message *) p;
	//cout << __FUNCTION__<< " type:"<<ev<< endl;
	//printf("%.*s", hm->message.len, hm->message.p);

	if (ev == MG_EV_HTTP_REQUEST)
	{
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
		/* Get form variables */ //if we are doing that, here for reference
								 /*mg_get_http_var(&hm->body, "n1", n1, sizeof(n1));
								 mg_get_http_var(&hm->body, "n2", n2, sizeof(n2));*/
								 /*
								 char post_data[10 * 1024];
								 int post_data_len = mg_read(conn, post_data, sizeof(post_data));

								 char param1[1024];
								 char param2[1024];

								 mg_get_var(post_data, post_data_len, "param1", param1, sizeof(param1));
								 mg_get_var(post_data, post_data_len, "param2", param2, sizeof(param2));*/


		mg_printf(c, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
		mg_printf_http_chunk(c, "{ \"result\": %d }", 22);
		mg_send_http_chunk(c, "", 0); /* Send empty chunk, the end of response */
#else //WORKS!
		inbound_webrequest.push_back("requestmade");
		cv.wait(lk);
		string s = outbound_webrequest[0];
		outbound_webrequest.pop_front();


		char value[200];
		const struct mg_str *body =
			hm->query_string.len > 0 ? &hm->query_string : &hm->body;
		mg_get_http_var(body, "greg", value, sizeof(value));
		cout << "!" << value << endl;

		size_t len = (size_t)hm->uri.len;
		memcpy(value, hm->uri.p, len);
		value[len] = 0;

		cout << "uri:" << value << endl;
		//printf("len:%d uri:%.*s\n", hm->uri.len,hm->uri.len, hm->uri.p);


		// if (mg_vcmp(&hm->uri, "/api/v1/sum") == 0)

		//mg_send_head(c, 200, hm->message.len, "Content-Type: text/plain");
		//mg_printf(c, "%s", "test"); 
		mg_printf(c, "HTTP/1.0 200 OK\r\n\r\n%s", s.c_str());
		c->flags |= MG_F_SEND_AND_CLOSE;


#endif


		/*char addr[32];

		cs_stat_t st;
		mg_sock_addr_to_str(&c->sa, addr, sizeof(addr),
		MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);*/
		//LOG(LL_INFO,		("HTTP request from %s: %.*s %.*s", addr, (int)hm->method.len,
		//		hm->method.p, (int)hm->uri.len, hm->uri.p));

		//	mg_send(c, "hithere", strlen("hithere"));
		//	c->flags |= MG_F_SEND_AND_CLOSE;


		/*	mg_printf(c,"%s",
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n\r\n"
		"<html>\r\n"
		"<body>\r\n"
		"Hello, World!\r\n"
		"</body>\r\n"
		"</html>\r\n"
		);*/

		//	string s;
		//	s = hm->message.p;
		//		cout << s << endl;

		//	mg_printf(c, "%s", "boobs!");

		//	s = outbound_webrequest[0];
		//	outbound_webrequest.pop_front();

		//mg_printf(c, "%.*s", s.size(), s.c_str());


	}


}


void web_poll(struct mg_mgr & mgr)
{
	what_thread();
	for (;;) {
		mg_mgr_poll(&mgr, 1000);
	}
}


int main(void) {
	struct mg_mgr mgr;
	struct mg_connection *nc;

	//what_thread();

	mg_mgr_init(&mgr, NULL);
	printf("Starting web server on port %s\n", s_http_port);

	nc = mg_bind(&mgr, s_http_port, ev_handler);


	if (nc == NULL) {
		printf("Failed to create listener\n");
		return 1;
	}

	// Set up HTTP server parameters
	mg_set_protocol_http_websocket(nc);
	//s_http_server_opts.document_root = ".";  // Serve current directory
	//s_http_server_opts.enable_directory_listing = "yes";
	std::thread gthread(gamethread);

	std::this_thread::sleep_for(1s);

	std::thread t1(web_poll, mgr);
	t1.join();

	mg_mgr_free(&mgr);

	return 0;
}

#endif

