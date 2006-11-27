/* Time-stamp: <2004-03-24 23:06:13 JST jcs>
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
|  $Id: itunesdb.h,v 1.1.1.1 2005/07/23 13:57:04 shiro Exp $
*/

#ifndef __ITUNESDB_H__
#define __ITUNESDB_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

//#include "track.h"
#include <glib.h>

#undef BEGIN_C_DECLS
#undef END_C_DECLS
#ifdef __cplusplus
#define BEGIN_C_DECLS extern "C" {
#define END_C_DECLS }
#else
#define BEGIN_C_DECLS
#define END_C_DECLS
#endif


BEGIN_C_DECLS

typedef struct
{
  gchar   *album;            /* album (utf8)          */
  gchar   *artist;           /* artist (utf8)         */
  gchar   *title;            /* title (utf8)          */
  gchar   *genre;            /* genre (utf8)          */
  gchar   *comment;          /* comment (utf8)        */
  gchar   *composer;         /* Composer (utf8)       */
  gchar   *fdesc;            /* ? (utf8)              */
  gchar   *ipod_path;        /* name of file on iPod: uses ":" instead of "/"*/
  gunichar2 *album_utf16;    /* album (utf16)         */
  gunichar2 *artist_utf16;   /* artist (utf16)        */
  gunichar2 *title_utf16;    /* title (utf16)         */
  gunichar2 *genre_utf16;    /* genre (utf16)         */
  gunichar2 *comment_utf16;  /* comment (utf16)       */
  gunichar2 *composer_utf16; /* Composer (utf16)      */
  gunichar2 *fdesc_utf16;    /* ? (utf16)             */
  gunichar2 *ipod_path_utf16;/* name of file on iPod: uses ":" instead of "/"*/
  gchar   *pc_path_utf8;     /* PC filename in utf8 encoding   */
  gchar   *pc_path_locale;   /* PC filename in locale encoding */
  guint32 ipod_id;           /* unique ID of track    */
  gint32  size;              /* size of file in bytes */
  gint32  oldsize;           /* used when updating tracks: size on iPod */
  gint32  tracklen;          /* Length of track in ms */
  gint32  cd_nr;             /* CD number             */
  gint32  cds;               /* number of CDs         */
  gint32  track_nr;          /* track number          */
  gint32  tracks;            /* number of tracks      */
  gint32  bitrate;           /* bitrate               */
  gint32  year;              /* year                  */
  gchar   *year_str;         /* year as string -- always identical to year */
  gint32  volume;            /* volume adjustment between -100 and +100    */
  guint32 peak_signal;	     /* LAME Peak Signal * 0x800000 */
  gint    radio_gain;	     /* RadioGain in dB*10 (as defined by www.replaygain.org) */
  gint    audiophile_gain;   /* AudiophileGain in dB*10 
				(as defined by www.replaygain.org)  */
  gboolean peak_signal_set;  /* has the peak signal been set?       */
  gboolean radio_gain_set;   /* has the radio gain been set?        */
  gboolean audiophile_gain_set;/* has the audiophile gain been set? */
  guint32 time_played;       /* time of last play  (Mac type)              */
  guint32 time_modified;     /* time of last modification  (Mac type)      */
  guint32 rating;            /* star rating (stars * RATING_STEP (20))     */
  guint32 playcount;         /* number of times track was played           */
  guint32 recent_playcount;  /* times track was played since last sync     */
  gchar   *hostname;         /* name of host this file has been imported on*/
  gboolean transferred;      /* has file been transferred to iPod?         */
  gchar   *md5_hash;         /* md5 hash of file (or NULL)                 */
  gchar   *charset;          /* charset used for ID3 tags                  */
} Track;

#include <time.h>
//#include "playlist.h"
// copied from playlist.h
typedef struct
{
    gchar *name;          /* name of playlist in UTF8 */
    gunichar2 *name_utf16;/* name of playlist in UTF16 */
    guint32 type;         /* 1: master play list (PL_TYPE_MPL) */
    gint  num;            /* number of tracks in playlist */
    GList *members;       /* tracks in playlist (Track *) */
    glong size;
} Playlist;

enum { /* types for playlist->type */
    PL_TYPE_NORM = 0,       /* normal playlist, visible in iPod */
    PL_TYPE_MPL = 1         /* master playlist, contains all tracks,
			       not visible in iPod */
};

gboolean itunesdb_parse (const gchar *path);
gboolean itunesdb_parse_file (const gchar *filename);
gboolean itunesdb_write (const gchar *path);
gboolean itunesdb_write_to_file (const gchar *filename);
gboolean itunesdb_copy_track_to_ipod (const gchar *path, Track *track,
				      const gchar *pcfile);
gchar *itunesdb_get_track_name_on_ipod (const gchar *path, Track *s);
gboolean itunesdb_cp (const gchar *from_file, const gchar *to_file);
guint32 itunesdb_time_get_mac_time (void);
void itunesdb_convert_filename_fs2ipod (gchar *ipod_file);
void itunesdb_convert_filename_ipod2fs (gchar *ipod_file);
time_t itunesdb_time_mac_to_host (guint32 mactime);
guint32 itunesdb_time_host_to_mac (time_t time);

gchar * resolve_path(const gchar *root,const gchar * const * components);

// in main programsn
gboolean it_add_track (Track *track);
Playlist *it_add_playlist (Playlist *plitem);
void it_add_trackid_to_playlist (Playlist *plitem, guint32 id);


END_C_DECLS
#endif
