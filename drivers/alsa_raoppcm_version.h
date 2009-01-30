/*****************************************************************************
 * alsa_raoppcm_version.h: alsa pcm driver to bridge to raop_play
 * Copyright (C) 2008 Shiro Ninomiya <shiron@snino.com>, 
 * 						Nils Winkler <nwinkler@users.sourceforge.net>
 * 
 * Reference article:
 *   http://alsa-project.org/~iwai/writing-an-alsa-driver/index.html
 *
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


#ifndef __ALSA_RAOPPCM_VERSION_H__
#define __ALSA_RAOPPCM_VERSION_H__




/******************************************************************************
   Includes
 *****************************************************************************/
#include <linux/autoconf.h>
#include <linux/version.h>




/******************************************************************************
   Constant Definitions
 *****************************************************************************/
#define DRIVER_AUTHOR           "Shiro Ninomiya <shiron@snino.com>, Nils Winkler <nwinkler@users.sourceforge.net>"
#define DRIVER_LICENSE          "GPL"
#define DRIVER_DEVICE_STRING    "ALSA_RAOPPCM"
#define DRIVER_NAME             "ALSA_RAOPPCM"
#define DRIVER_MAJOR_VERSION    0
#define DRIVER_MINOR_VERSION    5
#define DRIVER_PATCH_VERSION    2
#define DRIVER_VERSION_STRING   "0.5.2"
#define DRIVER_BUILD_DATE       "01/31/2006 15:40:00"
#define DRIVER_DESC             "ALSA Driver for streaming audio data to an Apple Airport Express"

#define STRUCT_MODULE           "???" 

#define DRIVER_INFO             DRIVER_DESC " for the "\
                                DRIVER_DEVICE_STRING ", v" \
                                DRIVER_VERSION_STRING " " \
                                DRIVER_BUILD_DATE 


#if ( LINUX_VERSION_CODE < KERNEL_VERSION( 2,6,0 ))
#define DRIVER_NAME_EXT         "ALSA_RAOPPCM.o"
#else
#define DRIVER_NAME_EXT         "ALSA_RAOPPCM.ko"
#endif




#endif  /* __ALSA_RAOPPCM_VERSION_H__ */
