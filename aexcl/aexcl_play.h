/*****************************************************************************
 * aexcl_play.h : Apple Airport Express Client Player
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
#ifndef __AEXCL_PLAY_H_
#define __AEXCL_PLAY_H_

typedef enum aex_cntl_enum {
	AEX_STOP=0,
	AEX_PLAY,
	AEX_PAUSE,
	AEX_FWD,
	AEX_REW,
	AEX_NEXT_TRACK,
	AEX_PREV_TRACK,
	AEX_VOL,
} aex_cntl_enum_t;

#endif
