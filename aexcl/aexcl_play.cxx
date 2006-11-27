/*****************************************************************************
 * aexcl_play.cxx : Apple Airport Express Client Player
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
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <getopt.h>
#include "aexcl_gui.h"
#include "aexcl_lib.h"
#include "mDNS.h"
#include "raop_play.h"

#define MDNS_PROGRAM "mDNSClient"
#define RAOP_PROT_NAME "_raop._tcp"
#define IP_NAME_DELIM "-"

typedef enum aex_sate_t {
	STATE_DISCON=0,
	STATE_STOP,
	STATE_PLAY,
	STATE_PAUSE,
}aex_sate_t;

typedef struct aexd_t {
	AexclGui *agui;
	aex_sate_t state;
	char **file_list;
	int file_index;
	int raop_outfd;
	int raop_infd;
	int raop_pid;
} aexd_t;
static aexd_t aexd;

static int aex_set_volume(int v=-1);
static int status_change(char *ns, char *ms=NULL);

static void chcb(Fl_Widget *o, void *p)
{
	Fl_Choice *ch=(Fl_Choice*)o;
	DBGMSG("%s: menue item %d is chosen\n",__func__, ch->value());
}

static void mdns_update()
{
	FILE *inf;
	char *sline1,*sline2, *name;
	size_t linelen1=128;
	size_t linelen2=128;
	char filename[128];
	int isize;

	//DBGMSG("%s\n",__func__);
	sprintf(filename,"%s%s", MDNS_RECORD_PRE, RAOP_PROT_NAME);
	inf=fopen(filename,"r");
	if(!inf){
		ERRMSG("%s:can't open mDNS record file %s\n", __func__,filename);
		return;
	}
	isize=aexd.agui->aex_chooser->size();
	sline1=(char*)malloc(linelen1);
	sline2=(char*)malloc(linelen2);
	while(1){
		if(getline(&sline1, &linelen1, inf)<=0) break;
		if(lf_to_null(sline1,linelen1)<=0) break;
		if(getline(&sline2, &linelen2, inf)<=0) break;
		if(lf_to_null(sline2,linelen2)<=0) break;
		if((name=strstr(sline1,"@")))
			name=name+1;
		else
			name=sline1;
		sline2=(char*)realloc(sline2,strlen(sline1)+strlen(name)+1+strlen(IP_NAME_DELIM));
		strcat(sline2,IP_NAME_DELIM);
		strcat(sline2,name);
		DBGMSG("%s:%s\n",__func__,sline2);
		aexd.agui->aex_chooser->add(sline2,"",chcb);
	}
	if(!isize && aexd.agui->aex_chooser->size())
		aexd.agui->aex_chooser->value(0);
	free(sline1);
	free(sline2);
}

static void mdns_event(int fd, void *p)
{
	char line[80];

	DBGMSG("%s\n",__func__);
	if(read_line(fd, line, sizeof(line),0,1)<0){
		aexd.agui->main_window->~Fl_Window();
		return;
	}
	if(!strcmp(line,"updated")) mdns_update();
}

static int cleanup_filelist(void)
{
	int i=0;
	if(!aexd.file_list) return 0;
	while(aexd.file_list[i])
		free(aexd.file_list[i++]);
	free(aexd.file_list);
	aexd.file_list=NULL;
	return 0;
}

static int aex_stop(void)
{
	if(aexd.state==STATE_DISCON) return 0;
	if(aexd.state==STATE_STOP) return 0;
	if(aexd.raop_outfd>=0){
		aex_set_volume(0);
		write(aexd.raop_outfd,"pause\n", 6);
		write(aexd.raop_outfd,"stop\n", 5);
		aexd.agui->status->value("not playing");
	}
	aexd.state=STATE_STOP;
	return 0;
}

static int play_next_song(void)
{
	char *cdir;
	int ipod=aexd.agui->ipod_mode->value();
	if(aexd.file_list && aexd.file_list[aexd.file_index] &&
		aexd.raop_outfd>=0){
		write(aexd.raop_outfd,"play ", 5);
		if(!ipod){
			cdir=aexd.agui->get_dirname();
			write(aexd.raop_outfd,cdir, strlen(cdir));
			write(aexd.raop_outfd,"/", 1);
		}
		write(aexd.raop_outfd,aexd.file_list[aexd.file_index],
		      strlen(aexd.file_list[aexd.file_index]));
		write(aexd.raop_outfd,"\n", 1);
		status_change("playing", aexd.file_list[aexd.file_index]);
		aexd.file_index++;
		if(!aexd.file_list[aexd.file_index] && aexd.agui->repeat->value())
			aexd.file_index=0;
		return 0;
	}else{
		return -1;
	}
	
}

static void raop_event(int fd, void *p)
{
	char line[80];
	
	if(read_line(fd, line, sizeof(line),0,1)<0){
		return;
	}
	DBGMSG("%s:%s\n",__func__,line);

	if(!strcmp(line,RAOP_CONNECTED)) {
		aexd.state=STATE_STOP;
		if(play_next_song()) goto erexit;
		aexd.state=STATE_PLAY;
		return;
	}
	if(!strcmp(line,RAOP_SONGDONE)){
		if((aexd.state==STATE_PLAY) && play_next_song()) goto erexit;
		return;
	}
	if(!strcmp(line,RAOP_ERROR)){
		DBGMSG("%s: raop_play had an error, go back to STOP state\n",__func__);
		goto erexit;
	}
	ERRMSG("%s:unknown message from raop_play\n",__func__);
	return;   
 erexit:
	aexd.agui->pause->hide();
	aexd.agui->play->show();
	aexd.agui->status->value("not playing");
	aexd.state=STATE_STOP;
	return;
}

static int run_raop_play(void)
{
	const char *rargv[6];
	char ip[32];
	char *astr;
	char vol[8];

	rargv[0]="raop_play";
	if(!aexd.agui->aex_chooser->size()) return -1;
	if(aexd.agui->aex_chooser->value()<0) return -1;
	rargv[1]=aexd.agui->aex_chooser->text(aexd.agui->aex_chooser->value());
	if(!(astr=strstr(rargv[1],IP_NAME_DELIM))) return -1;
	memset(ip,0,sizeof(ip));
	memcpy(ip,rargv[1],((__u32)(astr-rargv[1])<sizeof(ip))?astr-rargv[1]:sizeof(ip));
	rargv[1]=ip;
	rargv[2]="-i"; // interactive mode
	rargv[3]="--vol";
	sprintf(vol,"%d",(int)aexd.agui->volume->value());
	rargv[4]=vol;
	rargv[5]=NULL;
	aexd.raop_pid=child_start((char* const*)rargv, &aexd.raop_infd, &aexd.raop_outfd, NULL);
	return 0;
}

static int aex_nextplay(void)
{
	if(aexd.state==STATE_DISCON){
		run_raop_play();
		if(aexd.raop_infd>=0){
			Fl::add_fd(aexd.raop_infd, FL_READ, raop_event, NULL);
			return 0;
		}
	}else{
		if(!play_next_song()){
			aex_set_volume();
			aexd.state=STATE_PLAY;
			return 0;
		}
	}
	DBGMSG("%s:failed\n",__func__);
	return -1;
}

#define MAX_IPOD_DIR_DEPTH 10 // 10 should be more than enough
static char *ipodpath_to_syspath(const char *path)
{
	int i;
	char *np, *cp, *res;
	const char *components[MAX_IPOD_DIR_DEPTH];
	char *mp=aexd.agui->ipod_browser->get_mount_point();
	if(!mp) return NULL;
	np=strdup(path);
	cp=np;
	for(i=0;i<MAX_IPOD_DIR_DEPTH;i++){
		cp=strchr(cp,':');
		if(!cp){
			components[i]=NULL;
			break;
		}
		*cp=0;
		cp++;
		components[i]=cp;
	}
	res=resolve_path(mp,components);
	free(np);
	return res;
}

static int aex_newplay(void)
{
	int i,n;
	int count=0;
	const char *item;
	Fl_Browser *browser;
	int ipod;
	
	cleanup_filelist();
	//if(aexd.raop_pid) return -1; // should be stopped at this time
	ipod=aexd.agui->ipod_mode->value();
	browser=(ipod)?(Fl_Browser *)aexd.agui->ipod_browser:
	  (Fl_Browser *)aexd.agui->file_browser;
	count=(ipod)?aexd.agui->ipod_browser->num_of_selected():
	  aexd.agui->file_browser->num_of_selected();
	if(count<=0) return -1;
	if(!(aexd.file_list=(char **)malloc(sizeof(char*)*(count+1)))) return -1;
	memset(aexd.file_list,0,sizeof(char*)*(count+1));
	if((n=(ipod)?aexd.agui->ipod_browser->first_selected():
	    aexd.agui->file_browser->first_selected())<0) return -1;
	DBGMSG("selected num = %d\n",count);
	for(i=0;i<count;i++){
		if(ipod){
			if(!(item=(char*)aexd.agui->ipod_browser->data(n+1))) return -1;
			aexd.file_list[i]=ipodpath_to_syspath(item);
		}else{
			if(!(item=aexd.agui->file_browser->text(n+1))) return -1;
			aexd.file_list[i]=strdup(item);
		}
		if(!aexd.file_list[i]) return -1;
		DBGMSG("selected item = %s\n",aexd.file_list[i]);
		n=(ipod)?aexd.agui->ipod_browser->next_selected():
		  aexd.agui->file_browser->next_selected();
		if(n<0) break;
	}
	aexd.file_index=0;
	return aex_nextplay();
}
	
static int aex_contplay(void)
{
	if(aexd.state!=STATE_PAUSE) return 0;
	if(aexd.raop_outfd>=0){
		aexd.state=STATE_PLAY;
		write(aexd.raop_outfd,"play\n",5);
		status_change("playing");
	}
	return 0;
}

static int status_change(char *ns, char *ms)
{
	const char *astr;
	char bstr[128];
	sprintf(bstr,"%s ",ns);
	if(ms){
		strcat(bstr,": ");
		strncat(bstr,ms,sizeof(bstr));
		aexd.agui->status->value(bstr);
		return 0;
	}
	astr=aexd.agui->status->value();
	if(astr && (astr=strstr(astr,":"))){
		strncat(bstr,astr,sizeof(bstr));
		aexd.agui->status->value(bstr);
	}
	return 0;
}

static int aex_pause(void)
{
	if(aexd.state!=STATE_PLAY) return 0;
	if(aexd.raop_outfd>=0){
		aexd.state=STATE_PAUSE;
		write(aexd.raop_outfd,"pause\n",6);
		status_change("paused");
	}
	return 0;
}

static int aex_set_volume(int v)
{
	char vol[32];
	if(v==-1)
		sprintf(vol,"volume %d\n",(int)aexd.agui->volume->value());
	else
		sprintf(vol,"volume %d\n",v);
	write(aexd.raop_outfd,vol,strlen(vol));
	return 0;
}	

/*
 * FWD and REW are not supported, use NEXT_TRACK and PREV_TRACK instead
 * if this returns non-zero, aexd.agui keeps the same status (about pause, play buttons)
 */
static int aex_cntl(aex_cntl_enum_t act, int value)
{
	switch(act){
	case AEX_STOP:
		return aex_stop();
	case AEX_PLAY:
		if(aexd.state==STATE_STOP||aexd.state==STATE_DISCON)
			return aex_newplay();
		else
			return aex_contplay();
		break;
	case AEX_PAUSE:
		return aex_pause();
	case AEX_NEXT_TRACK:
		aex_stop();
		return aex_nextplay();
	case AEX_PREV_TRACK:
		aex_stop();
		aexd.file_index-=2;
		if(aexd.file_index>=0) return aex_nextplay();
		if(aexd.agui->repeat->value())
			aexd.file_index=aexd.agui->file_browser->num_of_selected()+aexd.file_index;
		if(aexd.file_index<0) aexd.file_index=0;
		return aex_nextplay();
	case AEX_FWD:
	case AEX_REW:
		break;
	case AEX_VOL:
		if(aexd.state==STATE_DISCON || aexd.raop_outfd<0) return 0;
		return aex_set_volume();
	}
	return 0;
}

static void sig_action(int signo, siginfo_t *siginfo, void *extra)
{
	if(signo==SIGTERM || signo==SIGINT){
		DBGMSG("received SIGTERM, going to terminate myself\n");
		aexd.agui->main_window->~Fl_Window();
		return;
	}
	// SIGCHLD
	if(signo==SIGCHLD){
		DBGMSG("received SIGCHLD, pid=%d\n",aexd.raop_pid);
		if(siginfo->si_pid==aexd.raop_pid){
			waitpid(aexd.raop_pid,NULL,0);
			aexd.state=STATE_DISCON;
			aexd.raop_pid=0;
			aexd.agui->pause->hide();
			aexd.agui->play->show();
			if(aexd.raop_outfd>=0) close(aexd.raop_outfd);
			aexd.raop_outfd=-1;
			if(aexd.raop_infd>=0) {
				Fl::remove_fd(aexd.raop_infd);
				close(aexd.raop_infd);
			}
			aexd.raop_infd=-1;
			aexd.agui->status->value("disconnected");
		}
		return;
	}
	DBGMSG("received unhandled signal\n");
}

static int print_usage(char *pname)
{
	printf("%s [--ipod ipod_mount_point] [--aexip AEX_ip_address]"
	       "[--help|-h] [audio_data_directory]\n", pname);
	printf("for detail, http://raop-play.sourceforge.net/\n");
	return -1;
}

int main(int argc, char *argv[])
{
	int mfd,mpid;
	char *marg[4]={MDNS_PROGRAM,"-t",RAOP_PROT_NAME,NULL};
	char *fdir=".";
	char *ipodmt=NULL;
	char aexip[32];
	int c;
	struct sigaction act;
	struct option long_options[] = {
		{"help", 0, 0, 'h'},
		{"ipod", 1, 0, 0},
		{"aexip", 1, 0, 1},
		{0, 0, 0, 0}
	};

	memset(aexip,0,sizeof(aexip));
	/* Assign sig_term as our SIGTERM handler */
	act.sa_sigaction = sig_action;
	sigemptyset(&act.sa_mask); // no other signals are blocked
	act.sa_flags = SA_SIGINFO; // use sa_sigaction instead of sa_handler
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGCHLD, &act, NULL);

	while (1) {
		c=getopt_long(argc, argv, "h", long_options, NULL);
		if (c == -1) break;
		switch (c) {
		case 'h':
			return print_usage(argv[0]);
		case 0:
			ipodmt=optarg;
			break;
		case 1:
			strncpy(aexip,optarg,sizeof(aexip)-2);
			break;
		}
	}
	if(optind<argc) fdir=argv[optind];

	// initilize aexd
	aexd.raop_infd=-1;
	aexd.raop_outfd=-1;

	// create gui
	aexd.agui=new AexclGui(fdir,aex_cntl);

	if(aexip[0]){
		strcat(aexip,IP_NAME_DELIM);
		aexd.agui->aex_chooser->add(aexip,"",chcb);
		aexd.agui->aex_chooser->value(0);
	}else{
		mpid=child_start(marg,&mfd,NULL,NULL);
		Fl::add_fd(mfd, FL_READ, mdns_event, NULL);
	}
	aexd.agui->file_browser->filter("*.{m4a|wav|mp3|ogg|aac|pls|flac}");
	if(ipodmt && !aexd.agui->ipod_browser->read_itunesdb(ipodmt)){
		aexd.agui->file_browser->hide();
		aexd.agui->ipod_browser->show();
		aexd.agui->ipod_mode->value(1);
	}else{
		aexd.agui->ipod_mode->value(0);
		aexd.agui->ipod_mode->deactivate();
	}
	Fl::run();
	kill(mpid,SIGTERM);
	if(aexd.raop_pid) kill(aexd.raop_pid,SIGTERM);
	delete aexd.agui;
	cleanup_filelist();
	DBGMSG("%s end\n",__func__);
	return 0;
}
