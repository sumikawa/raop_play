/*****************************************************************************
 * mp3_stream.h: mp3 file stream, header file
 *
 * Copyright (C) 2005 Shiro Ninomiya <shiron@snino.com>
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
#ifndef __AAC_STREAM_H_
#define __AAC_STREAM_H_


typedef struct aac_t {
/* public variables */
/* private variables */
#ifdef AAC_STREAM_C
	FILE *inf;
	int dpid;
	__u8 *buffer;
	char *fname;
#else
	__u32 dummy;
#endif
} aac_t;


int aac_open(auds_t *auds, char *fname);
int aac_close(auds_t *auds);
int aac_get_top_sample(auds_t *auds, __u8 **data, int *size);
int aac_get_next_sample(auds_t *auds, __u8 **data, int *size);


#endif