#ifndef __IPOD_BROWSER_H_
#define __IPOD_BROWSER_H_
#include <FL/Fl.H>
#include <FL/Fl_Multi_Browser.H>
#include <FL/Fl_Output.H>
#include "ipod/itunesdb.h"

#define MAX_MENU_LEVEL 5

enum menu_index_enum {
	MENU_INDEX_iPod = 0,
	MENU_INDEX_Playlist,
	MENU_INDEX_Artist,
	MENU_INDEX_Album,
	MENU_INDEX_Genre,
	MENU_INDEX_Composer,
	MENU_INDEX_Song,
	MENU_INDEX_Max,
};
	
class IPodBrowser : public Fl_Multi_Browser {
 public:
	Track **tracks;
	Playlist **playlists;
	int num_tracks,num_playlists;
	int num_of_selected();
	int next_selected();
	int first_selected();
	char *get_mount_point(void);

 private:
	char *mount_point;
	enum menu_index_enum menu_level[MAX_MENU_LEVEL];
	int menu_depth;
	char *selected_menu[MENU_INDEX_Max];
	int get_next_menuitem(char *item);
	char **get_filterd_items(enum menu_index_enum mi, Track ***songtracks);
	Track **songs_in_playlist(Playlist *pl);
	void show_status(Fl_Output *status, enum menu_index_enum cl);
	int cur_selected;
	int last_selected_line;
	int handle(int event);

 public:
	IPodBrowser(int X, int Y, int W, int H, const char *l=0);
	~IPodBrowser();
	int read_itunesdb(char *mp);
	int load(void *v);
	void cb_ipod_browser(IPodBrowser *ipb, void *v);
 private:
	void clean_up_playlists();
	void clean_up_tracks();
};

extern "C" { int it_add_track(Track *track); }
extern "C" { Playlist * it_add_playlist(Playlist *plitem); }
extern "C" { void it_add_trackid_to_playlist(Playlist *plitem, unsigned int id); }

#endif
