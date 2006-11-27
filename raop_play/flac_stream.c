/*****************************************************************************
 * flac_stream.c: flac file stream
 *
 * Copyright (C) 2005 Graziano Obertelli <graziano@acm.org>
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
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#define FLAC_STREAM_C
#include "audio_stream.h"
#include "flac_stream.h"
#include "aexcl_lib.h"

#define FLAC_DECODER "flac"
#define FIFO_NAME "/tmp/flac_stream"
static int run_decoder(flac_t *flac)
{
	char *darg[10]={FLAC_DECODER,"-d","--force-raw-format", 
		"--endian=little", "--sign=signed", "-f", "-o", NULL, NULL, NULL};
	darg[7]=FIFO_NAME;
	darg[8]=flac->fname;
	flac->dpid=child_start(darg,NULL,NULL,NULL);
	if(!(flac->inf=fopen(FIFO_NAME,"r"))) {
		ERRMSG("can't open fifo\n");
		return -1;
	}
	return 0;
}

static int stop_decoder(flac_t *flac)
{
	int i;

	if(!flac->dpid) return 0;
	kill(flac->dpid,SIGTERM);
	for(i=0;i<10;i++){
		if(!flac->dpid) return 0;
		usleep(10*1000);
	}
	ERRMSG("decoder process can't be terminated\n");
	return 0;
}

static void sigchld_callback(void *p, siginfo_t *siginfo)
{
	flac_t *flac=(flac_t*)p;
	if(siginfo->si_pid==flac->dpid){
		waitpid(flac->dpid,NULL,0);
		flac->dpid=0;
	}
}

#define NEEDED_LEN (1 + 3 + 10 + 8)
static int read_header(auds_t *auds, char *fname)
{
	FILE *inf;
	u_int8_t head[128], tmp;
	int rsize,i;
	unsigned int j;
	
	inf=fopen(fname, "r");
	if(!inf) return -1;
	rsize=fread(head,1,128,inf);
	fclose(inf);
	for(i=0;i<rsize-4;i++){
		if(!memcmp(head+i,"fLaC",4)) break;
	}
	i += 4;
	if(rsize-i<NEEDED_LEN){
		INFMSG("looks not flac file\n");
		return -1;
	}

	/* let's mask the first bit to get the metablock type: needs to
	 * be streaminfo (0) */
	memcpy(&tmp, head+i, 1);
	if ((tmp & 0x40) != 0) {
		INFMSG("it's not a streaminfo metablock!\n");
		return -1;
	}
	i += 14;
	
	/* now the first 20 bits is the sample rate: flac encodes in
	 * big-endian so we might need to translate */
	memcpy(&j, head+i, 4);
	j = ntohl(j);
	j = j >> 12;
	j = j & 0xFFFFF;
	if (j < 0) {
		INFMSG("trouble in getting the sample rate!\n");
		return -1;
	}
	auds->sample_rate=j;
	i += 2;

	/* and the next 3 bit is the # of channels */
	memcpy(&tmp, head+i, 1);
	tmp &= 0xE;
	tmp = tmp >> 1;
	DBGMSG("number of channels=%d\n", tmp + 1);
	auds->channels=tmp + 1;

	/* # encoding bit are in the next 5 bit */
	memcpy(&j, head+i, 1);
	j &= 0x1;
	j = j << 5;
	memcpy(&tmp, head+i+1, 1);
	tmp &= 0xF0;
	tmp = tmp >> 4;
	tmp = j | tmp;
	DBGMSG("%d bit\n", tmp + 1);

	return 0;
}

int flac_open(auds_t *auds, char *fname)
{
	flac_t *flac=malloc(sizeof(flac_t));
	if(!flac) return -1;
	memset(flac,0,sizeof(flac_t));
	auds->stream=(void*)flac;
	flac->fname=(char *)malloc(strlen(fname)+1);
	if(!flac->fname) goto erexit;
	strcpy(flac->fname,fname);
	flac->buffer=(u_int8_t *)malloc(MAX_SAMPLES_IN_CHUNK*4+16);
	if(!flac->buffer) goto erexit;
	auds->sigchld_cb=sigchld_callback;
	if(access(FIFO_NAME,F_OK) && mkfifo(FIFO_NAME,0600)<0){
		ERRMSG("can't make a named fifo:%s\n",FIFO_NAME);
		goto erexit;
	}
	auds->sample_rate=DEFAULT_SAMPLE_RATE;
	read_header(auds, fname);
	if(run_decoder(flac)) goto erexit;
	auds->chunk_size=aud_clac_chunk_size(auds->sample_rate);
	return 0;
 erexit:
	flac_close(auds);
	return -1;
}

int flac_close(auds_t *auds)
{
	flac_t *flac=(flac_t *)auds->stream;
	if(!flac) return -1;
	if(flac->inf) fclose(flac->inf);
	stop_decoder(flac);
	unlink(FIFO_NAME);
	if(flac->buffer) free(flac->buffer);
	if(flac->fname) free(flac->fname);
	free(flac);
	auds->stream=NULL;
	return 0;
}

int flac_get_top_sample(auds_t *auds, u_int8_t **data, int *size)
{
	flac_t *flac=(flac_t *)auds->stream;
	stop_decoder(flac);
	run_decoder(flac);
	return flac_get_next_sample(auds, data, size);
}

int flac_get_next_sample(auds_t *auds, u_int8_t **data, int *size)
{
	flac_t *flac=(flac_t *)auds->stream;
	data_source_t ds={.type=STREAM, .u.inf=flac->inf};
	return auds_write_pcm(auds, flac->buffer, data, size, auds->chunk_size, &ds);
}

