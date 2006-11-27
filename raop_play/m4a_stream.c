/*****************************************************************************
 * m4a_stream.c: .m4a file stream
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <asm/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "aexcl_lib.h"
#include "audio_stream.h"
#include "m4a_stream.h"

#define BUF_SIZE (8*1024)
#define CROSS_RECOVER_SIZE 4

#define AtomALAC "alac"
#define AtomSTSZ "stsz"
#define AtomSTCO "stco"
#define AtomCO64 "co64"
#define AtomSTSC "stsc"

typedef struct m4asd_t {
	__u8 buf[BUF_SIZE+CROSS_RECOVER_SIZE];
	int bufp;
	int vdata_size;
	__u8 *sst; // sample size table
	__u8 *cot; // chunk offset table
	__u8 *cst; // chunk samples table
	int nsst; // number of data in sst
	int ncot; // number of data in cot
	int ncst; // number of data in cst
	int cur_chunk; // current chunk number
	int cur_sp; // current sample number
	int cur_sp_in_chunk; // current sample number in chunk
	int cur_cst_index; // current chunk size table index
	int cur_pos; // current file position
	int co_size; // chunk offset data size, 4:32 bits, 8:64 bits
	__u8 *read_data;
	int read_data_size;
	FILE *inf;
}m4asd_t;

static inline int big_end_int(__u8 *p)
{
	return (*p<<24) | (*(p+1)<<16) | (*(p+2)<<8) | *(p+3);
}
	
static int read_buf(m4asd_t *m4asd, int cover_copy)
{
	int size;
	if(cover_copy){
		memcpy(m4asd->buf, m4asd->buf+BUF_SIZE, CROSS_RECOVER_SIZE);
		m4asd->bufp=0;
	}else{
		m4asd->bufp=CROSS_RECOVER_SIZE;
	}
	size=fread(m4asd->buf+CROSS_RECOVER_SIZE,1,BUF_SIZE,m4asd->inf);
	if(size==-1){
		ERRMSG("%s: read error: %s\n",__func__,strerror(errno));
		return -1;
	}
	m4asd->vdata_size=size+CROSS_RECOVER_SIZE;
	return size;
}

static int find_atom_pos(m4asd_t *m4asd, char *atom)
{
	while(1){
		if(m4asd->vdata_size-m4asd->bufp<4) {
			if(read_buf(m4asd, 1)<=0) {
				ERRMSG("%s: %s can't be found in the file\n",__func__,atom);
				return -1;
			}
		}
		while(m4asd->bufp<m4asd->vdata_size-3){
			if(m4asd->buf[m4asd->bufp]==atom[0] &&
			   m4asd->buf[m4asd->bufp+1]==atom[1] &&
			   m4asd->buf[m4asd->bufp+2]==atom[2] &&
			   m4asd->buf[m4asd->bufp+3]==atom[3] ) {
				return 0;
			}
			m4asd->bufp++;
		}
	}
	return -1;
}

static int get_atom_data(m4asd_t *m4asd, __u8 **data)
{
	int dsize, size,cpsize;
	__u8 *dp;
	size=big_end_int(m4asd->buf+m4asd->bufp-4);
	dsize=size;
	if(size<8) return -1;
	size-=8;
	if(realloc_memory((void**)data, size, __func__)) return -1;
	dp=*data;
	m4asd->bufp+=4;
	while(size>0){
		if(m4asd->vdata_size-m4asd->bufp <= 0){
			if(read_buf(m4asd, 0)<=0) {
				ERRMSG("%s: reached bottom of the file\n",__func__);
				return -1;
			}
		}
		cpsize=(size < m4asd->vdata_size-m4asd->bufp)?size:m4asd->vdata_size-m4asd->bufp;
		memcpy(dp,m4asd->buf+m4asd->bufp,cpsize);
		size-=cpsize;
		dp+=cpsize;
		m4asd->bufp+=cpsize;
	}
	return dsize;
}
	
/*
 * return sample size table data in *data which is allocated in this function
 * number of data at data+8 (big endian)
 * sample size at data+12+4*i (big endian)
 * return value : number of data, -1 if error
 */
static int get_sample_size_table(m4asd_t *m4asd, __u8 **data)
{
	int rval=-1;
	int size;
	if(find_atom_pos(m4asd, AtomSTSZ)) return -1;
	if((size=get_atom_data(m4asd, data))<0) goto erexit;
	return big_end_int(*data+8);
 erexit:
	if(*data) free(*data);
	*data=NULL;
	return rval;
}

/*
 * return chunk offset table data in *data which is allocated in this function
 * number of data at data+4 (big endian)
 * chunk offset at data+8+4*i (big endian)
 *              at data+8+8*i if *co64==1 (32 bits big endian)
 * return value : number of data, -1 if error
 */
static int get_chunk_offset_table(m4asd_t *m4asd, __u8 **data)
{
	int rval=-1;
	int size;
	
	m4asd->co_size=4;
	if(find_atom_pos(m4asd, AtomSTCO)){
		if(find_atom_pos(m4asd, AtomCO64)) goto erexit;
		m4asd->co_size=8;
	}
	if((size=get_atom_data(m4asd, data))<0) goto erexit;
	return big_end_int(*data+4);
 erexit:
	if(*data) free(*data);
	*data=NULL;
	return rval;
}


/*
 * return chunk samples table data in *data which is allocated in this function
 * number of data at data+4 (big endian)
 * first chunk number table: data+8+(i*12), (big endian)
 * "number of sample in a chunk" table: data+12+(i*12), (big endian)
 * return value : number of data, -1 if error
 */
static int get_chunk_samples_table(m4asd_t *m4asd, __u8 **data)
{
	int rval=-1;
	int size;
	if(find_atom_pos(m4asd, AtomSTSC)) return -1;
	if((size=get_atom_data(m4asd, data))<0) goto erexit;
	return big_end_int(*data+4);
 erexit:
	if(*data) free(*data);
	*data=NULL;
	return rval;
}

static int get_next_pos(m4asd_t *m4asd, int *pos, int *size)
{
	int sinc;
	int next_first_chunk;
	__u8 *co;
	
	if(m4asd->cur_chunk>=m4asd->ncot) return -1;
	if(!m4asd->cur_pos){
		if(m4asd->co_size==4){
			m4asd->cur_pos=big_end_int(m4asd->cot+8);
		}else{
			// ignore upper 4 bytes
			m4asd->cur_pos=big_end_int(m4asd->cot+8+4);
		}
	}
	*pos=m4asd->cur_pos;
	*size=big_end_int(m4asd->sst+12+4*m4asd->cur_sp);
	sinc=big_end_int(m4asd->cst+12+m4asd->cur_cst_index*12);

	m4asd->cur_pos+=*size;
	m4asd->cur_sp_in_chunk++;
	m4asd->cur_sp++;
	if(m4asd->cur_sp_in_chunk>=sinc){
		// the sample reached to the end of this chunk, increment it
		m4asd->cur_chunk++;
		m4asd->cur_sp_in_chunk=0;
		if(m4asd->cur_chunk>=m4asd->ncot) return 0; // this will fail at the next time
		if(m4asd->cur_cst_index<m4asd->ncst-1){
			// check if sample per chunk data need to be updated
			next_first_chunk=big_end_int(m4asd->cst+8+(m4asd->cur_cst_index+1)*12);
			if(m4asd->cur_chunk>=next_first_chunk)
				m4asd->cur_cst_index++;
		}

		co=m4asd->cot+8+m4asd->co_size*m4asd->cur_chunk;
		if(m4asd->co_size==4){
			m4asd->cur_pos=big_end_int(co);
		}else{
			// ignore upper 4 bytes
			m4asd->cur_pos=big_end_int(co+4);
		}
		//DBGMSG("%s:chunk is updated next pos=0x%x\n",__func__,m4asd->cur_pos);
	}
	return 0;
}


int m4a_open(auds_t *auds, char *fname)
{
	m4asd_t *m4asd;

	m4asd=(m4asd_t *)malloc(sizeof(m4asd_t));
	if(!m4asd) return -1;
	memset(m4asd, 0, sizeof(m4asd_t));
	auds->stream=(void *)m4asd;

	m4asd->inf=fopen(fname,"r");
	if(!m4asd->inf){
		ERRMSG("%s can't open %s\n",__func__,fname);
		goto erexit;
	}

	if(read_buf(m4asd, 0)<0) goto erexit;
	if(find_atom_pos(m4asd, AtomALAC)) goto erexit;
	if((m4asd->ncst=get_chunk_samples_table(m4asd,&m4asd->cst))==-1) goto erexit;
	if((m4asd->nsst=get_sample_size_table(m4asd,&m4asd->sst))==-1) goto erexit;
	if((m4asd->ncot=get_chunk_offset_table(m4asd,&m4asd->cot))==-1) goto erexit;
	return 0;

 erexit:
	m4a_close(auds);
	return -1;
}

int m4a_close(auds_t *auds)
{
	m4asd_t *m4asd=(m4asd_t *)auds->stream;
	if(!m4asd) return -1;
	if(m4asd->inf) fclose(m4asd->inf);
	if(m4asd->sst) free(m4asd->sst);
	if(m4asd->cot) free(m4asd->cot);
	if(m4asd->cst) free(m4asd->cst);
	if(m4asd->read_data) free(m4asd->read_data);
	free(m4asd);
	auds->stream=NULL;
	return 0;
}


int m4a_get_top_sample(auds_t *auds, __u8 **data, int *size)
{
	m4asd_t *m4asd=(m4asd_t *)auds->stream;
	
	if(!m4asd) return -1;
	m4asd->cur_chunk=0;
	m4asd->cur_sp=0;
	m4asd->cur_sp_in_chunk=0;
	m4asd->cur_pos=0;
	return m4a_get_next_sample(auds, data, size);
}

int m4a_get_next_sample(auds_t *auds, __u8 **data, int *size)
{
	m4asd_t *m4asd=(m4asd_t *)auds->stream;
	int pos;
	int rsize;
	
	if(!m4asd) return -1;
	if(get_next_pos(m4asd, &pos, size)) return -1;
	//DBGMSG("%s:next pos=0x%x, size=0x%x(%d)\n",__func__,pos,*size,*size);
	if(*size<=0) return -1;
	if(*size>0x80000) return -1; // shouldn't be this big
	fseek(m4asd->inf,pos,SEEK_SET);
	if(m4asd->read_data_size<*size) {
		if(realloc_memory((void**)&m4asd->read_data,*size,__func__)) return -1;
		m4asd->read_data_size=*size;
	}
	rsize=fread(m4asd->read_data,1,*size,m4asd->inf);
	if(*size!=rsize){
		ERRMSG("%s:can't read the size of data, indicated size=%d, read size=%d\n",
		       __func__,*size,rsize);
		return -1;
	}
	*data=m4asd->read_data;
	return 0;
}




