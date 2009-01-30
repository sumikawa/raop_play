/*****************************************************************************
 * glibsubst.c : substitute functions for glib
 * this supports plain English text only.
 * To support localized characters, use libglib-2.0 .
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include "itunesdb.h"
#include "aexcl_lib.h"


#if (GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 16)
inline gpointer g_malloc(gulong size)
#else
inline gpointer g_malloc(gsize size)
#endif
{
	return malloc(size);
}

#if (GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 16)
inline gpointer g_malloc0(gulong size)
#else
inline gpointer g_malloc0(gsize size)
#endif
{
	gpointer p;
	if((p=malloc(size))) memset(p,0,size);
	return p;
}

inline void g_free(gpointer p)
{
	return free(p);
}

inline gchar* g_strdup(const gchar *str)
{
	return strdup(str);
}

// support only simple ascii strings
gunichar2 *g_utf8_to_utf16 (const gchar *str,
			    glong        len,              
			    glong       *items_read,       
			    glong       *items_written,    
			    GError     **error)
{
	int i;
	gunichar2 *res;
	res=malloc((strlen(str)+1)*2);
	if(!res) return NULL;
	for(i=0;i<strlen(str);i++){
		res[i]=str[i];
	}
	res[i]=0;
	return res;
}

/*
 * this covers only next few 2-byte characters
 */
#define LEFT_SINGLE_QUOTE 0x2018
#define RIGHT_SINGLE_QUOTE 0x2019
#define LEFT_DOUBLE_QUOTE 0x201c
#define RIGHT_DOUBLE_QUOTE 0x201d
gchar* g_utf16_to_utf8(const gunichar2 *str,
		       glong len,
		       glong *items_read,
		       glong *items_written,
		       GError **error)
{
	int i,count=0;
	gchar *res;
	while(str[count]) count++;
	res=malloc(count+1);
	if(!res) return NULL;
	for(i=0;i<count;i++){
		switch(str[i]){
		case LEFT_SINGLE_QUOTE:
		case RIGHT_SINGLE_QUOTE:
			res[i]='\'';
			break;
		case LEFT_DOUBLE_QUOTE:
		case RIGHT_DOUBLE_QUOTE:
			res[i]='"';
			break;
		default:
			res[i]=str[i]&0xff;
			break;
		}
	}
	res[i]=0;
	return res;
}


gpointer g_list_nth_data (GList     *list,
			  guint      n)
{
	while ((n-- > 0) && list)
		list = list->next;
  
	return list ? list->data : NULL;
}

GList* g_list_last (GList *list)
{
	if (list)
	{
		while (list->next)
			list = list->next;
	}
  
	return list;
}

GList* g_list_first (GList *list)
{
	if (list)
	{
		while (list->prev)
			list = list->prev;
	}
  
	return list;
}


GList* g_list_remove (GList	     *list,
		      gconstpointer  data)
{
	GList *tmp;
  
	tmp = list;
	while (tmp)
	{
		if (tmp->data != data)
			tmp = tmp->next;
		else
		{
			if (tmp->prev)
				tmp->prev->next = tmp->next;
			if (tmp->next)
				tmp->next->prev = tmp->prev;
	  
			if (list == tmp)
				list = list->next;
	  
			//_g_list_free_1 (tmp);
			free(tmp);
	  
			break;
		}
	}
	return list;
}

	
GList* g_list_append (GList	*list,
		      gpointer	 data)
{
	GList *new_list;
	GList *last;
  
	//new_list = _g_list_alloc ();
	new_list=g_malloc0(sizeof(GList));
	new_list->data = data;
  
	if (list)
	{
		last = g_list_last (list);
		/* g_assert (last != NULL); */
		last->next = new_list;
		new_list->prev = last;

		return list;
	}
	else
		return new_list;
}


gchar* g_path_get_dirname (const gchar *file_name)
{
	register gchar *base;
	register gsize len;    
  
	if(!file_name) return NULL;
  
	base = strrchr (file_name, G_DIR_SEPARATOR);
	if (!base)
		return g_strdup (".");
	while (base > file_name && *base == G_DIR_SEPARATOR)
		base--;
	len = (guint) 1 + base - file_name;
  
	base = (gchar*)malloc(len+1);
	memmove (base, file_name, len);
	base[len] = 0;
  
	return base;
}



gchar *g_build_filename1 (const gchar *first_element, const gchar *file_name,  void *dummy)
{
	gchar *res;
	res = (gchar*)malloc(strlen(first_element)+strlen(file_name)+2);
	strcpy(res,first_element);
	strcat(res,"/");
	strcat(res,file_name);
	return res;
}

// assume, it can be filename as it is
gchar* g_filename_from_utf8 (const gchar *utf8string,
				    gssize       len,            
				    gsize       *bytes_read,    
				    gsize       *bytes_written,
				    GError     **error)
{
	return strdup(utf8string);
}

gchar* g_filename_to_utf8 (const gchar *opsysstring, 
			   gssize       len,           
			   gsize       *bytes_read,   
			   gsize       *bytes_written,
			   GError     **error)
{
	return strdup(opsysstring);
}


gchar* g_utf8_casefold (const gchar *str,
			gssize len)
{
	int i;
	gchar *res = (gchar*) malloc(strlen(str)+1);
	if(res) {
		for(i=0;i<strlen(str);i++){
			res[i]=tolower(str[i]);
		}
		res[i]=0;
	}
	return (gchar*)res;
}


/* this ia a full copy from glib-2.0 */
gboolean g_file_test (const gchar *filename,
		      GFileTest    test)
{
	if ((test & G_FILE_TEST_EXISTS) && (access (filename, F_OK) == 0))
		return TRUE;
  
	if ((test & G_FILE_TEST_IS_EXECUTABLE) && (access (filename, X_OK) == 0))
		return TRUE;

	if (test & G_FILE_TEST_IS_SYMLINK)
	{
		struct stat s;

		if ((lstat (filename, &s) == 0) && S_ISLNK (s.st_mode))
			return TRUE;
	}
  
	if (test & (G_FILE_TEST_IS_REGULAR | G_FILE_TEST_IS_DIR))
	{
		struct stat s;
      
		if (stat (filename, &s) == 0)
		{
			if ((test & G_FILE_TEST_IS_REGULAR) && S_ISREG (s.st_mode))
				return TRUE;
	  
			if ((test & G_FILE_TEST_IS_DIR) && S_ISDIR (s.st_mode))
				return TRUE;
		}
	}

	return FALSE;
}

typedef struct _GDir {
	DIR *dir;
} _GDir;

GDir *g_dir_open (const gchar  *path,
		  guint         flags,
		  GError      **error)
{
	GDir *dir;

	if(!path) return NULL;
	if(!(dir = (GDir*)malloc(sizeof(GDir)))) return NULL;
	dir->dir = opendir(path);

	if (dir->dir)
		return dir;

	/* show error message */
	g_free (dir);
	return NULL;
}

G_CONST_RETURN gchar* g_dir_read_name (GDir *dir)
{
	struct dirent *entry;

	if(!dir) return NULL;

	entry = readdir (dir->dir);
	while (entry 
	       && (0 == strcmp (entry->d_name, ".") ||
		   0 == strcmp (entry->d_name, "..")))
		entry = readdir (dir->dir);

	if (entry)
		return entry->d_name;
	else
		return NULL;
}


void g_dir_close (GDir *dir)
{
	if(!dir) return;

	closedir (dir->dir);
	g_free (dir);
}


gint g_utf8_collate (const gchar *str1,
		     const gchar *str2)
{
	return strcmp(str1,str2);
}
