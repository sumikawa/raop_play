/*****************************************************************************
 * ipod_browser.cxx : browse files in ipod
 *
 * Copyright (C) 2004 Shiro Ninomiya <shiron@snino.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "ipod_browser.h"
#include "aexcl_lib.h"

#define MENU_UP "-- Up menu --"
#define MENU_ALL "-- All --"
static IPodBrowser *ipodbrowser=NULL;
static const char *top_menus[]={
	"iPod",
	"Playlists",
	"Artists",
	"Albums",
	"Genres",
	"Composers",
	"Songs",
	NULL,};


static int menu_hierarchy[]={
	MENU_INDEX_Genre,MENU_INDEX_Artist,MENU_INDEX_Album,MENU_INDEX_Song,0,
	MENU_INDEX_Composer,MENU_INDEX_Album,0,
	MENU_INDEX_Playlist,MENU_INDEX_Song,0,
	-1};

static void cb_ipod_browseri(IPodBrowser *ipb, void *v)
{
	ipb->cb_ipod_browser(ipb,v);
}

int IPodBrowser::get_next_menuitem(char *item)
{
	int i=0;
	enum menu_index_enum current_item;
	current_item=menu_level[menu_depth];
	if(current_item==MENU_INDEX_iPod){
		for(i=0;i<MENU_INDEX_Max;i++)
			if(!strcmp(top_menus[i],item)) return i;
		return -1;
	}
	while(menu_hierarchy[i]>=0){
		if(current_item==menu_hierarchy[i]) return menu_hierarchy[i+1];
		i++;
	}
	return -1;
}

IPodBrowser::IPodBrowser(int X, int Y, int W, int H, const char *l):
	Fl_Multi_Browser(X,Y,W,H,l)
{
	num_tracks=0;
	num_playlists=0;
	tracks=NULL;
	playlists=NULL;
	mount_point=NULL;
	memset(selected_menu,0,sizeof(selected_menu));
	ipodbrowser=this;
	cur_selected=0;
	last_selected_line=0;
        this->callback((Fl_Callback*)(cb_ipod_browseri));
}

IPodBrowser::~IPodBrowser()
{
	clean_up_playlists();
	clean_up_tracks();
	for(int i=0;i<MENU_INDEX_Max;i++)
		if(selected_menu[i]) free(selected_menu[i]);
	if(mount_point) free(mount_point);
}

char *IPodBrowser::get_mount_point(void)
{
	return mount_point;
}

int IPodBrowser::read_itunesdb(char *mp)
{
	clean_up_playlists();
	clean_up_tracks();
	memset(menu_level,0,sizeof(menu_level));
	menu_depth=0;
	if(!itunesdb_parse(mp)) return -1;
	if(realloc_memory((void**)&mount_point,strlen(mp)+1,__func__)) return -1;
	strcpy(mount_point,mp);
	load(NULL);
	return 0;
}

void IPodBrowser::clean_up_playlists()
{
	if(!playlists) return;
	for(int i=0;i<num_playlists;i++){
		if(playlists[i]) {
			if(playlists[i]->members) free(playlists[i]->members);
			free(playlists[i]);
		}
	}
	free(playlists);
	playlists=NULL;
	num_playlists=0;
}

void IPodBrowser::clean_up_tracks()
{
	for(int i=0;i<num_tracks;i++){
		if(tracks[i]) free(tracks[i]);
	}
	free(tracks);
	tracks=NULL;
}

int IPodBrowser::num_of_selected()
{
	// this count only songs, so return 0 for other than song level
	if(menu_level[menu_depth]!=MENU_INDEX_Song) return 0;
	int count=0;
	void *l=this->item_first();
	while(l){
		if(this->item_selected(l)) count++;
		l=this->item_next(l);
	}
	return count;
}

int IPodBrowser::first_selected()
{
	int count=0;
	void *l=this->item_first();
	while(l){
		if(this->item_selected(l)) {
			cur_selected=count;
			return count;
		}
		l=this->item_next(l);
		count++;
	}
	return -1;
}

int IPodBrowser::next_selected()
{
	int count=cur_selected+1;
	void *l=this->find_line(cur_selected+1);
	if(l) l=this->item_next(l);
	while(l){
		if(this->item_selected(l)) {
			cur_selected=count;return count;}
		l=this->item_next(l);
		count++;
	}
	return -1;
}

Track **IPodBrowser::songs_in_playlist(Playlist *pl)
{
	Track **pltracks=NULL;
	pltracks=(Track **)malloc((pl->num+1)*sizeof(Track*));
	if(!pltracks) return NULL;
	int i;
	for(i=0;i<pl->num;i++){
		pltracks[i]=(Track *)pl->members[i].data;
	}
	pltracks[i]=NULL;
	return pltracks;
}

char **IPodBrowser::get_filterd_items(enum menu_index_enum mi, Track ***songtracks)
{
	char **items=NULL;
	int num=0;
	int fi;
	for(int i=0;i<ipodbrowser->num_tracks;i++){
		int nomatch=0;
		for(fi=MENU_INDEX_Artist;fi<MENU_INDEX_Composer;fi++){
			char *name=NULL;
			if(!selected_menu[fi] || !selected_menu[fi][0]) continue;
			switch(fi){
			case MENU_INDEX_Artist:
				name=tracks[i]->artist;
				break;
			case MENU_INDEX_Album:
				name=tracks[i]->album;
				break;
			case MENU_INDEX_Genre:
				name=tracks[i]->genre;
				break;
			case MENU_INDEX_Composer:
				name=tracks[i]->composer;
				break;
			}
			if(!name){
				nomatch=1;
				break;
			}
			nomatch=strcmp(selected_menu[fi],name);
			if(nomatch) break;
		}
		if(nomatch) continue;
		char *name=NULL;
		switch(mi){
		case MENU_INDEX_Artist:
			name=tracks[i]->artist;
			break;
		case MENU_INDEX_Album:
			name=tracks[i]->album;
			break;
		case MENU_INDEX_Genre:
			name=tracks[i]->genre;
			break;
		case MENU_INDEX_Composer:
			name=tracks[i]->composer;
			break;
		case MENU_INDEX_Song:
			name=tracks[i]->title;
			break;
		default:
			break;
		}
		if(!name) continue;
		for(int j=0;j<num;j++) {
			if(!strcmp(items[j],name)) {
				name=NULL;
				break;
			}
		}
		if(!name) continue;
		if(!(num%64)){
			if(realloc_memory((void**)&items,
					  (((num/64)+1)*64+1)*sizeof(char*),__func__))
				return items;
			if(mi==MENU_INDEX_Song){
				if(realloc_memory((void**)songtracks,
						  (((num/64)+1)*64+1)*sizeof(Track*),__func__))
					return items;
			}
		}
		items[num]=name;
		if(mi==MENU_INDEX_Song) (*songtracks)[num]=tracks[i];
		num++;
	}
	if(items) items[num]=NULL;
	if(*songtracks) (*songtracks)[num]=NULL;
	return items;
}

int IPodBrowser::load(void *v)
{
	int i=0;
	clear();
	switch(menu_level[menu_depth]){
	case MENU_INDEX_iPod:
		for(i=MENU_INDEX_Playlist;i<MENU_INDEX_Max;i++){
			add(top_menus[i],NULL);
		}
		type(FL_HOLD_BROWSER);
		return i;
	case MENU_INDEX_Playlist:
		add(MENU_UP,NULL);
		while(playlists[i]){
			if(playlists[i]->type!=PL_TYPE_MPL)
				add(playlists[i]->name,playlists[i]);
			i++;
		}
		type(FL_HOLD_BROWSER);
		return i;
	default:
		char **items=NULL;
		Track **pltracks=NULL;
		if(v)
			pltracks=songs_in_playlist((Playlist *)v);
		else
			items=get_filterd_items(menu_level[menu_depth],&pltracks);
		add(MENU_UP,NULL);
		if(menu_level[menu_depth]!=MENU_INDEX_Song) {
			add(MENU_ALL,NULL);
			type(FL_HOLD_BROWSER);
		}else{
			type(FL_MULTI_BROWSER);
		}
		if(pltracks){
			while(pltracks[i]) {
				add(pltracks[i]->title,pltracks[i]->ipod_path);
				//DBGMSG("added a song, %s, %s\n",pltracks[i]->title,
				//       pltracks[i]->ipod_path);
				i++;
			}
		}else if(items){
			while(items[i]) {
				add(items[i],NULL);
				//DBGMSG("added a name, %s\n",items[i]);
				i++;
			}
		}
		if(pltracks) free(pltracks);
		if(items) free(items);
			
		return i;
	}
	return 0;
}

int IPodBrowser::handle(int event)
{
	if (event == FL_ENTER || event == FL_LEAVE) return 1;
	if (event != FL_PUSH) return Fl_Multi_Browser::handle(event);
	if(menu_level[menu_depth]!=MENU_INDEX_Song) return Fl_Multi_Browser::handle(event);
	if (Fl::event_state(FL_CTRL|FL_SHIFT)) return Fl_Multi_Browser::handle(event);
	if(!last_selected_line) return Fl_Multi_Browser::handle(event);

	int X, Y, W, H; bbox(X, Y, W, H);
	int my;
	char whichway;
	if (!Fl::event_inside(X, Y, W, H)) return 0;
	if (Fl::visible_focus()) Fl::focus(this);
	my = Fl::event_y();
	void* l = find_item(my);
	if (l) {
		whichway = !item_selected(l);
		Fl_Browser_::select(l, whichway, when() & FL_WHEN_CHANGED);
		return 1;
	}
	return Fl_Multi_Browser::handle(event);
}

void IPodBrowser::cb_ipod_browser(IPodBrowser *ipb, void *v)
{
	int i;
	char *line=(char *)text(value());
	if(last_selected_line!=value()){
		last_selected_line=value();
		return;
	}
	last_selected_line=0;
	if(!line) return;
	if(!strcmp(line,MENU_UP)){
		menu_level[menu_depth--]=MENU_INDEX_iPod;
		if(selected_menu[menu_level[menu_depth]])
			selected_menu[menu_level[menu_depth]][0]=0;
		ipb->load(NULL);
		if(v) show_status((Fl_Output *)v,menu_level[menu_depth]);
		return;
	}
	if((i=get_next_menuitem(line))<0){
		ERRMSG("%s: can't get a next menu item, nenu_depth=%d,"
		       "menu_level[menu_depth]=%d\n",__func__,
		       menu_depth,menu_level[menu_depth]);
		return;
	}
	if(!i) return; // no deeper menu, stay there
	if(strcmp(line,MENU_ALL)){
		if(realloc_memory((void**)&selected_menu[menu_level[menu_depth]],
				  strlen(line)+1,__func__))
			return;
		strcpy(selected_menu[menu_level[menu_depth]],line);
	}
	menu_level[++menu_depth]=(enum menu_index_enum)i;
	if(v) show_status((Fl_Output *)v,menu_level[menu_depth]);
	ipb->load(ipb->data(ipb->value()));
}


void IPodBrowser::show_status(Fl_Output *status, enum menu_index_enum cl)
{
	int i;
	char astr[256] = {0,};
	int firsttime=1;
	if(cl==MENU_INDEX_iPod){
		status->value(astr);
		return;
	}
	strncat(astr, "Showing ",sizeof(astr));
	strncat(astr, top_menus[cl],sizeof(astr));
	for(i=MENU_INDEX_Playlist;i<MENU_INDEX_Song;i++){
		if(selected_menu[i] && selected_menu[i][0]){
			if(firsttime)
				strncat(astr, " : ",sizeof(astr));
			else
				strncat(astr, ", ", sizeof(astr));
			strncat(astr, top_menus[i], sizeof(astr));
			strncat(astr, "=", sizeof(astr));
			strncat(astr, selected_menu[i], sizeof(astr));
			firsttime=0;
		}
	}
	status->value(astr);
	return;
}

/*
 * functions to be called from itunesdb.c
 */

static Track * find_track_from_id(unsigned int id) {
	if(!ipodbrowser) return NULL;
	for(int i=0;i<ipodbrowser->num_tracks;i++){
		if(ipodbrowser->tracks[i]->ipod_id==id) {
			return ipodbrowser->tracks[i];
		}
	}
	return NULL;
}

int it_add_track(Track *track) {
	if((ipodbrowser->num_tracks%64)==0){
		if(realloc_memory((void**)&ipodbrowser->tracks,
				  ((ipodbrowser->num_tracks/64)+1)*64*sizeof(Track *),__func__))
			return -1;
		memset(ipodbrowser->tracks+ipodbrowser->num_tracks,0,sizeof(Track *)*64);
	}
	ipodbrowser->tracks[ipodbrowser->num_tracks++]=track;
	return 0;
}

Playlist * it_add_playlist(Playlist *plitem) {
	if((ipodbrowser->num_playlists%16)==0){
		if(realloc_memory((void**)&ipodbrowser->playlists,
				  ((ipodbrowser->num_playlists/16)+1)*16*sizeof(Playlist *),
				  __func__))
			return NULL;
		memset(ipodbrowser->playlists+ipodbrowser->num_playlists,0,sizeof(Playlist *)*16);
	}
	ipodbrowser->playlists[ipodbrowser->num_playlists++]=plitem;
	return plitem;
}

void it_add_trackid_to_playlist(Playlist *plitem, unsigned int id) {
	if(!plitem) return;
	if((plitem->num%16)==0){
		if(realloc_memory((void**)&plitem->members,
				  ((plitem->num/16)+1)*16*sizeof(GList),__func__))
			return;
		memset(plitem->members+plitem->num,0,sizeof(GList)*16);
	}
	plitem->members[plitem->num++].data=(gpointer)find_track_from_id(id);
}

