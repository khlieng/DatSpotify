#include <uv.h>
#include <stdio.h>
#include <time.h>
#include <portaudio.h>
#include <jansson.h>

#include "libservurr.h"
#include "spotify.h"
#include "typedefs.h"
#include "fifo.h"
#include "stack.h"

#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "libuv.lib")
#pragma comment(lib, "libspotify.lib")
#pragma comment(lib, "portaudio_x86.lib")
#pragma comment(lib, "libservurr.lib")
#pragma comment(lib, "jansson.lib")
#endif

#define PORT 80

static uv_loop_t * loop;
static uv_udp_t udp_recv;
uv_barrier_t barrier;

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

static void get_title(vurr_req_t * req, vurr_res_t * res) {
	char * title = get_sp_title();
	json_t * json = json_object();
	char * response;

	if (title) {
		char * copy = strdup(title);
		char * artist = strtok(copy, "|");
		char * title = strtok(NULL, "|");

		json_object_set_new(json, "artist", json_string_nocheck(artist));
		json_object_set_new(json, "title", json_string_nocheck(title));
		response = json_dumps(json, JSON_COMPACT);

		free(copy);
	}
	json_decref(json);

	vurr_res_set(res, "Content-Type", "application/json");
	res->data = response;
	res->len = strlen(response);
}

static void work_get_image(uv_work_t * work) {
	vurr_res_t * res = (vurr_res_t *)work->data;
	puts("calling load_image");
	load_image(res);
	free(work);
}

static void get_image(vurr_req_t * req, vurr_res_t * res) {
	uv_work_t * work = (uv_work_t *)malloc(sizeof(*work));
	work->data = res;
	res->defer = TRUE;
	puts("GET /image");
	uv_queue_work(vurr_loop(), work, work_get_image, NULL);
}

static void get_playpause(vurr_req_t * req, vurr_res_t * res) { pause(); /*SendMessage(get_sp_window(), WM_APPCOMMAND, NULL, PLAYPAUSE);*/ }
static void get_mute(vurr_req_t * req, vurr_res_t * res) { /*SendMessage(get_sp_window(), WM_APPCOMMAND, NULL, MUTE);*/ }
static void get_volumeup(vurr_req_t * req, vurr_res_t * res) { /*SendMessage(get_sp_window(), WM_APPCOMMAND, NULL, VOLUMEUP);*/ }
static void get_volumedown(vurr_req_t * req, vurr_res_t * res) { /*SendMessage(get_sp_window(), WM_APPCOMMAND, NULL, VOLUMEDOWN);*/ }
static void get_stop(vurr_req_t * req, vurr_res_t * res) { /*SendMessage(get_sp_window(), WM_APPCOMMAND, NULL, STOP);*/ }
static void get_prev(vurr_req_t * req, vurr_res_t * res) { prev_track(); /*SendMessage(get_sp_window(), WM_APPCOMMAND, NULL, PREVIOUSTRACK);*/ }
static void get_next(vurr_req_t * req, vurr_res_t * res) { next_track(); /*SendMessage(get_sp_window(), WM_APPCOMMAND, NULL, NEXTTRACK);*/ }
static void get_volume(vurr_req_t * req, vurr_res_t * res) {
	float vol = atof(req->query) / 100;
	set_volume(vol);
}

static void work_get_playlist(uv_work_t * work) {
	vurr_res_t * res = (vurr_res_t *)work->data;
	int index = atoi(res->req->query);
	char * json = ds_get_playlist(index);

	if (json) {
		res->data = json;
		res->len = strlen(json);
	}

	vurr_res_set(res, "Content-Type", "application/json");
	vurr_write(res);
	free(work);
}

/*static void after_get_playlist(uv_work_t * work, int status) {
	vurr_res_t * res = (vurr_res_t *)work->data;
	puts("calling vurr_write");
	printf("after_get_playlist thread: %d\n", GetCurrentThreadId());
	vurr_write(res);
	free(work);
}*/

static void get_playlist(vurr_req_t * req, vurr_res_t * res) {
	uv_work_t * work = (uv_work_t *)malloc(sizeof(*work));
	work->data = res;
	res->defer = TRUE;
	printf("get_playlist thread: %d\n", GetCurrentThreadId());
	uv_queue_work(vurr_loop(), work, work_get_playlist, NULL);
}

static void get_playurl(vurr_req_t * req, vurr_res_t * res) {
	play_url(req->query);
}

static void on_hotkey(vurr_event_t * ev) {
	switch((int)ev->data) {
	case 1:
		pause();
		break;

	case 2:
		next_track();
		break;

	case 3:
		prev_track();
		break;
	}
}

static void run_libservurr(void * arg) {
	srand(time(NULL));

	vurr_get("/title", get_title);
	vurr_get("/image", get_image);
	vurr_get("/playpause", get_playpause);
	vurr_get("/mute", get_mute);
	vurr_get("/volumeup", get_volumeup);
	vurr_get("/volumedown", get_volumedown);
	vurr_get("/stop", get_stop);
	vurr_get("/prev", get_prev);
	vurr_get("/next", get_next);
	vurr_get("/volume", get_volume);
	vurr_get("/playlist", get_playlist);
	vurr_get("/playurl", get_playurl);
	
	vurr_on("hotkey", on_hotkey);

	printf("libservurr thread: %d\n", GetCurrentThreadId());
	
	puts("Loading static files...");
	vurr_static("data");
	
	uv_barrier_wait(&barrier);
	uv_barrier_destroy(&barrier);

	vurr_run(PORT);
}

static void run_message_loop(void * arg) {
	MSG msg;
	RegisterHotKey(NULL, 1, 0, VK_MEDIA_PLAY_PAUSE);
	RegisterHotKey(NULL, 2, 0, VK_MEDIA_NEXT_TRACK);
	RegisterHotKey(NULL, 3, 0, VK_MEDIA_PREV_TRACK);
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_HOTKEY) {
			vurr_do("hotkey", (void *)msg.wParam);
		}
	}
}

static void signal_handler(uv_signal_t * signal, int signum) {
	ds_cleanup();
	uv_signal_stop(signal);
	exit(0);
}

int main(int argc, char * argv[]) {
	uv_thread_t spotify_thread;
	uv_thread_t vurr_thread;
	uv_thread_t msg_thread;
	uv_signal_t signal_sighup;
	uv_signal_t signal_sigint;

	uv_barrier_init(&barrier, 2);
	
	loop = uv_default_loop();
	
	uv_udp_init(loop, &udp_recv);
	uv_udp_bind(&udp_recv, uv_ip4_addr("0.0.0.0", 9001), 0);
	uv_udp_recv_start(&udp_recv, alloc_buffer, udp_read);
	
	//uv_udp_init(loop, &udp_send);
	//uv_udp_bind(&udp_send, uv_ip4_addr("0.0.0.0", 0), 0);
	//uv_udp_set_broadcast(&udp_send, TRUE);

	uv_signal_init(loop, &signal_sighup);
	uv_signal_start(&signal_sighup, signal_handler, SIGHUP);
	uv_signal_init(loop, &signal_sigint);
	uv_signal_start(&signal_sigint, signal_handler, SIGINT);

	uv_thread_create(&vurr_thread, run_libservurr, NULL);
	uv_thread_create(&spotify_thread, run_libspotify, NULL);
	uv_thread_create(&msg_thread, run_message_loop, NULL);

	printf("datspotify libuv thread: %d\n", GetCurrentThreadId());

	return uv_run(loop, UV_RUN_DEFAULT);
}