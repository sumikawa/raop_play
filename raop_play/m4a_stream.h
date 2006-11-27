/*****************************************************************************
 * m4a_stream.h: .m4a file stream, header file
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
#ifndef __M4A_STREAM_H_
#define __M4A_STREAM_H_

typedef struct m4a_t {__u32 dummy;} m4a_t;

int m4a_open(auds_t *auds, char *fname);
int m4a_close(auds_t *auds);
int m4a_get_top_sample(auds_t *auds, __u8 **data, int *size);
int m4a_get_next_sample(auds_t *auds, __u8 **data, int *size);


#endif
