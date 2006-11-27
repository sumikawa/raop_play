/*****************************************************************************
 * pls_stream.c: pls file stream
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
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#define PLS_STREAM_C
#include "audio_stream.h"
#include "pls_stream.h"
#include "aexcl_lib.h"



static int str_termlf(char *line)
{
	int i;
	for(i=0;i<strlen(line);i++){
		if(line[i]=='\n'){
			line[i]=0;
			return i;
		}
	}
	return 0;
}

static int pls_next_open(auds_t *auds, pls_t *pls)
{
	size_t len=1024;
	char *line=(char *)malloc(len);
	int lsize,lp,i, findex;

	if(auds->auds) auds_close(auds->auds);
	auds->auds=NULL;
	if(!line) return -1;
	pls->index++;
	while((lsize=getline(&line,&len,pls->inf))>0){
		if(strstr(line,"File")!=line) continue;
		str_termlf(line);
		for(i=4;i<lsize;i++){
			if(line[i]!='=') continue;
			lp=i+1;
			line[i]=0;
			findex=atoi(line+4);
			if(pls->index!=findex) break;
			if((auds->auds=auds_open(line+lp, 0))) return 0;
			return -1;
		}
	}
	return -1;
}
		

int pls_open(auds_t *auds, char *fname)
{
	pls_t *pls=malloc(sizeof(pls_t));
		
	if(!pls) return -1;
	memset(pls,0,sizeof(pls_t));
	auds->stream=(void*)pls;
	pls->fname=(char *)malloc(strlen(fname)+1);
	if(!pls->fname) goto erexit;
	strcpy(pls->fname,fname);
	if(!(pls->inf=fopen(pls->fname,"r"))) goto erexit;
	pls->index=0;
	return pls_next_open(auds, pls);
 erexit:
	pls_close(auds);
	return -1;
}

int pls_close(auds_t *auds)
{
	pls_t *pls=(pls_t *)auds->stream;
	if(auds->auds) auds_close(auds->auds);
	if(!pls) return -1;
	if(pls->inf) fclose(pls->inf);
	if(pls->fname) free(pls->fname);
	free(pls);
	auds->stream=NULL;
	return 0;
}

int pls_top_song(auds_t *auds)
{
	pls_t *pls=(pls_t *)auds->stream;
	pls->index=0;
	fseek(pls->inf, 0, SEEK_SET);
	return pls_next_open(auds, pls);
}

int pls_next_song(auds_t *auds)
{
	pls_t *pls=(pls_t *)auds->stream;
	return pls_next_open(auds, pls);
}
