/*****************************************************************************
 * aac_stream.c: aac file stream
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
#include <signal.h>
#include <sys/wait.h>
#define AAC_STREAM_C
#include "audio_stream.h"
#include "aac_stream.h"
#include "aexcl_lib.h"


#define AAC_DECODER "faad"
#define FIFO_NAME "/tmp/aac_stream"
static int run_decoder(aac_t *aac)
{
	char *darg[8]={AAC_DECODER,"-f","2","-o", NULL, NULL, NULL};
	darg[4]=FIFO_NAME;
	darg[5]=aac->fname;
	aac->dpid=child_start(darg,NULL,NULL,NULL);
	if(!(aac->inf=fopen(FIFO_NAME,"r"))) {
		ERRMSG("can't open fifo\n");
		return -1;
	}
	return 0;
}

static int stop_decoder(aac_t *aac)
{
	int i;

	if(!aac->dpid) return 0;
	kill(aac->dpid,SIGTERM);
	for(i=0;i<10;i++){
		if(!aac->dpid) return 0;
		usleep(10*1000);
	}
	ERRMSG("decoder process can't be terminated\n");
	return 0;
}

static void sigchld_callback(void *p, siginfo_t *siginfo)
{
	aac_t *aac=(aac_t*)p;
	if(siginfo->si_pid==aac->dpid){
		waitpid(aac->dpid,NULL,0);
		aac->dpid=0;
	}
}

static int read_header(auds_t *auds, char *fname)
{
	// Seems not easy to read aac or mp4 header, go easy way here
	char cmdstr[1024];
	snprintf(cmdstr, sizeof(cmdstr), "%s -i %s 2>&1 | sed -n 's/.* \\([0-9]*\\) Hz.*/\\1/p'",
		AAC_DECODER, fname);
	FILE *inf=popen(cmdstr,"r");
	char data[16];
	int srate;
	memset(data,0,16);
	fread(data,1,16,inf);
	if((srate=atoi(data))>=8000){
		auds->sample_rate=srate;
		DBGMSG("sample rate=%d\n",srate);
	}
	pclose(inf);
	return 0;
}

int aac_open(auds_t *auds, char *fname)
{
	aac_t *aac=malloc(sizeof(aac_t));
	if(!aac) return -1;
	memset(aac,0,sizeof(aac_t));
	auds->stream=(aac_t*)aac;
	aac->fname=(char *)malloc(strlen(fname)+1);
	if(!aac->fname) goto erexit;
	strcpy(aac->fname,fname);
	aac->buffer=(__u8 *)malloc(MAX_SAMPLES_IN_CHUNK*4+16);
	if(!aac->buffer) goto erexit;
	auds->sigchld_cb=sigchld_callback;
	auds->sample_rate=DEFAULT_SAMPLE_RATE;
	read_header(auds, fname);
	if(access(FIFO_NAME,F_OK) && mkfifo(FIFO_NAME,0600)<0){
		ERRMSG("can't make a named fifo:%s\n",FIFO_NAME);
		goto erexit;
	}
	run_decoder(aac);
	auds->chunk_size=aud_clac_chunk_size(auds->sample_rate);
	return 0;
 erexit:
	aac_close(auds);
	return -1;
}

int aac_close(auds_t *auds)
{
	aac_t *aac=(aac_t *)auds->stream;
	if(!aac) return -1;
	if(aac->inf) fclose(aac->inf);
	stop_decoder(aac);
	unlink(FIFO_NAME);
	if(aac->buffer) free(aac->buffer);
	if(aac->fname) free(aac->fname);
	free(aac);
	return 0;
}

int aac_get_top_sample(auds_t *auds, __u8 **data, int *size)
{
	aac_t *aac=(aac_t *)auds->stream;
	stop_decoder(aac);
	run_decoder(aac);
	return aac_get_next_sample(auds, data, size);
}


int aac_get_next_sample(auds_t *auds, __u8 **data, int *size)
{
	aac_t *aac=(aac_t *)auds->stream;
	data_source_t ds={.type=STREAM, .u.inf=aac->inf};
	return auds_write_pcm(auds, aac->buffer, data, size, auds->chunk_size, &ds);
}

