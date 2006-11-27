/*****************************************************************************
 * ogg_stream.h: ogg file stream, header file
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
#ifndef __OGG_STREAM_H_
#define __OGG_STREAM_H_


typedef struct ogg_t {
/* public variables */
/* private variables */
#ifdef OGG_STREAM_C
	FILE *inf;
	int dpid;
	u_int8_t *buffer;
	char *fname;
#else
	u_int32_t dummy;
#endif
} ogg_t;


int ogg_open(auds_t *auds, char *fname);
int ogg_close(auds_t *auds);
int ogg_get_top_sample(auds_t *auds, u_int8_t **data, int *size);
int ogg_get_next_sample(auds_t *auds, u_int8_t **data, int *size);


#endif
