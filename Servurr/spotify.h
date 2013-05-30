#ifndef SPOTIFY_H
#define SPOTIFY_H

#include <libspotify\api.h>

sp_session * get_sp_session();
uv_mutex_t * get_sp_mutex();
char * get_sp_title();
int grab_spotify_window();
int grab_track_title();
HWND get_sp_window();
void run_libspotify(void * arg);

void SP_CALLCONV search_complete(sp_search * search, void * userdata);
void SP_CALLCONV search_complete_play(sp_search * search, void * userdata);

#endif