/* Time-stamp: <2004-03-31 23:21:45 JST jcs>
|
|  Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
|  Part of the gtkpod project.
|
|  URL: http://gtkpod.sourceforge.net/
|
|  Most of the code in this file has been ported from the perl
|  script "mktunes.pl" (part of the gnupod-tools collection) written
|  by Adrian Ulrich <pab at blinkenlights.ch>.
|
|  gnupod-tools: http://www.blinkenlights.ch/cgi-bin/fm.pl?get=ipod
|
|  The code contained in this file is free software; you can redistribute
|  it and/or modify it under the terms of the GNU Lesser General Public
|  License as published by the Free Software Foundation; either version
|  2.1 of the License, or (at your option) any later version.
|
|  This file is distributed in the hope that it will be useful,
|  but WITHOUT ANY WARRANTY; without even the implied warranty of
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|  Lesser General Public License for more details.
|
|  You should have received a copy of the GNU Lesser General Public
|  License along with this code; if not, write to the Free Software
|  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
|
|  iTunes and iPod are trademarks of Apple
|
|  This product is not supported/written/published by Apple!
|
|  $Id: itunesdb.c,v 1.1.1.1 2007/08/26 20:42:57 nwinkler Exp $
*/




/* Some notes on how to use the functions in this file:


   *** Reading the iTunesDB ***

   gboolean itunesdb_parse (gchar *path); /+ path to mountpoint +/
   will read an iTunesDB and pass the data over to your program. Your
   programm is responsible to keep a representation of the data.

   The information given in the "Play Counts" file is also read if
   available and the playcounts, star rating and the time last played
   is updated.

   For each track itunesdb_parse() will pass a filled out Track structure
   to "it_add_track()", which has to be provided. The return value is
   TRUE on success and FALSE on error. At the time being, the return
   value is ignored, however.

   The minimal Track structure looks like this (feel free to have
   it_add_track() do with it as it pleases -- and yes, you are
   responsible to free the memory):

   typedef struct
   {
     gunichar2 *album_utf16;    /+ album (utf16)         +/
     gunichar2 *artist_utf16;   /+ artist (utf16)        +/
     gunichar2 *title_utf16;    /+ title (utf16)         +/
     gunichar2 *genre_utf16;    /+ genre (utf16)         +/
     gunichar2 *comment_utf16;  /+ comment (utf16)       +/
     gunichar2 *composer_utf16; /+ Composer (utf16)      +/
     gunichar2 *fdesc_utf16;    /+ Filetype descr (utf16)+/
     gunichar2 *ipod_path_utf16;/+ name of file on iPod: uses ":" instead of "/" +/
     guint32 ipod_id;           /+ unique ID of track    +/
     gint32  size;              /+ size of file in bytes +/
     gint32  tracklen;          /+ Length of track in ms +/
     gint32  cd_nr;             /+ CD number             +/
     gint32  cds;               /+ number of CDs         +/
     gint32  track_nr;          /+ track number          +/
     gint32  tracks;            /+ number of tracks      +/
     gint32  year;              /+ year                  +/
     gint32  bitrate;           /+ bitrate               +/
     guint32 time_played;       /+ time of last play  (Mac type)         +/
     guint32 time_modified;     /+ time of last modification  (Mac type) +/
     guint32 rating;            /+ star rating (stars * 20)              +/
     guint32 playcount;         /+ number of times track was played      +/
     guint32 recent_playcount;  /+ times track was played since last sync+/
     gboolean transferred;      /+ has file been transferred to iPod?    +/
   } Track;

   "transferred" will be set to TRUE because all tracks read from a
   iTunesDB are obviously (or hopefully) already transferred to the
   iPod.

   "recent_playcount" is for information only and will not be stored
   to the iPod.

   By #defining ITUNESDB_PROVIDE_UTF8, itunesdb_parse() will also
   provide utf8 versions of the above utf16 strings. You must then add
   members "gchar *album"... to the Track structure.

   For each new playlist in the iTunesDB, it_add_playlist() is
   called with a pointer to the following Playlist struct:

   typedef struct
   {
     gunichar2 *name_utf16;
     guint32 type;         /+ 1: master play list (PL_TYPE_MPL) +/
   } Playlist;

   Again, by #defining ITUNESDB_PROVIDE_UTF8, a member "gchar *name"
   will be initialized with a utf8 version of the playlist name.

   it_add_playlist() must return a pointer under which it wants the
   playlist to be referenced when it_add_track_to_playlist() is called.

   For each track in the playlist, it_add_trackid_to_playlist() is called
   with the above mentioned pointer to the playlist and the trackid to
   be added.

   gboolean it_add_track (Track *track);
   Playlist *it_add_playlist (Playlist *plitem);
   void it_add_trackid_to_playlist (Playlist *plitem, guint32 id);


   *** Writing the iTunesDB ***

   gboolean itunesdb_write (gchar *path), /+ path to mountpoint +/
   will write an updated version of the iTunesDB.

   The "Play Counts" file is renamed to "Play Counts.bak" if it exists
   to avoid it being read multiple times.

   It uses the following functions to retrieve the data necessary data
   from memory:

   guint it_get_nr_of_tracks (void);
   Track *it_get_track_by_nr (guint32 n);
   guint32 it_get_nr_of_playlists (void);
   Playlist *it_get_playlist_by_nr (guint32 n);
   guint32 it_get_nr_of_tracks_in_playlist (Playlist *plitem);
   Track *it_get_track_in_playlist_by_nr (Playlist *plitem, guint32 n);

   The master playlist is expected to be "it_get_playlist_by_nr(0)". Only
   the utf16 strings in the Playlist and Track struct are being used.

   Please note that non-transferred tracks are not automatically
   transferred to the iPod. A function

   gboolean itunesdb_copy_track_to_ipod (gchar *path, Track *track, gchar *pcfile)

   is provided to help you do that, however.

   The following functions most likely will also come in handy:

   gboolean itunesdb_cp (gchar *from_file, gchar *to_file);
   guint32 itunesdb_time_get_mac_time (void);
   time_t itunesdb_time_mac_to_host (guint32 mactime);
   guint32 itunesdb_time_host_to_mac (time_t time);
   void itunesdb_convert_filename_fs2ipod(gchar *ipod_file);
   void itunesdb_convert_filename_ipod2fs(gchar *ipod_file);

   Define "itunesdb_warning()" as you need (or simply use g_print and
   change the default g_print handler with g_set_print_handler() as is
   done in gtkpod).

   Jorg Schuler, 19.12.2002 */


/* call itunesdb_parse () to read the iTunesDB  */
/* call itunesdb_write () to write the iTunesDB */



#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

//#include <gtk/gtk.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "itunesdb.h"
//#include "support.h"
#define _(String) (String)
//#include "file.h"

#ifdef IS_GTKPOD
/* we're being linked with gtkpod */
#define itunesdb_warning(...) g_print(__VA_ARGS__)
#else
/* The following prints the error messages to the shell, converting
 * UTF8 to the current locale on the fly: */
//#define itunesdb_warning(...) do { gchar *utf8=g_strdup_printf (__VA_ARGS__); gchar *loc=g_locale_from_utf8 (utf8, -1, NULL, NULL, NULL); fprintf (stderr, "%s", loc); g_free (loc); g_free (utf8);} while (FALSE)
#define itunesdb_warning(...) fprintf(stderr, __VA_ARGS__)
#endif

#ifdef GLIB_SUBST
extern gchar *g_build_filename1 (const gchar *first_element, const gchar *file_name,  void *dummy);
#else
#define g_build_filename1 g_build_filename
#endif

/* We instruct itunesdb_parse to provide utf8 versions of the strings */
#define ITUNESDB_PROVIDE_UTF8

#define ITUNESDB_DEBUG 0
#define ITUNESDB_MHIT_DEBUG 0

#define ITUNESDB_COPYBLK 65536      /* blocksize for cp () */

/* list with the contents of the Play Count file for use when
 * importing the iTunesDB */
static GList *playcounts = NULL;

/* structure to hold the contents of one entry of the Play Count file */
struct playcount {
    guint32 playcount;
    guint32 time_played;
    guint32 rating;
};

enum {
  MHOD_ID_TITLE = 1,
  MHOD_ID_PATH = 2,
  MHOD_ID_ALBUM = 3,
  MHOD_ID_ARTIST = 4,
  MHOD_ID_GENRE = 5,
  MHOD_ID_FDESC = 6,
  MHOD_ID_COMMENT = 8,
  MHOD_ID_COMPOSER = 12,
  MHOD_ID_PLAYLIST = 100
};

static struct playcount *get_next_playcount (void);


/* Compare the two data. TRUE if identical */
static gboolean cmp_n_bytes (gchar *data1, gchar *data2, gint n)
{
  gint i;

  for(i=0; i<n; ++i)
    {
      if (data1[i] != data2[i]) return FALSE;
    }
  return TRUE;
}


/* Seeks to position "seek", then reads "n" bytes. Returns -1 on error
   during seek, or the number of bytes actually read */
static gint seek_get_n_bytes (FILE *file, gchar *data, glong seek, gint n)
{
  gint i;
  gint read;

  if (fseek (file, seek, SEEK_SET) != 0) return -1;

  for(i=0; i<n; ++i)
    {
      read = fgetc (file);
      if (read == EOF) return i;
      *data++ = (gchar)read;
    }
  return i;
}


/* Get the 4-byte-number stored at position "seek" in "file"
   (or -1 when an error occured) */
static guint32 get4int(FILE *file, glong seek)
{
  guchar data[4];
  guint32 n;

  if (seek_get_n_bytes (file, (gchar*)data, seek, 4) != 4) return -1;
  n =  ((guint32)data[3]) << 24;
  n += ((guint32)data[2]) << 16;
  n += ((guint32)data[1]) << 8;
  n += ((guint32)data[0]);
  return n;
}


/* Fix UTF16 String for BIGENDIAN machines (like PPC) */
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
static guint32 utf16_strlen (gunichar2 *utf16)
{
  guint32 i=0;
  while (utf16[i] != 0) ++i;
  return i;
}
#endif

static gunichar2 *fixup_utf16(gunichar2 *utf16_string) {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
gint32 i;
 if (utf16_string)
 {
     for(i=0; i<utf16_strlen(utf16_string); i++)
     {
	 utf16_string[i] = ((utf16_string[i]<<8) & 0xff00) |
	     ((utf16_string[i]>>8) & 0xff);
     }
 }
#endif
return utf16_string;
}


/* return the length of the header *ml, the genre number *mty,
   and a string with the entry (in UTF16?). After use you must
   free the string with g_free (). Returns NULL in case of error. */
static gunichar2 *get_mhod (FILE *file, glong seek, gint32 *ml, gint32 *mty)
{
  gchar data[4];
  gunichar2 *entry_utf16 = NULL;
  gint32 xl;

#if ITUNESDB_DEBUG
  fprintf(stderr, "get_mhod seek: %x\n", (int)seek);
#endif

  if (seek_get_n_bytes (file, data, seek, 4) != 4)
    {
      *ml = -1;
      return NULL;
    }
  if (cmp_n_bytes (data, "mhod", 4) == FALSE )
    {
      *ml = -1;
      return NULL;
    }
  *ml = get4int (file, seek+8);       /* length         */
  *mty = get4int (file, seek+12);     /* mhod_id number */
  xl = get4int (file, seek+28);       /* entry length   */

#if ITUNESDB_DEBUG
  fprintf(stderr, "ml: %x mty: %x, xl: %x\n", *ml, *mty, xl);
#endif

  switch (*mty)
  {
  case MHOD_ID_PLAYLIST: /* do something with the "weird mhod" */
      break;
  default:
      entry_utf16 = g_malloc (xl+2);
      if (seek_get_n_bytes (file, (gchar *)entry_utf16, seek+40, xl) != xl) {
	  g_free (entry_utf16);
	  entry_utf16 = NULL;
	  *ml = -1;
      }
      else
      {
	  entry_utf16[xl/2] = 0; /* add trailing 0 */
      }
      break;
  }
  return fixup_utf16(entry_utf16);
}



/* get a PL, return pos where next PL should be, name and content */
static glong get_pl(FILE *file, glong seek)
{
  gunichar2 *plname_utf16 = NULL, *plname_utf16_maybe;
#ifdef ITUNESDB_PROVIDE_UTF8
  gchar *plname_utf8;
#endif
  guint32 type, pltype, tracknum, n;
  guint32 nextseek;
  gint32 zip;
  Playlist *plitem;
  guint32 ref;

  gchar data[4];


#if ITUNESDB_DEBUG
  fprintf(stderr, "mhyp seek: %x\n", (int)seek);
#endif

  if (seek_get_n_bytes (file, data, seek, 4) != 4) return -1;
  if (cmp_n_bytes (data, "mhyp", 4) == FALSE)      return -1; /* not pl */
  /* Some Playlists have added 256 to their type -- I don't know what
     it's for, so we just ignore it for now -> & 0xff */
  pltype = get4int (file, seek+20) & 0xff;  /* Type of playlist (1= MPL) */
  tracknum = get4int (file, seek+16); /* number of tracks in playlist */
  nextseek = seek + get4int (file, seek+8); /* possible begin of next PL */
  zip = get4int (file, seek+4); /* length of header */
  if (zip == 0) return -1;      /* error! */
  do
  {
      seek += zip;
      if (seek_get_n_bytes (file, data, seek, 4) != 4) return -1;
      plname_utf16_maybe = get_mhod(file, seek, &zip, (gint32*)&type); /* PL name */
      if (zip != -1) switch (type)
      {
      case MHOD_ID_PLAYLIST:
	  break; /* here we could do something about the "weird mhod" */
      case MHOD_ID_TITLE:
	  if (plname_utf16_maybe)
	  {
	      /* sometimes there seem to be two mhod TITLE headers */
	      if (plname_utf16) g_free (plname_utf16);
	      plname_utf16 = plname_utf16_maybe;
	  }
	  break;
      }
  } while (zip != -1); /* read all MHODs */
  if (!plname_utf16)
  {   /* we did not read a valid mhod TITLE header -> */
      /* we simply make up our own name */
	if (pltype == 1)
	    plname_utf16 = g_utf8_to_utf16 (_("Master-PL"),
					    -1, NULL, NULL, NULL);
	else plname_utf16 = g_utf8_to_utf16 (_("Playlist"),
					     -1, NULL, NULL, NULL);
  }
#ifdef ITUNESDB_PROVIDE_UTF8
  plname_utf8 = g_utf16_to_utf8 (plname_utf16, -1, NULL, NULL, NULL);
#endif


#if ITUNESDB_DEBUG
  fprintf(stderr, "pln: %s(%d Tracks) \n", plname_utf8, (int)tracknum);
#endif

  plitem = g_malloc0 (sizeof (Playlist));

#ifdef ITUNESDB_PROVIDE_UTF8
  plitem->name = plname_utf8;
#endif
  plitem->name_utf16 = plname_utf16;
  plitem->type = pltype;

  /* create new playlist */
  plitem = it_add_playlist(plitem);

#if ITUNESDB_DEBUG
  fprintf(stderr, "added pl: %s", plname_utf8);
#endif

  n = 0;  /* number of tracks read */
  while (n < tracknum)
    {
      /* We read the mhip headers and skip everything else. If we
	 find a mhyp header before n==tracknum, something is wrong */
      if (seek_get_n_bytes (file, data, seek, 4) != 4) return -1;
      if (cmp_n_bytes (data, "mhyp", 4) == TRUE) return -1; /* Wrong!!! */
      if (cmp_n_bytes (data, "mhip", 4) == TRUE)
	{
	  ref = get4int(file, seek+24);
	  it_add_trackid_to_playlist(plitem, ref);
	  ++n;
	}
      seek += get4int (file, seek+8);
    }
  return nextseek;
}


static glong get_mhit(FILE *file, glong seek)
{
  Track *track;
  gchar data[4];
#ifdef ITUNESDB_PROVIDE_UTF8
  gchar *entry_utf8;
#endif
  gunichar2 *entry_utf16;
  gint type;
  gint zip = 0;
  struct playcount *playcount;

#if ITUNESDB_DEBUG
  fprintf(stderr, "get_mhit seek: %x\n", (int)seek);
#endif

  if (seek_get_n_bytes (file, data, seek, 4) != 4) return -1;
  if (cmp_n_bytes (data, "mhit", 4) == FALSE ) return -1; /* we are lost! */

  track = g_malloc0 (sizeof (Track));

  track->ipod_id = get4int(file, seek+16);     /* iPod ID          */
  track->rating = get4int(file, seek+28) >> 24;/* rating           */
  track->time_modified = get4int(file, seek+32);/* modification time    */
  track->size = get4int(file, seek+36);        /* file size        */
  track->tracklen = get4int(file, seek+40);    /* time             */
  track->track_nr = get4int(file, seek+44);    /* track number     */
  track->tracks = get4int(file, seek+48);      /* nr of tracks     */
  track->year = get4int(file, seek+52);        /* year             */
  track->bitrate = get4int(file, seek+56);     /* bitrate          */
  track->volume = get4int(file, seek+64);      /* volume adjust    */
  track->playcount = get4int(file, seek+80);   /* playcount        */
  track->time_played = get4int(file, seek+88); /* last time played */
  track->cd_nr = get4int(file, seek+92);       /* CD nr            */
  track->cds = get4int(file, seek+96);         /* CD nr of..       */
  track->transferred = TRUE;                   /* track is on iPod! */

#if ITUNESDB_MHIT_DEBUG
time_t time_mac_to_host (guint32 mactime);
gchar *time_time_to_string (time_t time);
#define printf_mhit(sk, str)  printf ("%3d: %d (%s)\n", sk, get4int (file, seek+sk), str);
#define printf_mhit_time(sk, str) { gchar *buf = time_time_to_string (itunesdb_time_mac_to_host (get4int (file, seek+sk))); printf ("%3d: %s (%s)\n", sk, buf, str); g_free (buf); }
  {
      printf ("\nmhit: seek=%lu\n", seek);
      printf_mhit (  4, "header size");
      printf_mhit (  8, "mhit size");
      printf_mhit ( 12, "nr of mhods");
      printf_mhit ( 16, "iPod ID");
      printf_mhit ( 20, "?");
      printf_mhit ( 24, "?");
      printf (" 28: %u (type)\n", get4int (file, seek+28) & 0xffffff);
      printf (" 28: %u (rating)\n", get4int (file, seek+28) >> 24);
      printf_mhit ( 32, "timestamp file");
      printf_mhit_time ( 32, "timestamp file");
      printf_mhit ( 36, "size");
      printf_mhit ( 40, "tracklen (ms)");
      printf_mhit ( 44, "track_nr");
      printf_mhit ( 48, "total tracks");
      printf_mhit ( 52, "year");
      printf_mhit ( 56, "bitrate");
      printf_mhit ( 60, "sample rate");
      printf (" 60: %u (sample rate LSB)\n", get4int (file, seek+60) & 0xffff);
      printf (" 60: %u (sample rate HSB)\n", (get4int (file, seek+60) >> 16));
      printf_mhit ( 64, "?");
      printf_mhit ( 68, "?");
      printf_mhit ( 72, "?");
      printf_mhit ( 76, "?");
      printf_mhit ( 80, "playcount");
      printf_mhit ( 84, "?");
      printf_mhit ( 88, "last played");
      printf_mhit_time ( 88, "last played");
      printf_mhit ( 92, "CD");
      printf_mhit ( 96, "total CDs");
      printf_mhit (100, "?");
      printf_mhit (104, "?");
      printf_mhit_time (104, "?");
      printf_mhit (108, "?");
      printf_mhit (112, "?");
      printf_mhit (116, "?");
      printf_mhit (120, "?");
      printf_mhit (124, "?");
      printf_mhit (128, "?");
      printf_mhit (132, "?");
      printf_mhit (136, "?");
      printf_mhit (140, "?");
      printf_mhit (144, "?");
      printf_mhit (148, "?");
      printf_mhit (152, "?");
  }
#undef printf_mhit_time
#undef printf_mhit
#endif

  seek += get4int (file, seek+4);             /* 1st mhod starts here! */
  while(zip != -1)
    {
     seek += zip;
     entry_utf16 = get_mhod (file, seek, &zip, &type);
     if (entry_utf16 != NULL) {
#ifdef ITUNESDB_PROVIDE_UTF8
       entry_utf8 = g_utf16_to_utf8 (entry_utf16, -1, NULL, NULL, NULL);
#endif
       switch (type)
	 {
	 case MHOD_ID_ALBUM:
#ifdef ITUNESDB_PROVIDE_UTF8
	   track->album = entry_utf8;
#endif
	   track->album_utf16 = entry_utf16;
	   break;
	 case MHOD_ID_ARTIST:
#ifdef ITUNESDB_PROVIDE_UTF8
	   track->artist = entry_utf8;
#endif
	   track->artist_utf16 = entry_utf16;
	   break;
	 case MHOD_ID_TITLE:
#ifdef ITUNESDB_PROVIDE_UTF8
	   track->title = entry_utf8;
#endif
	   track->title_utf16 = entry_utf16;
	   break;
	 case MHOD_ID_GENRE:
#ifdef ITUNESDB_PROVIDE_UTF8
	   track->genre = entry_utf8;
#endif
	   track->genre_utf16 = entry_utf16;
	   break;
	 case MHOD_ID_PATH:
#ifdef ITUNESDB_PROVIDE_UTF8
	   track->ipod_path = entry_utf8;
#endif
	   track->ipod_path_utf16 = entry_utf16;
	   break;
	 case MHOD_ID_FDESC:
#ifdef ITUNESDB_PROVIDE_UTF8
	   track->fdesc = entry_utf8;
#endif
	   track->fdesc_utf16 = entry_utf16;
	   break;
	 case MHOD_ID_COMMENT:
#ifdef ITUNESDB_PROVIDE_UTF8
	   track->comment = entry_utf8;
#endif
	   track->comment_utf16 = entry_utf16;
	   break;
	 case MHOD_ID_COMPOSER:
#ifdef ITUNESDB_PROVIDE_UTF8
	   track->composer = entry_utf8;
#endif
	   track->composer_utf16 = entry_utf16;
	   break;
	 default: /* unknown entry -- discard */
#ifdef ITUNESDB_PROVIDE_UTF8
	   g_free (entry_utf8);
#endif
	   g_free (entry_utf16);
	   break;
	 }
     }
    }

  playcount = get_next_playcount ();
  if (playcount)
  {
      if (playcount->rating)  track->rating = playcount->rating;
      if (playcount->time_played) track->time_played = playcount->time_played;
      track->playcount += playcount->playcount;
      track->recent_playcount = playcount->playcount;
      g_free (playcount);
  }
  it_add_track (track);
  return seek;   /* no more black magic */
}

/* get next playcount, that is the first entry of GList
 * playcounts. This entry is removed from the list. You must free the
 * return value after use */
static struct playcount *get_next_playcount (void)
{
    struct playcount *playcount = g_list_nth_data (playcounts, 0);

    if (playcount)  playcounts = g_list_remove (playcounts, playcount);
    return playcount;
}

/* delete all entries of GList *playcounts */
static void reset_playcounts (void)
{
    struct playcount *playcount;
    while ((playcount=get_next_playcount())) g_free (playcount);
}

/* Read the Play Count file (formed by adding "Play Counts" to the
 * directory contained in @filename) and set up the GList *playcounts
 * */
static void init_playcounts (const gchar *filename)
{
  gchar *dirname = g_path_get_dirname(filename);
  gchar *plcname = g_build_filename1(dirname, "Play Counts", NULL);
  FILE *plycts = fopen (plcname, "r");
  gboolean error = TRUE;

  reset_playcounts ();

  if (plycts) do
  {
      gchar data[4];
      guint32 header_length, entry_length, entry_num, i=0;
      time_t tt = time (NULL);

      localtime (&tt);  /* set the ext. variable 'timezone' (see below) */
      if (seek_get_n_bytes (plycts, data, 0, 4) != 4)  break;
      if (cmp_n_bytes (data, "mhdp", 4) == FALSE)      break;
      header_length = get4int (plycts, 4);
      /* all the headers I know are 0x60 long -- if this one is longer
	 we can simply ignore the additional information */
      if (header_length < 0x60)                        break;
      entry_length = get4int (plycts, 8);
      /* all the entries I know are 0x0c (firmware 1.3) or 0x10
       * (firmware 2.0) in length */
      if (entry_length < 0x0c)                         break;
      /* number of entries */
      entry_num = get4int (plycts, 12);
      for (i=0; i<entry_num; ++i)
      {
	  struct playcount *playcount = g_malloc0 (sizeof (struct playcount));
	  glong seek = header_length + i*entry_length;

	  playcounts = g_list_append (playcounts, playcount);
	  /* check if entry exists by reading its last four bytes */
	  if (seek_get_n_bytes (plycts, data,
				seek+entry_length-4, 4) != 4) break;
	  playcount->playcount = get4int (plycts, seek);
	  playcount->time_played = get4int (plycts, seek+4);
          /* NOTE:
	   *
	   * The iPod (firmware 1.3) doesn't seem to use the timezone
	   * information correctly -- no matter what you set iPod's
	   * timezone to it will always record in UTC -- we need to
	   * subtract the difference between current timezone and UTC
	   * to get a correct display. 'timezone' (initialized above)
	   * contains the difference in seconds.
           */
	  if (playcount->time_played)
	      playcount->time_played += timezone;

	  /* rating only exists if the entry length is at least 0x10 */
	  if (entry_length >= 0x10)
	      playcount->rating = get4int (plycts, seek+12);
      }
      if (i == entry_num)  error = FALSE;
  } while (FALSE);
  if (plycts)  fclose (plycts);
  if (error)   reset_playcounts ();
  g_free (dirname);
  g_free (plcname);
}


/* Parse the iTunesDB and store the tracks using it_addtrack () defined
   in track.c.
   Returns TRUE on success, FALSE on error.
   "path" should point to the mount point of the iPod,
   e.g. "/mnt/ipod" and be in local encoding */
/* Support for playlists should be added later */
gboolean itunesdb_parse (const gchar *path)
{
  gchar *filename;
  const gchar *db[] = {"iPod_Control","iTunes","iTunesDB",NULL};
  gboolean result;

  filename = resolve_path(path, db);
  result = itunesdb_parse_file (filename);
  g_free (filename);
  return result;
}

/* Same as itunesdb_parse(), but let's specify the filename directly */
gboolean itunesdb_parse_file (const gchar *filename)
{
  FILE *itunes = NULL;
  gboolean result = FALSE;
  gchar data[8];
  glong seek=0, pl_mhsd=0;
  guint32 zip, nr_tracks=0, nr_playlists=0;
  gboolean swapped_mhsd = FALSE;

#if ITUNESDB_DEBUG
  fprintf(stderr, "Parsing %s\nenter: %4d\n", filename, it_get_nr_of_tracks ());
#endif

  if (!filename) return FALSE;

  itunes = fopen (filename, "r");
  do
  { /* dummy loop for easier error handling */
      if (itunes == NULL)
      {
	  itunesdb_warning (_("Could not open iTunesDB \"%s\" for reading.\n"),
			  filename);
	  break;
      }
      if (seek_get_n_bytes (itunes, data, seek, 4) != 4)
      {
	  itunesdb_warning (_("Error reading \"%s\".\n"), filename);
	  break;
      }
      /* for(i=0; i<8; ++i)  printf("%02x ", data[i]); printf("\n");*/
      if (cmp_n_bytes (data, "mhbd", 4) == FALSE)
      {
	  itunesdb_warning (_("\"%s\" is not a iTunesDB.\n"), filename);
	  break;
      }
      seek = get4int (itunes, 4);
      /* all the headers I know are 0x68 long -- if this one is longer
	 we can simply ignore the additional information */
      /* we don't need any information of the mhbd header... */
      /*      if (seek < 0x68)
      {
	  itunesdb_warning (_("\"%s\" is not a iTunesDB.\n"), filename);
	  break;
	  }*/
      do
      {
	  if (seek_get_n_bytes (itunes, data, seek, 8) != 8)  break;
	  if (cmp_n_bytes (data, "mhsd", 4) == TRUE)
	  { /* mhsd header -> determine start of playlists */
	      if (get4int (itunes, seek + 12) == 1)
	      { /* OK, tracklist, save start of playlists */
		  if (!swapped_mhsd)
		      pl_mhsd = seek + get4int (itunes, seek+8);
	      }
	      else if (get4int (itunes, seek + 12) == 2)
	      { /* bad: these are playlists... switch */
		  if (swapped_mhsd)
		  { /* already switched once -> forget it */
		      break;
		  }
		  else
		  {
		      pl_mhsd = seek;
		      seek += get4int (itunes, seek+8);
		      swapped_mhsd = TRUE;
		  }
	      }
	      else
	      { /* neither playlist nor track MHSD --> skip it */
		  seek += get4int (itunes, seek+8);
	      }
	  }
	  if (cmp_n_bytes (data, "mhlt", 4) == TRUE)
	  { /* mhlt header -> number of tracks */
	      nr_tracks = get4int (itunes, seek+8);
	      if (nr_tracks == 0)
	      {   /* no tracks -- skip directly to next mhsd */
		  result = TRUE;
		  break;
	      }
	  }
	  if (cmp_n_bytes (data, "mhit", 4) == TRUE)
	  { /* mhit header -> start of tracks*/
	      result = TRUE;
	      break;
	  }
	  zip = get4int (itunes, seek+4);
	  if (zip == 0)  break;
	  seek += zip;
      } while (result == FALSE);
      if (result == FALSE)  break; /* some error occured */
      result = FALSE;
      /* now we should be at the first MHIT */

      /* Read Play Count file if available */
      init_playcounts (filename);

      /* get every file entry */
      if (nr_tracks)  while(seek != -1) {
	  /* get_mhit returns where it's guessing the next MHIT,
	     if it fails, it returns '-1' */
	  seek = get_mhit (itunes, seek);
      }

      /* next: playlists */
      seek = pl_mhsd;
      do
      {
	  if (seek_get_n_bytes (itunes, data, seek, 8) != 8)  break;
	  if (cmp_n_bytes (data, "mhsd", 4) == TRUE)
	  { /* mhsd header */
	      if (get4int (itunes, seek + 12) != 2)
	      {  /* this is not a playlist MHSD -> skip it */
		  seek += get4int (itunes, seek+8);
	      }
	  }
	  if (cmp_n_bytes (data, "mhlp", 4) == TRUE)
	  { /* mhlp header -> number of playlists */
	      nr_playlists = get4int (itunes, seek+8);
	  }
	  if (cmp_n_bytes (data, "mhyp", 4) == TRUE)
	  { /* mhyp header -> start of playlists */
	      result = TRUE;
	      break;
	  }
	  zip = get4int (itunes, seek+4);
	  if (zip == 0)  break;
	  seek += zip;
      } while (result == FALSE);
      if (result == FALSE)  break; /* some error occured */
      result = FALSE;

#if ITUNESDB_DEBUG
    fprintf(stderr, "iTunesDB part2 starts at: %x\n", (int)seek);
#endif

    while(seek != -1) {
	seek = get_pl(itunes, seek);
    }

    result = TRUE;
  } while (FALSE);

  if (itunes != NULL)     fclose (itunes);
#if ITUNESDB_DEBUG
  fprintf(stderr, "exit:  %4d\n", it_get_nr_of_tracks ());
#endif
  return result;
}



/*
 * this is picked up from files.c
 */
/* There seems to be a problem with some distributions
      (kernel versions or whatever -- even identical version
      numbers don't don't show identical behaviour...): even
      though vfat is supposed to be case insensitive, a
     difference is made between upper and lower case under
     some special circumstances. As in
     "/iPod_Control/Music/F00" and "/iPod_Control/Music/f00
     "... If the former filename does not exist, we try to find an existing
     case insensitive match for each component of the filename. 
     If we can find such a match, we return it.  Otherwise, we return NULL.*/
     
   /* We start by assuming that the ipod mount point exists.  Then, for each
    * component c of track->ipod_path, we try to find an entry d in good_path that
    * is case-insensitively equal to c.  If we find d, we append d to good_path and make
    * the result the new good_path.  Otherwise, we quit and return
   NULL.
   @root: in local encoding, @components: in utf8
 */
gchar * resolve_path(const gchar *root,const gchar * const * components)
{
	gchar *good_path = g_strdup(root);
	guint32 i;
    
	for(i = 0 ; components[i] ; i++) {
		GDir *cur_dir;
		gchar *component_as_filename;
		gchar *test_path;
		gchar *component_stdcase;
		const gchar *dir_file=NULL;

		/* skip empty components */
		if (strlen (components[i]) == 0) continue;
		component_as_filename = 
			g_filename_from_utf8(components[i],-1,NULL,NULL,NULL);
		test_path = g_build_filename1(good_path,component_as_filename,NULL);
		g_free(component_as_filename);
		if(g_file_test(test_path,G_FILE_TEST_EXISTS)) {
			/* This component does not require fixup */
			g_free(good_path);
			good_path = test_path;
			continue;
		}
		g_free(test_path);
		component_stdcase = g_utf8_casefold(components[i],-1);
		/* Case insensitively compare the current component with each entry
		 * in the current directory. */

		cur_dir = g_dir_open(good_path,0,NULL);
		if (cur_dir) while ((dir_file = g_dir_read_name(cur_dir)))
		{
			gchar *file_utf8 = g_filename_to_utf8(dir_file,-1,NULL,NULL,NULL);
			gchar *file_stdcase = g_utf8_casefold(file_utf8,-1);
			gboolean found = !g_utf8_collate(file_stdcase,component_stdcase);
			gchar *new_good_path;
			g_free(file_stdcase);
			if(!found)
			{
				/* This is not the matching entry */
				g_free(file_utf8);
				continue;
			}
      
			new_good_path = dir_file ? g_build_filename1(good_path,dir_file,NULL) : NULL;
			g_free(good_path);
			good_path= new_good_path;
			/* This is the matching entry, so we can stop searching */
			break;
		}
    
		if(!dir_file) {
			/* We never found a matching entry */
			g_free(good_path);
			good_path = NULL;
		}

		g_free(component_stdcase);
		if (cur_dir) g_dir_close(cur_dir);
		if(!good_path || !g_file_test(good_path,G_FILE_TEST_EXISTS))
			break; /* We couldn't fix this component, so don't try later ones */
	}
    
	if(good_path && g_file_test(good_path,G_FILE_TEST_EXISTS))
		return good_path;
          
	return NULL;
}


