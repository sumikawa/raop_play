/*****************************************************************************
 * ogg_stream.c: ogg file stream
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
#include <asm/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#define OGG_STREAM_C
#include "audio_stream.h"
#include "ogg_stream.h"
#include "aexcl_lib.h"

#define OGG_DECODER "ogg123"
#define FIFO_NAME "/tmp/ogg_stream"
static int run_decoder(ogg_t *ogg)
{
	char *darg[9]={OGG_DECODER,"-d","raw", "-o", "byteorder:little", "-f",
		       NULL, NULL, NULL};
	darg[6]=FIFO_NAME;
	darg[7]=ogg->fname;
	ogg->dpid=child_start(darg,NULL,NULL,NULL);
	if(!(ogg->inf=fopen(FIFO_NAME,"r"))) {
		ERRMSG("can't open fifo\n");
		return -1;
	}
	return 0;
}

static int stop_decoder(ogg_t *ogg)
{
	int i;

	if(!ogg->dpid) return 0;
	kill(ogg->dpid,SIGTERM);
	for(i=0;i<10;i++){
		if(!ogg->dpid) return 0;
		usleep(10*1000);
	}
	ERRMSG("decoder process can't be terminated\n");
	return 0;
}

static void sigchld_callback(void *p, siginfo_t *siginfo)
{
	ogg_t *ogg=(ogg_t*)p;
	if(siginfo->si_pid==ogg->dpid){
		waitpid(ogg->dpid,NULL,0);
		ogg->dpid=0;
	}
}

typedef struct vorbis_head_t{
	__u8 packtype;
	__u8 name[6];
	__u32 version;
	__u8 channel;
	__u32 rate;
	__u32 bitrate_upper;
	__u32 bitrate_nominal;
	__u32 bitrate_lower;
	__u32 blocksize[2];
}__attribute__ ((packed)) vorbis_head_t;

static int read_header(auds_t *auds, char *fname)
{
	FILE *inf;
	__u8 head[1024];
	vorbis_head_t *vh;
	int rsize,i,j;
	
	inf=fopen(fname, "r");
	if(!inf) return -1;
	rsize=fread(head,1,1024,inf);
	fclose(inf);
	for(i=0;i<rsize-4;i++){
		if(!memcmp(head+i,"OggS",4)) break;
	}
	if(rsize-i-4<sizeof(vorbis_head_t)){
		INFMSG("looks not ogg file\n");
		return -1;
	}
	for(j=i+4;j<rsize-sizeof(vorbis_head_t);j++){
		if(!memcmp(head+j,"\001vorbis",7)) break;
	}
	if(j==rsize-sizeof(vorbis_head_t)){
		INFMSG("not vorbis, can't get sample rate info.\n");
		return -1;
	}
	vh=(vorbis_head_t *)(head+j);
	//DBGMSG("channel=%d, sample rate=%d, bitrate=%d",
	//       vh->channel, vh->rate, vh->bitrate_nominal);
	auds->sample_rate=vh->rate;
	auds->channels=vh->channel;
	return 0;
}

int ogg_open(auds_t *auds, char *fname)
{
	ogg_t *ogg=malloc(sizeof(ogg_t));
	if(!ogg) return -1;
	memset(ogg,0,sizeof(ogg_t));
	auds->stream=(void*)ogg;
	ogg->fname=(char *)malloc(strlen(fname)+1);
	if(!ogg->fname) goto erexit;
	strcpy(ogg->fname,fname);
	ogg->buffer=(__u8 *)malloc(MAX_SAMPLES_IN_CHUNK*4+16);
	if(!ogg->buffer) goto erexit;
	auds->sigchld_cb=sigchld_callback;
	if(access(FIFO_NAME,F_OK) && mkfifo(FIFO_NAME,0600)<0){
		ERRMSG("can't make a named fifo:%s\n",FIFO_NAME);
		goto erexit;
	}
	auds->sample_rate=DEFAULT_SAMPLE_RATE;
	read_header(auds, fname);
	if(run_decoder(ogg)) goto erexit;
	auds->chunk_size=aud_clac_chunk_size(auds->sample_rate);
	return 0;
 erexit:
	ogg_close(auds);
	return -1;
}

int ogg_close(auds_t *auds)
{
	ogg_t *ogg=(ogg_t *)auds->stream;
	if(!ogg) return -1;
	if(ogg->inf) fclose(ogg->inf);
	stop_decoder(ogg);
	unlink(FIFO_NAME);
	if(ogg->buffer) free(ogg->buffer);
	if(ogg->fname) free(ogg->fname);
	free(ogg);
	auds->stream=NULL;
	return 0;
}

int ogg_get_top_sample(auds_t *auds, __u8 **data, int *size)
{
	ogg_t *ogg=(ogg_t *)auds->stream;
	stop_decoder(ogg);
	run_decoder(ogg);
	return ogg_get_next_sample(auds, data, size);
}

int ogg_get_next_sample(auds_t *auds, __u8 **data, int *size)
{
	ogg_t *ogg=(ogg_t *)auds->stream;
	data_source_t ds={.type=STREAM, .u.inf=ogg->inf};
	return auds_write_pcm(auds, ogg->buffer, data, size, auds->chunk_size, &ds);
}

