/*****************************************************************************
 * mp3_stream.c: mp3 file stream
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
#include <sys/wait.h>
#include <fcntl.h>
#include <id3tag.h>
#define MP3_STREAM_C
#include "audio_stream.h"
#include "mp3_stream.h"
#include "aexcl_lib.h"

#define MP3_DECODER "mpg321"
static int run_decoder(auds_t *auds, mp3_t *mp3)
{
	char *darg[4]={MP3_DECODER,"-s",NULL, NULL};
	int efd;
	char buf[1024];
	char *bufp;
	int rsize,tsize=0,i;
	int rval=-1;

	darg[2]=mp3->fname;
	if(auds->data_type==AUD_TYPE_MP3){
		mp3->dpid=child_start(darg,&mp3->dfd,NULL,NULL);
		return 0;
	}
	/* for AUD_TYPE_URLMP3, get sample rate from the stderr
	   message of the decoder. This way may work for all and I don't need read_header. */
	mp3->dpid=child_start(darg,&mp3->dfd,NULL,&efd);
	i=0;
	while(1){
		// need some time to start getting data from http://..., 3 sec should be enough
		if(i==30 || tsize>=1023){
			INFMSG("can't get sample rate from %s\n",mp3->fname);
			goto erexit;
		}
		rsize=read(efd,buf+tsize,1023-tsize);
		if(rsize<=0) goto erexit;
		tsize+=rsize;
		buf[tsize]=0;
		if((bufp=strstr(buf,"Hz"))) break;
		usleep(100000); // 100msec
		i++;
	}
	if(!memcmp(bufp+3,"mono",4)){
		auds->channels=1;
	}
	bufp--;
	while(bufp-- > buf)
		if(*(bufp)>'9' || *(bufp)<'0') break;
	auds->sample_rate=atoi(++bufp);
	DBGMSG("sample rate=%d, channels=%d\n",auds->sample_rate, auds->channels);
	rval=0;
 erexit:
	close(efd);
	return rval;
}

static int stop_decoder(mp3_t *mp3)
{
	int i;

	if(!mp3->dpid) return 0;
	kill(mp3->dpid,SIGTERM);
	for(i=0;i<10;i++){
		if(!mp3->dpid) return 0;
		usleep(10*1000);
	}
	ERRMSG("decoder process can't be terminated\n");
	return 0;
}

static void sigchld_callback(void *p, siginfo_t *siginfo)
{
	mp3_t *mp3=(mp3_t*)p;
	if(siginfo->si_pid==mp3->dpid){
		waitpid(mp3->dpid,NULL,0);
		mp3->dpid=0;
	}
}

static int read_header(auds_t *auds, char *fname)
{
	FILE *inf;
	__u8 head[4];
	int ver, srate;
	int rnum[3][3]={{44100,48000,32000},
		      {22050,24000,16000},
		      {11025,12000,8000},
	};
	id3_length_t headerStart = 0;
	struct id3_file *id3file;
	struct id3_tag *id3tag;

	/* get id3tag size */
	id3file = id3_file_open(fname,ID3_FILE_MODE_READONLY);
	if (id3file) {
		id3tag = id3_file_tag(id3file);
		if (id3tag) {
			headerStart = id3tag->paddedsize;
			DBGMSG("id3 tagsize: %d\n", (int)id3tag->paddedsize);
		}
		id3_file_close(id3file);
	}else{
		INFMSG("id3_file_open can't open file, go ahead anyway\n");
	}

	inf=fopen(fname, "r");
	if(!inf) return -1;

	fseek(inf,headerStart,SEEK_SET);
	fread(head,1,4,inf);
	fclose(inf);

	ver=(head[1]>>3)&0x3;
	srate=(head[2]>>2)&0x3;
	if(ver==1 || srate==3){
		INFMSG("mp3 frame header can't be read, go with default sample rate\n");
		return -1;
	}
	switch(ver){
	case 0:
		// 2.5
		auds->sample_rate=rnum[2][srate];
		break;
	case 1:
		// reserved
		break;
	case 2:
		// 2
		auds->sample_rate=rnum[1][srate];
		break;
	case 3:
		// 1
		auds->sample_rate=rnum[0][srate];
		break;
	}
	DBGMSG("sample rate=%d\n",auds->sample_rate);
	return 0;
}


int mp3_open(auds_t *auds, char *fname)
{
	mp3_t *mp3=malloc(sizeof(mp3_t));
	if(!mp3) return -1;
	memset(mp3,0,sizeof(mp3_t));
	auds->stream=(void*)mp3;
	mp3->fname=(char *)malloc(strlen(fname)+1);
	if(!mp3->fname) goto erexit;
	strcpy(mp3->fname,fname);
	mp3->buffer=(__u8 *)malloc(MAX_SAMPLES_IN_CHUNK*4+16);
	if(!mp3->buffer) goto erexit;
	auds->sigchld_cb=sigchld_callback;
	auds->sample_rate=DEFAULT_SAMPLE_RATE;
	if(auds->data_type==AUD_TYPE_MP3)
		read_header(auds, fname);
	run_decoder(auds, mp3);
	auds->chunk_size=aud_clac_chunk_size(auds->sample_rate);
	return 0;
 erexit:
	mp3_close(auds);
	return -1;
}

int mp3_close(auds_t *auds)
{
	mp3_t *mp3=(mp3_t *)auds->stream;
	if(!mp3) return -1;
	if(mp3->dfd>=0) close(mp3->dfd);
	stop_decoder(mp3);
	if(mp3->buffer) free(mp3->buffer);
	if(mp3->fname) free(mp3->fname);
	free(mp3);
	auds->stream=NULL;
	return 0;
}

int mp3_get_top_sample(auds_t *auds, __u8 **data, int *size)
{
	mp3_t *mp3=(mp3_t *)auds->stream;
	stop_decoder(mp3);
	run_decoder(auds, mp3);
	return mp3_get_next_sample(auds, data, size);
}

int mp3_get_next_sample(auds_t *auds, __u8 **data, int *size)
{
	mp3_t *mp3=(mp3_t *)auds->stream;
	data_source_t ds={.type=DESCRIPTOR, .u.fd=mp3->dfd};
	return auds_write_pcm(auds, mp3->buffer, data, size, auds->chunk_size, &ds);
}

