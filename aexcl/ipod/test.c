#include <stdio.h>
#include "itunesdb.h"


int main(int argc, char* argv[])
{
	const char *filename;

	if(argc<2){
		printf("%s itunedb_path\n",argv[0]);
		return -1;
	}
	filename=argv[1];
	itunesdb_parse(filename);
	return 0;
}

gboolean it_add_track (Track *track)
{
	printf("album=%s, artist=%s, title=%s, genre=%s\n",
	       track->album,track->artist,track->title,track->genre);
	printf("comment=%s, composer=%s, fdesc=%s, ipod_path=%s\n",
	       track->comment,track->composer,track->fdesc,track->ipod_path);
	printf("ipod_id=%d\n", track->ipod_id);
	printf("---------------------------------\n");
	return 0;
}

static Playlist *pl[10];
static int plnum=0;
Playlist *it_add_playlist (Playlist *plitem)
{
	printf("0x%x, name=%s, type=%d, num=%d, members=%d\n",(int)plitem,
	       plitem->name, plitem->type, plitem->num, (int)plitem->members);
	pl[plnum++]=plitem;
	return plitem;
}

void it_add_trackid_to_playlist (Playlist *plitem, guint32 id)
{
	printf("add to playlist : id=%d, 0x%x, name=%s, type=%d, num=%d, members=%d\n",id, (int)plitem,
	       plitem->name, plitem->type, plitem->num, (int)plitem->members);
}


