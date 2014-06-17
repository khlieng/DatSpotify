#include <stdio.h>
#include <conio.h>
#include <time.h>
#include <uv.h>
#include <portaudio.h>
#include <jansson.h>

#include "libservurr.h"
#include "spotify.h"
#include "typedefs.h"
#include "fifo.h"
#include "stack.h"

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
static uv_mutex_t * notify_mutex;
static uv_cond_t * sp_cond;
static int notify_events;
static PaStream * stream;
static fifo_t * audio_fifo;
static sp_playlist * playlist;
static sp_track * playing;
static int playing_duration;

static uv_mutex_t * pa_mutex;
static int track_ended = 0;
static float volume = 1.0;
static float volume_mod = 1.0;
static int paused = FALSE;
static int acc = 0;
static int elapsed = 0;
static int loading = FALSE;
static int fadeout = FALSE;
static int fadein = FALSE;
static int prefetch = FALSE;

static sp_track * upcoming_track;

static fifo_t * pq;
static stack_t * backstack;

extern uv_barrier_t barrier;

static char titleBuffer[512];

typedef struct {
	void * frames;
	int num_frames;
	char begin;
} audio_data_t;

static void SP_CALLCONV image_loaded(sp_image * image, void * userdata) {
	size_t size;
	const void * data = sp_image_data(image, &size);
	vurr_res_t * res = (vurr_res_t *)userdata;
	char * response = (char *)malloc(size);

	printf("image size: %d\n", size);

	//memcpy(response, headerJPEG, strlen(headerJPEG));
	memcpy(response, data, size);
	
	res->data = response;
	res->len = size;
	
	vurr_res_set(res, "Content-Type", "image/jpeg");
	puts("calling vurr_write");
	vurr_write(res);
	puts("releasing image");
	sp_image_release(image);
}

static void SP_CALLCONV logged_out(sp_session * session) {
    puts("Logged out");
}

static void SP_CALLCONV container_loaded(sp_playlistcontainer *pc, void *userdata)
{
    playlist = sp_playlistcontainer_playlist(pc, 1);
	puts("playlist container loaded");
	printf("%s\n", sp_playlist_name(playlist));
}

static sp_playlistcontainer_callbacks pc_callbacks = {
	0,
	0,
	0,
    container_loaded
};

static void SP_CALLCONV logged_in(sp_session * session, sp_error error) {
    if (error != SP_ERROR_OK) {
        printf("login failed: %s\n", sp_error_message(error));
        logged_out(session);
    }
    else {
		sp_playlistcontainer * playlists;

        puts("logged in");

		playlists = sp_session_playlistcontainer(session);
		sp_playlistcontainer_add_callbacks(playlists, &pc_callbacks, NULL);
		if (sp_playlistcontainer_is_loaded(playlists)) {
			playlist = sp_playlistcontainer_playlist(playlists, 1);
			
			upcoming_track = sp_playlist_track(playlist, rand() % sp_playlist_num_tracks(playlist));
		}
    }
}

static void load_track(sp_track * track) {
	int duration;

	if (playing) {
		sp_session_player_play(g_session, FALSE);
		sp_session_player_unload(g_session);
	}

	duration = sp_track_duration(track) / 1000;

	strcpy(titleBuffer, sp_artist_name(sp_track_artist(track, 0)));
	strcat(titleBuffer, "|");
	strcat(titleBuffer, sp_track_name(track));

	playing = track;
	playing_duration = duration;
	prefetch = FALSE;

	sp_session_player_load(g_session, track);
	sp_session_player_play(g_session, TRUE);

	upcoming_track = sp_playlist_track(playlist, rand() % sp_playlist_num_tracks(playlist));

	uv_mutex_lock(pa_mutex);
	loading = TRUE;
	fadeout = TRUE;
	elapsed = 0;
	acc = 0;
	paused = FALSE;
	uv_mutex_unlock(pa_mutex);
}

static int SP_CALLCONV music_delivery(sp_session * session, const sp_audioformat * format,
							   const void * frames, int num_frames) {
	audio_data_t * audio;
	size_t size;
	char begin = FALSE;
	
	if (num_frames == 0) {
		puts("delivery 0");
		return 0;
	}
	
	uv_mutex_lock(pa_mutex);
	if (audio_fifo->len > 10) {
		uv_mutex_unlock(pa_mutex);
		return 0;
	}

	if (loading) {
		loading = FALSE;
		begin = TRUE;
		puts("new track delivery");
	}

	size = num_frames * sizeof(int16_t) * format->channels;
	audio = (audio_data_t *)malloc(sizeof(*audio));
	audio->frames = malloc(size);
	audio->num_frames = num_frames;
	audio->begin = begin;
	memcpy(audio->frames, frames, size);

	fifo_push(audio_fifo, audio);
	uv_mutex_unlock(pa_mutex);

	return num_frames;
}

static int t = 0;

static void portaudio_drain(void * arg) {
	while(1) {
		t += 1;
		if (t > 2000) {
			t -= 2000;
			puts("pa_drain");
		}
		
		uv_mutex_lock(pa_mutex);
		if (!paused) {
			if (audio_fifo->len > 1) {
				audio_data_t * audio = (audio_data_t *)fifo_pop(audio_fifo);
				short * samples = (short *)audio->frames;
				int i;

				if (audio->begin) {
					printf("begin hit, vol: %f\n", volume_mod);
					fadeout = FALSE;
					fadein = TRUE;
				}

				if (fadeout && volume_mod > 0.0) {
					volume_mod -= 0.2;
					if (volume_mod < 0.0) {
						volume_mod = 0.0;
					}
				}

				if (fadein && volume_mod < 1.0) {
					volume_mod += 0.2;
				}
				else {
					if (fadein) {
						printf("fadein done, vol: %f\n", volume_mod);
					}
					fadein = FALSE;
				}

				acc += audio->num_frames;
				if (acc >= 44100) {
					acc -= 44100;
					elapsed++;

					uv_mutex_lock(sp_mutex);
					if (!prefetch && playing && playing_duration - elapsed < 10) {
						prefetch = TRUE;
						puts("prefetching...");
						sp_session_player_prefetch(g_session, upcoming_track);
					}
					uv_mutex_unlock(sp_mutex);
				}

				if (volume < 1.0 || volume_mod < 1.0) {
					for (i = 0; i < audio->num_frames * 2; i++) {
						samples[i] *= volume * volume_mod;
					}
				}
				uv_mutex_unlock(pa_mutex);

				Pa_WriteStream(stream, audio->frames, audio->num_frames);

				free(audio->frames);
				free(audio);
			}
			else {
				uv_mutex_unlock(pa_mutex);
				Sleep(1);
			}
		}
		else {
			uv_mutex_unlock(pa_mutex);
			Sleep(1);
		}
	}
}

static void SP_CALLCONV end_of_track(sp_session * session) {
	uv_mutex_lock(notify_mutex);

	notify_events = TRUE;
	track_ended = TRUE;
	
	uv_cond_broadcast(sp_cond);
	uv_mutex_unlock(notify_mutex);
}

static void SP_CALLCONV notify_main_thread(sp_session * session) {
	uv_mutex_lock(notify_mutex);

    notify_events = TRUE;
	
	uv_cond_broadcast(sp_cond);
	uv_mutex_unlock(notify_mutex);
}

static void SP_CALLCONV log_message(sp_session * session, const char * message) {
	printf("%s\n", message);
}

static void SP_CALLCONV credentials_blob_updated(sp_session * session, const char * blob) {
	FILE * file = fopen("blob", "w");
	fwrite(blob, strlen(blob), 1, file);
	fclose(file);
	printf("blob updated: %d\n", strlen(blob));
}

static void SP_CALLCONV search_complete_play(sp_search * search, void * userdata) {
	if (sp_search_error(search) == SP_ERROR_OK) {
		if (sp_search_num_tracks(search) > 0) {
			sp_track * track = sp_search_track(search, 0);

			strcpy(titleBuffer, sp_artist_name(sp_track_artist(track, 0)));
			strcat(titleBuffer, "|");
			strcat(titleBuffer, sp_track_name(track));

			sp_session_player_load(g_session, track);
			sp_session_player_play(g_session, TRUE);
			playing = track;
			playing_duration = sp_track_duration(track) / 1000;
		}
	}
	sp_search_release(search);
}

char * get_sp_title() {
	return titleBuffer;
}

sp_session * get_sp_session() {
	return g_session;
}

uv_mutex_t * get_sp_mutex() {
	return sp_mutex;
}

void load_image(vurr_res_t * res) {
	uv_mutex_lock(sp_mutex);
	if (playing) {
		sp_album * album = sp_track_album(playing);
		const byte * cover = sp_album_cover(album, SP_IMAGE_SIZE_LARGE);
		
		if (cover) {
			sp_image * image = sp_image_create(g_session, cover);
			sp_image_add_load_callback(image, &image_loaded, res);
		}
		else {
			vurr_write(res);
		}
	}
	else {
		vurr_write(res);
	}
	uv_mutex_unlock(sp_mutex);
}

void set_volume(float v) {
	uv_mutex_lock(pa_mutex);
	volume = v;
	uv_mutex_unlock(pa_mutex);
}

void pause() {
	uv_mutex_lock(pa_mutex);
	paused = !paused;
	uv_mutex_unlock(pa_mutex);
}

void next_track() {
	if (!g_session) return;

	stack_push(backstack, playing);

	uv_mutex_lock(sp_mutex);
	load_track(upcoming_track);
	uv_mutex_unlock(sp_mutex);
}

void prev_track() {
	sp_track * track = (sp_track *)stack_pop(backstack);

	if (track) {
		uv_mutex_lock(sp_mutex);
		load_track(track);
		uv_mutex_unlock(sp_mutex);
	}
}

void play_url(const char * url) {
	sp_link * link;
	sp_track * track;

	uv_mutex_lock(sp_mutex);
	link = sp_link_create_from_string(url);
	track = sp_link_as_track(link);

	if (track) {
		load_track(track);
	}

	sp_link_release(link);
	uv_mutex_unlock(sp_mutex);
}

void push_track(char * search) {
	char * copy = strdup(search);
	int len = pq->len;
	fifo_push(pq, copy);
	if (len == 0) {
		sp_search_create(g_session, search, 0, 10, 0, 10, 0, 10, 0, 10, SP_SEARCH_STANDARD, &search_complete_play, NULL);
	}
}

void play_track(char * search) {
	sp_search_create(g_session, search, 0, 10, 0, 10, 0, 10, 0, 10, SP_SEARCH_STANDARD, &search_complete_play, NULL);
}

char * ds_get_playlist(int index) {
	char url[256];
	sp_playlist * playlist;
	
	uv_mutex_lock(sp_mutex);
	playlist = sp_playlistcontainer_playlist(sp_session_playlistcontainer(g_session), index);

	if (playlist) {
		json_t * json = json_object();
		json_t * tracks = json_array();
		char * json_s;
		int i;

		for (i = 0; i < sp_playlist_num_tracks(playlist); i++) {
			sp_track * sptrack = sp_playlist_track(playlist, i);
			sp_link * link = sp_link_create_from_track(sptrack, 0);
			json_t * track = json_object();

			sp_link_as_string(link, url, sizeof(url));

			json_object_set_new(track, "artist", json_string(sp_artist_name(sp_track_artist(sptrack, 0))));
			json_object_set_new(track, "title", json_string(sp_track_name(sptrack)));			
			json_object_set_new(track, "url", json_string(url));
			json_array_append_new(tracks, track);

			sp_link_release(link);
		}
		uv_mutex_unlock(sp_mutex),

		json_object_set_new(json, "tracks", tracks);
		json_s = json_dumps(json, JSON_COMPACT);
		json_decref(json);
		return json_s;
	}
	uv_mutex_unlock(sp_mutex);
	return NULL;
}

void run_libspotify(void * arg) {
	sp_session_callbacks session_callbacks;
    sp_error error;
    sp_session * session;
    sp_session_config config;
    char username[256];
    char password[256];
	int c, i = 0;
    int next_timeout = 0;
	uv_thread_t pa_thread;

	pq = fifo_new();
	backstack = stack_new();
	audio_fifo = fifo_new();

	notify_mutex = (uv_mutex_t *)malloc(sizeof(uv_mutex_t));
	sp_mutex = (uv_mutex_t *)malloc(sizeof(uv_mutex_t));
	pa_mutex = (uv_mutex_t *)malloc(sizeof(uv_mutex_t));
	sp_cond = (uv_cond_t *)malloc(sizeof(uv_cond_t));
	uv_mutex_init(notify_mutex);
	uv_mutex_init(sp_mutex);
	uv_mutex_init(pa_mutex);
	uv_cond_init(sp_cond);

	srand(time(NULL));
	
    memset(&session_callbacks, 0, sizeof(session_callbacks));
    session_callbacks.logged_in = &logged_in;
    session_callbacks.logged_out = &logged_out;
    session_callbacks.notify_main_thread = &notify_main_thread;
	session_callbacks.music_delivery = &music_delivery;
	session_callbacks.end_of_track = &end_of_track;
	session_callbacks.log_message = &log_message;
	session_callbacks.credentials_blob_updated = &credentials_blob_updated;
	
    memset(&config, 0, sizeof(config));
    config.api_version = SPOTIFY_API_VERSION;
    config.application_key = g_appkey;
    config.application_key_size = g_appkey_size;
    config.cache_location = ".cache";
    config.settings_location = ".settings";
    config.callbacks = &session_callbacks;
    config.user_agent = "Test";
    config.tracefile = "trace.txt";
	
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

	puts("creating session...");
    error = sp_session_create(&config, &session);
	
    if (error != SP_ERROR_OK) {
        puts("could not create session");
    }
    else {
		/*FILE * file_blob = fopen("blob", "r");
		char blob[64];*/
		sp_bitrate bitrate = SP_BITRATE_320k;
		error = sp_session_preferred_bitrate(session, bitrate);
		if (error != SP_ERROR_OK) {
			puts("failed setting bitrate");
		}
		/*if (sp_session_relogin(session) == SP_ERROR_NO_CREDENTIALS) {
			puts("No relogin creds");
		}

		if (sp_session_remembered_user(session, username, 256) < 0) {*/
		/*}
		else {
			printf("Using stored username: %s\n", username);
		}

		if (!file_blob) {*/
        puts("logging in...");
		sp_session_login(session, username, password, FALSE, NULL);
		/*}
		else {
			fread(blob, 64, 1, file_blob);
			fclose(file_blob);
			puts("Using stored password");
			sp_session_login(session, username, NULL, TRUE, blob);
		}*/
    }
	g_session = session;

	uv_thread_create(&pa_thread, portaudio_drain, NULL);

	Pa_Initialize();
	Pa_OpenDefaultStream(&stream,
						 0,
						 2,
						 paInt16,
						 44100,
						 paFramesPerBufferUnspecified,
						 0,
						 0);
	Pa_StartStream(stream);

	push_track("tristam i remember");

	uv_mutex_lock(notify_mutex);
    while (1) {
        while (!notify_events) {
            uv_cond_wait(sp_cond, notify_mutex);
        }

        notify_events = FALSE;
		uv_mutex_unlock(notify_mutex);

		puts("spotify main thread says hi");

		if (track_ended) {
			track_ended = FALSE;
			next_track();
		}
		
		uv_mutex_lock(sp_mutex);
        do {
            sp_session_process_events(g_session, &next_timeout);
        } while (next_timeout == 0);
		uv_mutex_unlock(sp_mutex);

        uv_mutex_lock(notify_mutex);
    }
}

void ds_cleanup() {
	puts("pa cleanup");
	if (stream) {
		uv_mutex_lock(pa_mutex);
		Pa_StopStream(stream);
		Pa_CloseStream(stream);
		uv_mutex_unlock(pa_mutex);
	}
	puts("sp cleanup");
	if (g_session) {
		uv_mutex_lock(sp_mutex);
		if (sp_session_user(g_session)) {
			sp_session_logout(g_session);
		}
		sp_session_release(g_session);
		uv_mutex_unlock(sp_mutex);
	}
}