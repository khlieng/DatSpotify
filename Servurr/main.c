#include <stdio.h>
#include <uv.h>
//#include <portaudio.h>

#include "libservurr.h"
#include "spotify.h"
#include "typedefs.h"
#include "util.h"

#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "libuv.lib")
#pragma comment(lib, "libspotify.lib")
//#pragma comment(lib, "portaudio_x86.lib")
#pragma comment(lib, "libservurr.lib")
#endif

#define headerJSON "HTTP/1.1 200 OK\nContent-Type: application/json; charset=utf-8\n\n"

#define PORT 80
#define MAXPENDING 128

#define PLAYPAUSE 917504
#define MUTE 524288
#define VOLUMEDOWN 589824
#define VOLUMEUP 655360
#define STOP 851968
#define PREVIOUSTRACK 786432
#define NEXTTRACK 720896

static uv_loop_t * loop;
static uv_udp_t udp_recv;
uv_barrier_t barrier;
//uv_udp_t udp_send;

static void update_title(uv_timer_t * handle, int status) {
	if (!grab_track_title()) {
		if (grab_spotify_window()) {
			puts("Spotify window found");
		}
	}
}

static uv_buf_t alloc_buffer(uv_handle_t * handle, size_t suggested_size) {
	return uv_buf_init((char *)calloc(1, suggested_size), suggested_size);
}

static void udp_send_done(uv_udp_send_t * req, int status) {
	udp_send_req_t * sr = (udp_send_req_t *)req;

	free(sr->buf.base);
	free(sr);
}

static void udp_read(uv_udp_t * req, ssize_t nread, uv_buf_t buf, struct sockaddr * addr, unsigned flags) {
	udp_send_req_t * send_req = (udp_send_req_t *)malloc(sizeof(udp_send_req_t));
	char * msg = (char *)malloc(nread + 1);
	
	if (nread == -1) {
		puts("nread == -1");
		uv_close((uv_handle_t *)req, NULL);
		free(buf.base);
		return;
	}

	strcpy(msg, buf.base);
	free(buf.base);
	
	send_req->buf.base = msg;
	send_req->buf.len = nread + 1;
	uv_udp_send((uv_udp_send_t *)send_req, req, &send_req->buf, 1, *(struct sockaddr_in *)addr, udp_send_done);
}

void get_title(vurr_res_t * res) {
	char * json = (char *)malloc(sizeof(char) * 512);
	char * title = get_sp_title();
	int title_len = strlen(title);

	strcpy(json, headerJSON);

	if (title_len > 10) {
		char * copy = strdup(title + 10);
		char * artist = strtok(copy, "|");
		char * title = strtok(NULL, "|");

		strcat(json, "{\"artist\":\"");
		strcat(json, artist);
		strcat(json, "\",\"title\":\"");
		strcat(json, title);
		strcat(json, "\"}");

		free(copy);
	}
	else if (title_len < 8) {
		strcat(json, "{\"artist\":\"Nothing is\",\"title\":\"currently playing\"}");
	}

	res->data = json;
	res->len = strlen(json);
}

void get_image(vurr_res_t * res) {
	char * title = get_sp_title();
	if (strlen(title) > 10) {
		char * copy = strdup(title + 10);
		char * artist = strtok(copy, "|");
		char * title = strtok(NULL, "|");
		char * search = (char *)malloc(sizeof(char) * 512);

		strcpy(search, artist);
		strcat(search, title);
		free(copy);

		res->data = search;
		res->defer = TRUE;

		uv_mutex_lock(get_sp_mutex());
		sp_search_create(get_sp_session(), search, 0, 10, 0, 10, 0, 10, 0, 10, SP_SEARCH_STANDARD, &search_complete, (void *)res);
		uv_mutex_unlock(get_sp_mutex());
	}
}

void get_playpause(vurr_res_t * res)  { SendMessage(get_sp_window(), WM_APPCOMMAND, NULL, PLAYPAUSE); }
void get_mute(vurr_res_t * res)		  { SendMessage(get_sp_window(), WM_APPCOMMAND, NULL, MUTE); }
void get_volumeup(vurr_res_t * res)	  { SendMessage(get_sp_window(), WM_APPCOMMAND, NULL, VOLUMEUP); }
void get_volumedown(vurr_res_t * res) { SendMessage(get_sp_window(), WM_APPCOMMAND, NULL, VOLUMEDOWN); }
void get_stop(vurr_res_t * res)		  { SendMessage(get_sp_window(), WM_APPCOMMAND, NULL, STOP); }
void get_prev(vurr_res_t * res)		  { SendMessage(get_sp_window(), WM_APPCOMMAND, NULL, PREVIOUSTRACK); }
void get_next(vurr_res_t * res)		  { SendMessage(get_sp_window(), WM_APPCOMMAND, NULL, NEXTTRACK); }

void run_libservurr(void * arg) {
	vurr.get("/title", get_title);
	vurr_get("/image", get_image);
	vurr_get("/playpause", get_playpause);
	vurr_get("/mute", get_mute);
	vurr_get("/volumeup", get_volumeup);
	vurr_get("/volumedown", get_volumedown);
	vurr_get("/stop", get_stop);
	vurr_get("/prev", get_prev);
	vurr_get("/next", get_next);
	
	vurr_static("data");
	
	uv_barrier_wait(&barrier);
	uv_barrier_destroy(&barrier);

	vurr.run(PORT);
}

int main(int argc, char * argv[]) {
	uv_thread_t spotify_thread;
	uv_thread_t vurr_thread;
	uv_timer_t timer;

	uv_barrier_init(&barrier, 2);

	if (grab_spotify_window()) {
		puts("Spotify window found");
	}
	else {
		puts("No spotify window found");
	}
	
	loop = uv_default_loop();

	uv_timer_init(loop, &timer);
	uv_timer_start(&timer, update_title, 50, 50);
	
	uv_udp_init(loop, &udp_recv);
	uv_udp_bind(&udp_recv, uv_ip4_addr("0.0.0.0", 9001), 0);
	uv_udp_recv_start(&udp_recv, alloc_buffer, udp_read);
	
	//uv_udp_init(loop, &udp_send);
	//uv_udp_bind(&udp_send, uv_ip4_addr("0.0.0.0", 0), 0);
	//uv_udp_set_broadcast(&udp_send, TRUE);

	uv_thread_create(&vurr_thread, run_libservurr, NULL);
	uv_thread_create(&spotify_thread, run_libspotify, NULL);

	return uv_run(loop, UV_RUN_DEFAULT);
}