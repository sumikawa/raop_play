/*****************************************************************************
 * wav_stream.c: wave file stream
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
#include <stdio.h>
#include <unistd.h>
#define WAV_STREAM_C
#include "audio_stream.h"
#include "wav_stream.h"
#include "aexcl_lib.h"

static int read_wave_header(wav_t *wav, int *sample_rate, int *channels);

int wav_open(auds_t *auds, char *fname)
{
	wav_t *wav=malloc(sizeof(wav_t));
	if(!wav) return -1;
	memset(wav,0,sizeof(wav_t));
	auds->stream=(void *)wav;
	wav->inf=fopen(fname,"r");
	if(!wav->inf) goto erexit;
	if(read_wave_header(wav, &auds->sample_rate, &auds->channels)==-1) goto erexit;
	auds->chunk_size=aud_clac_chunk_size(auds->sample_rate);
	wav->buffer=(u_int8_t *)malloc(MAX_SAMPLES_IN_CHUNK*4+16);
	if(!wav->buffer) goto erexit;
	return 0;
 erexit:
	wav_close(auds);
	return -1;
}

int wav_close(auds_t *auds)
{
	wav_t *wav=(wav_t *)auds->stream;
	if(!wav) return -1;
	if(wav->inf) fclose(wav->inf);
	if(wav->buffer) free(wav->buffer);
	free(wav);
	auds->stream=NULL;
	return 0;
}

int wav_get_top_sample(auds_t *auds, u_int8_t **data, int *size)
{
	wav_t *wav=(wav_t *)auds->stream;
	fseek(wav->inf,sizeof(wave_header_t),SEEK_SET);
	wav->playedbytes=0;
	return wav_get_next_sample(auds, data, size);
}

int wav_get_next_sample(auds_t *auds, u_int8_t **data, int *size)
{
	wav_t *wav=(wav_t *)auds->stream;
	int bsize=(wav->subchunk2size - wav->playedbytes>=auds->chunk_size)?
		auds->chunk_size:wav->subchunk2size - wav->playedbytes;
	data_source_t ds={.type=STREAM, .u.inf=wav->inf};
	if(!bsize) return -1;
	wav->playedbytes+=bsize;
	return auds_write_pcm(auds, wav->buffer, data, size, bsize, &ds);
}


static int read_wave_header(wav_t *wav, int *sample_rate, int *channels)
{
	wave_header_t head;
	FILE *infile=wav->inf;
	
	if(fread(&head,1,sizeof(head),infile)<sizeof(head)) return -1;
	if(strncmp(head.charChunkID,"RIFF",4) || strncmp(head.Format,"WAVE",4)){
		ERRMSG("This is not a WAV file\n");
		return -1;
	}
	*channels=head.NumChannels;
	if(head.NumChannels!=2 && head.NumChannels!=1){
		ERRMSG("This is neither stereo nor mono, NumChannels=%d\n",head.NumChannels);
		return -1;
	}
	if(head.BitsPerSample!=16){
		ERRMSG("bits per sample = %d, we need 16 bits data\n", head.BitsPerSample);
		return -1;
	}
	*sample_rate=head.SampleRate;
#if 0	
	if(head.SampleRate!=DEFAULT_SAMPLE_RATE){
		ERRMSG("sample rate = %d, we need %d\n", head.SampleRate,DEFAULT_SAMPLE_RATE);
		return -1;
	}
#endif	
	if(strncmp(head.Subchunk2ID,"data",4)){
		ERRMSG("sub chunk is not \"data\"\n");
		//return -1;
	}
	wav->subchunk2size=head.Subchunk2Size;
	return 0;
}

