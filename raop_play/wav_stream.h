/*****************************************************************************
 * wav_stream.h: wave file stream, header file
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
#ifndef __WAV_STREAM_H_
#define __WAV_STREAM_H_

typedef struct wave_header_t{
	char charChunkID[4];
	u_int32_t ChunkSize;
	char Format[4];
	char Subchunk1ID[4];
	u_int32_t Subchunk1Size;
	u_int16_t AudioFormat;
	u_int16_t NumChannels;
	u_int32_t SampleRate;
	u_int32_t ByteRate;
	u_int16_t BlockAlign;
	u_int16_t BitsPerSample;
	char Subchunk2ID[4];
	u_int32_t Subchunk2Size;
} __attribute__ ((packed)) wave_header_t;

typedef struct wav_t {
/* public variables */
/* private variables */
#ifdef WAV_STREAM_C
	FILE *inf;
	u_int8_t *buffer;
	int subchunk2size;
	int playedbytes;
#else
	u_int32_t dummy;
#endif
} wav_t;


int wav_open(auds_t *auds, char *fname);
int wav_close(auds_t *auds);
int wav_get_top_sample(auds_t *auds, u_int8_t **data, int *size);
int wav_get_next_sample(auds_t *auds, u_int8_t **data, int *size);


#endif
