#include <uv.h>
#include <stdio.h>
#include <conio.h>
#include <portaudio.h>

#include "libservurr.h"
#include "spotify.h"
#include "typedefs.h"

static const uint8_t g_appkey[] = {
    0x01, 0xE2, 0xB5, 0xF0, 0xA6, 0x0D, 0xD8, 0xE1, 0xC2, 0xFF, 0x8C, 0x0B, 0xC9, 0x59, 0x21, 0xE8,
    0x67, 0xFB, 0xE9, 0xC7, 0x8D, 0x71, 0x19, 0x24, 0x0D, 0x62, 0xC8, 0x45, 0xE6, 0x58, 0x9D, 0x90,
    0x89, 0x64, 0x84, 0x7C, 0xEC, 0x0B, 0x50, 0x7F, 0xAF, 0x6C, 0x5B, 0x38, 0xD2, 0xAB, 0x05, 0x61,
    0x31, 0x27, 0x0F, 0xA0, 0xA6, 0x21, 0x64, 0x0E, 0xFF, 0x02, 0xC7, 0x63, 0x03, 0x1B, 0x99, 0xD8,
    0x6B, 0x81, 0x3C, 0x17, 0x40, 0xC9, 0x5C, 0x0C, 0x56, 0x01, 0x0B, 0xBF, 0x42, 0xE6, 0x6B, 0x0E,
    0x14, 0x74, 0x5B, 0xC0, 0xCA, 0xF7, 0xFC, 0x12, 0x7E, 0x9D, 0x73, 0x69, 0x39, 0xBE, 0x30, 0xDD,
    0xF2, 0x11, 0xEB, 0x44, 0x60, 0x06, 0x8B, 0x9E, 0x4B, 0xAE, 0x0B, 0x10, 0x58, 0x7A, 0x85, 0x6B,
    0x26, 0x05, 0xEE, 0xB4, 0xCA, 0x35, 0x2E, 0x1D, 0xF3, 0x8A, 0x33, 0xE2, 0x56, 0xE1, 0xA8, 0x53,
    0x93, 0xBC, 0x53, 0x38, 0x77, 0xB5, 0x80, 0x5C, 0xEF, 0x66, 0xDE, 0xA1, 0x51, 0x6D, 0xC1, 0xD4,
    0x21, 0x41, 0x69, 0x4F, 0xAF, 0x06, 0x5A, 0x68, 0x8E, 0x80, 0x9F, 0x6B, 0x18, 0xEA, 0x55, 0xF5,
    0x5D, 0x4C, 0x92, 0x1C, 0x1A, 0x24, 0xB3, 0x2C, 0x4E, 0x4B, 0x75, 0xA6, 0xB4, 0x3F, 0x9B, 0x31,
    0x6F, 0xE8, 0xDC, 0xBD, 0x10, 0xC3, 0xD0, 0xE9, 0x52, 0x15, 0x57, 0x5A, 0x6F, 0xE5, 0xFD, 0x37,
    0x48, 0x64, 0xEE, 0xE8, 0xD9, 0x1D, 0x07, 0x42, 0x7A, 0xDC, 0x84, 0xC2, 0x4C, 0x5A, 0xD1, 0x1E,
    0xDD, 0xBF, 0x95, 0x28, 0x0E, 0x90, 0x2C, 0xC9, 0x5A, 0x07, 0xF5, 0x42, 0x91, 0xBE, 0xD3, 0x3D,
    0x22, 0x99, 0xCC, 0x0B, 0x88, 0x63, 0xE3, 0xC9, 0x91, 0xEB, 0x7D, 0xE1, 0x1D, 0x3E, 0x02, 0xD4,
    0x4B, 0x51, 0xDE, 0x44, 0xF1, 0x37, 0x69, 0xF4, 0xD5, 0xB6, 0x8E, 0xFF, 0xEC, 0xF3, 0xA8, 0x81,
    0xC4, 0x67, 0x86, 0xE6, 0x51, 0x51, 0xB3, 0xD5, 0x38, 0xAA, 0x98, 0x59, 0x92, 0xBC, 0xC3, 0xBE,
    0x5D, 0x9E, 0x33, 0x5E, 0x87, 0xB0, 0x7B, 0xCD, 0x52, 0xAF, 0x87, 0x6E, 0x6B, 0xCD, 0x9F, 0xC3,
    0x62, 0x5A, 0x92, 0xE7, 0x44, 0xDF, 0xC8, 0xF3, 0x1A, 0x9B, 0xCE, 0x3C, 0x98, 0x40, 0x30, 0x28,
    0xA5, 0xF0, 0x73, 0xC8, 0x8D, 0xCC, 0x37, 0x56, 0xAA, 0x2E, 0x73, 0xB1, 0x8F, 0x6E, 0x19, 0x66,
    0x34,
};
static const size_t g_appkey_size = sizeof(g_appkey);

static sp_session * g_session;
static uv_mutex_t * sp_mutex;
static uv_cond_t * sp_cond;
static int notify_events;
static PaStream * stream;

extern uv_barrier_t barrier;

#define headerJPEG "HTTP/1.1 200 OK\nContent-Type: image/jpeg\n\n"

static char titleBufferRead[256];
static char titleBuffer[256];

#ifdef WIN32
static HWND window;
#endif

static void Fix() {
	int i = 0;
	while (titleBuffer[i] != '\0') {
		if (titleBuffer[i] == -106) {
			titleBuffer[i] = '|';
		}
		i++;
	}
}

#ifdef WIN32
static BOOL CALLBACK enumWindowsProc (HWND hwnd, LPARAM lParam) 
{
	GetWindowText(hwnd, titleBufferRead, 256);
	if (strncmp(titleBufferRead, "Spotify", 7) == 0) {
		window = hwnd;
		return FALSE;
    }
	else {
		return TRUE;
	}
}

int grab_spotify_window() {
	EnumWindows(enumWindowsProc, NULL);
	return IsWindow(window) != 0;
}

int grab_track_title() {
	char prev[256];
	strcpy(prev, titleBufferRead);
	if (IsWindow(window) != 0) {
		GetWindowText(window, titleBufferRead, 256);

		if (strcmp(prev, titleBufferRead) != 0) {
			strcpy(titleBuffer, titleBufferRead);
			Fix();
			puts(titleBuffer);
		}

		return TRUE;
	}
	return FALSE;
}

HWND get_sp_window() {
	return window;
}
#endif

char * get_sp_title() {
	return titleBuffer;
}

sp_session * get_sp_session() {
	return g_session;
}

uv_mutex_t * get_sp_mutex() {
	return sp_mutex;
}

static void SP_CALLCONV image_loaded(sp_image * image, void * userdata) {
	size_t size;
	const void * data = sp_image_data(image, &size);
	vurr_res_t * res = (vurr_res_t *)userdata;
	char * response = (char *)malloc(sizeof(char) * (strlen(headerJPEG) + size));

	memcpy(response, headerJPEG, strlen(headerJPEG));
	memcpy(response + strlen(headerJPEG), data, size);

	free(res->data);
	
	res->data = response;
	res->len = strlen(headerJPEG) + size;
	
	vurr_write(res);
	sp_image_release(image);
}

void SP_CALLCONV search_complete(sp_search * search, void * userdata) {
	if (sp_search_error(search) == SP_ERROR_OK) {
		if (sp_search_num_tracks(search) > 0) {
			const byte * cover = sp_album_cover(sp_track_album(sp_search_track(search, 0)), SP_IMAGE_SIZE_LARGE);
			sp_image * image = sp_image_create(g_session, cover);
			sp_image_add_load_callback(image, &image_loaded, userdata);
		}
	}
	sp_search_release(search);
}

void SP_CALLCONV search_complete_play(sp_search * search, void * userdata) {
	if (sp_search_error(search) == SP_ERROR_OK) {
		if (sp_search_num_tracks(search) > 0) {
			sp_track * track = sp_search_track(search, 0);
			sp_session_player_load(g_session, track);
			sp_session_player_play(g_session, TRUE);
		}
	}
	sp_search_release(search);
}

static void SP_CALLCONV logged_out(sp_session * session) {
    puts("Logged out");
}

static void SP_CALLCONV logged_in(sp_session * session, sp_error error) {
    if (error != SP_ERROR_OK) {
        puts("Login failed");
        logged_out(session);
    }
    else {
        puts("Logged in");
    }
}

static int SP_CALLCONV music_delivery(sp_session * session, const sp_audioformat * format,
							   const void * frames, int num_frames) {
	if (num_frames == 0) 
		return 0;

	return num_frames;
}

static void SP_CALLCONV notify_main_thread(sp_session * session) {
	uv_mutex_lock(sp_mutex);

    notify_events = TRUE;
	
	uv_mutex_unlock(sp_mutex);
	uv_cond_broadcast(sp_cond);
}

void run_libspotify(void * arg) {
	sp_session_callbacks session_callbacks;
    sp_error session_error;
    sp_session * session;
    sp_session_config config;
    char username[128];
    char password[128];
	int c, i = 0;
    int next_timeout = 0;

	sp_mutex = (uv_mutex_t *)malloc(sizeof(uv_mutex_t));
	sp_cond = (uv_cond_t *)malloc(sizeof(uv_cond_t));
	uv_mutex_init(sp_mutex);
	uv_cond_init(sp_cond);

	uv_barrier_wait(&barrier);

    printf("\nUsername: ");
	gets(username);
	printf("Password: ");
	while ((c = getch()) != 13) {
		password[i] = c;
		putchar('*');
		i++;
	}
	password[i] = '\0';
	puts("");
	
    memset(&session_callbacks, 0, sizeof(session_callbacks));
    session_callbacks.logged_in = &logged_in;
    session_callbacks.logged_out = &logged_out;
    session_callbacks.notify_main_thread = &notify_main_thread;
	session_callbacks.music_delivery = &music_delivery;

    memset(&config, 0, sizeof(config));
    config.api_version = SPOTIFY_API_VERSION;
    config.application_key = g_appkey;
    config.application_key_size = g_appkey_size;
    config.cache_location = ".cache";
    config.settings_location = ".settings";
    config.callbacks = &session_callbacks;
    config.user_agent = "Test";
    config.tracefile = "trace.txt";

	puts("Creating session...");
    session_error = sp_session_create(&config, &session);

    if (session_error != SP_ERROR_OK) {
        puts("Could not create session");
    }
    else {
        puts("Logging in...");
        sp_session_login(session, username, password, FALSE, NULL);
    }

    g_session = session;

	/*Pa_Initialize();
	Pa_OpenDefaultStream(&stream,
						 0,
						 2,
						 paInt16,
						 44100,
						 paFramesPerBufferUnspecified,
						 0,
						 0);
	Pa_StartStream(stream);*/

	//sp_search_create(g_session, "Superbus All Alone", 0, 10, 0, 10, 0, 10, 0, 10, SP_SEARCH_STANDARD, &search_complete_play, NULL);

	uv_mutex_lock(sp_mutex);
    while (1) {
        while (!notify_events) {
            uv_cond_wait(sp_cond, sp_mutex);
        }

        notify_events = FALSE;
		uv_mutex_unlock(sp_mutex);

        do {
            sp_session_process_events(g_session, &next_timeout);
        } while (next_timeout == 0);

        uv_mutex_lock(sp_mutex);
    }
}