#ifndef SPOTIFY_H
#define SPOTIFY_H

#include <libspotify\api.h>

sp_session * get_sp_session();
uv_mutex_t * get_sp_mutex();
char * get_sp_title();
void load_image(vurr_res_t * res);
void set_volume(float v);
void pause();
void next_track();
void prev_track();
void play_url(const char * url);
char * ds_get_playlist(int index);
void run_libspotify(void * arg);
void ds_cleanup();

void SP_CALLCONV search_complete(sp_search * search, void * userdata);
void SP_CALLCONV search_complete_play(sp_search * search, void * userdata);

#endif