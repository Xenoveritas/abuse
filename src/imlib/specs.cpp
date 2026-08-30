/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#if defined HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
# include <io.h>
#endif

#ifndef O_BINARY
// Assume we don't have a binary flag on this platform.
// FIXME: This should probably be done in a less stupid way.
#define O_BINARY 0
#endif

#include "common.h"

#include "image.h"
#include "palette.h"
#include "specs.h"
#include "dprint.h"

char const *spec_types[] =
{
    "Invalid type", // 0
    "Color table",  // 1
    "Palette",      // 2
    "Invalid Type", // 3
    "Image",        // 4
    "Fore Tile",
    "Back Tile",
    "Character",
    "8 Morph",
    "16 Morph",
    "Grue objs",
    "Extern WAV",
    "DMX MUS",
    "Patched morph",
    "Normal file",
    "Compress1 file",
    "Vector Image",
    "Light list",
    "Grue fgmap",
    "Grue bgmap",
    "Data array",
    "Character2",
    "Particle",
    "Extern lcache"
};


int total_files_open=0;
char spec_main_file[100];

static char *spec_prefix=NULL;
static char *save_spec_prefix=NULL;

static jFILE spec_main_jfile((FILE*)0);
static int spec_main_fd = -1;
static long spec_main_offset = -1;
static spec_directory spec_main_sd;

void set_filename_prefix(char const *prefix)
{
    if( spec_prefix )
    {
        free( spec_prefix );
    }

    if( prefix )
    {
        spec_prefix = strcpy( (char *)malloc( strlen( prefix ) + 2 ), prefix );
        int len = strlen( prefix );
        if( prefix[len - 1] != '\\' && prefix[len - 1] != '/')
        {
            spec_prefix[len] = PATH_SEPARATOR_CHAR;
            spec_prefix[len + 1] = 0;
        }
    }
    else
    {
        spec_prefix = NULL;
    }
}

char *get_filename_prefix()
{
    return spec_prefix;
}


void set_save_filename_prefix(char const *save_prefix)
{
    if( save_spec_prefix )
    {
        free( save_spec_prefix );
    }

    if( save_prefix )
    {
        int len = strlen( save_prefix );
        save_spec_prefix = (char *)malloc( len + 1 );
        strcpy( save_spec_prefix, save_prefix );
/* AK - Commented this out as it may cause problems
        if( save_prefix[len - 1] != '\\' && save_prefix[len - 1] != '/' )
        {
            save_spec_prefix[len] = '/';
            save_spec_prefix[len + 1] = '\0';
        } */
    }
    else
    {
        save_spec_prefix = NULL;
    }
}

char *get_save_filename_prefix()
{
  return save_spec_prefix;
}

int search_order=SPEC_SEARCH_OUTSIDE_INSIDE;

static void (*no_space_handle_fun)()=NULL;

void set_no_space_handler(void (*handle_fun)())
{
  no_space_handle_fun=handle_fun;
}


bFILE::bFILE()
{
  rbuf_size=8192;
  rbuf=(unsigned char *)malloc(rbuf_size);
  rbuf_start=rbuf_end=0;

  wbuf_size=8192;
  wbuf=(unsigned char *)malloc(wbuf_size);
  wbuf_end=0;
}

bFILE::~bFILE()
{
  if (rbuf) free(rbuf);
  flush_writes();
  if (wbuf) free(wbuf);
}

int bFILE::flush_writes()
{
  if (wbuf_end!=0)
  {
    unsigned long ret=unbuffered_write(wbuf,wbuf_end);
    if (ret!=wbuf_end && no_space_handle_fun)
      no_space_handle_fun();

    wbuf_end=0;
    return ret;
  }
  return 0;
}

int bFILE::read(void *buf, size_t count)       // returns number of bytes read, calls unbuffer_read
{
  if (!allow_read_buffering())
    return unbuffered_read(buf,count);

  int total_read=0,error=0;
  if (!count) return 0;
  while (count && !error)
  {
    if (rbuf_start<rbuf_end)
    {
      unsigned int avail_size=rbuf_end-rbuf_start;
      int copy_size=avail_size>count ? count : avail_size;
      memcpy(buf,rbuf+rbuf_start,copy_size);
      buf=(void *)(((unsigned char *)buf)+copy_size);
      rbuf_start+=copy_size;
      if (rbuf_start>=rbuf_end)
      {
                if (rbuf_end!=rbuf_size)  // buffer wasn't full before so there is no way we can complete read
                  error=1;
                rbuf_start=rbuf_end=0;
      }
      total_read+=copy_size;
      count-=copy_size;
    } else
    {
      rbuf_end=unbuffered_read(rbuf,rbuf_size);
      if (rbuf_end==0) error=1;
      rbuf_start=0;
    }
  }
  return total_read;
}


int bFILE::write(void const *buf, size_t count)      // returns number of bytes written
{
  if (allow_write_buffering())
  {
    int total_written=0;
    while (count)
    {
      int copy_size=wbuf_end+count<=wbuf_size ? count :  wbuf_size-wbuf_end;
      memcpy(wbuf+wbuf_end,buf,copy_size);
      wbuf_end+=copy_size;
      count-=copy_size;
      buf=(void *)(((char *)buf)+copy_size);
      if (wbuf_end==wbuf_size)
        if ((unsigned int)flush_writes()!=wbuf_size)
      return total_written;

      total_written+=copy_size;
    }
    return total_written;
  } else
  {
    unsigned long ret=unbuffered_write(buf,count);
    if (ret!=count && no_space_handle_fun)
      no_space_handle_fun();
  }
  return 0;
}

int bFILE::seek(long offset, int whence) // whence=SEEK_SET, SEEK_CUR, SEEK_END, ret=0=success
{
//    rbuf_start=rbuf_end=0;
//    unbuffered_seek(offset,SEEK_SET);

  long realpos=unbuffered_tell();
  long curpos=realpos-rbuf_end+rbuf_start;
  if (whence==SEEK_CUR) offset+=curpos;
  else if (whence==SEEK_END) offset=file_size()-offset;

  if (offset<realpos-(long)rbuf_end || offset>=realpos)
  {
    rbuf_start=rbuf_end=0;
    unbuffered_seek(offset,SEEK_SET);
  } else
    rbuf_start=rbuf_end-(realpos-offset);
  return 1;
}

int bFILE::tell()
{
  return unbuffered_tell()-rbuf_end+rbuf_start+
         wbuf_end;    // if this a write file, add on how much we've written
}

int bFILE::allow_read_buffering() { return 1; }
int bFILE::allow_write_buffering() { return 1; }

void set_spec_main_file(char const *filename, int Search_order)
{
  dprintf("Specs : main file set to %s\n",filename);
  strcpy(spec_main_file,filename);
  search_order=Search_order;

  spec_main_jfile.open_external(filename,"rb",O_BINARY|O_RDONLY);
  spec_main_fd = spec_main_jfile.get_fd();
  if (spec_main_fd==-1)
    return;
  spec_main_sd.startup(&spec_main_jfile);
}

jFILE::jFILE(FILE *file_pointer)                       // assumes fp is at begining of file
{
  access=0;
  fd=-1;
  file_length=0;
  start_offset=0;
  flags=JFILE_CLONED;
}

void jFILE::open_external(char const *filename, char const *mode, int flags)
{
  int skip_size=0;
  char tmp_name[200];
#ifdef WIN32
  // Need to make sure it's not an absolute Windows path
  if (spec_prefix && filename[0] != '/' && (filename[0] != '\0' && filename[1] != ':'))
#else
  if (spec_prefix && filename[0] != '/')
#endif
  {
    snprintf(tmp_name,200,"%s%s",spec_prefix,filename);
  }
  else
  {
    strcpy(tmp_name,filename);
  }

//  int old_mask=umask(S_IRWXU | S_IRWXG | S_IRWXO);
  if (flags&O_WRONLY)
  {
    if ((flags&O_APPEND)==0)
    {
      skip_size=1;
      //int errval = unlink(tmp_name);
    }

    flags-=O_WRONLY;
    flags|=O_CREAT|O_RDWR;
#ifdef WIN32
    //printf("Open %s flags %x\n", tmp_name, flags);
    fd=open(tmp_name,flags,_S_IREAD | _S_IWRITE);
#else
    fd=open(tmp_name,flags,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif
  }
  else
  {
    //printf("Open %s flags %x\n", tmp_name, flags);
    fd = open(tmp_name, flags);
  }

//  umask(old_mask);
  if (fd>=0 && !skip_size)
  {
    file_length=lseek(fd,0,SEEK_END);
    if ((flags&O_APPEND)==0)
      lseek(fd,0,SEEK_SET);
    else
        current_offset = file_length;
    start_offset=0;
  } else
  {
    file_length=0;
    start_offset=0;
  }
}


class null_file : public bFILE     // this file type will use virtual opens inside of a spe
{
  public :
  virtual int open_failure() { return 1; }
  virtual int unbuffered_read(void *buf, size_t count)   { return 0; }
  virtual int unbuffered_write(void const *buf, size_t count)  { return 0; }
  virtual int unbuffered_seek(long offset, int whence)   { return 0; }

  virtual int unbuffered_tell() { return 0; }
  virtual int file_size() { return 0; }
  virtual ~null_file() { ; }
} ;


static bFILE *(*open_file_fun)(char const *,char const *)=NULL;
int (*verify_file_fun)(char const *,char const *)=NULL;

void set_file_opener(bFILE *(*open_fun)(char const *, char const *))
{
  open_file_fun=open_fun;
}

bFILE *open_file(char const *filename, char const *mode)
{
  if (!verify_file_fun || verify_file_fun(filename,mode))
  {
    if (open_file_fun)
    {
      return open_file_fun(filename,mode);
    }
    else
    {
      return new jFILE(filename,mode);
    }
  }
  else
  {
    return new null_file;
  }
}

void jFILE::open_internal(char const *filename, char const *mode, int flags)
{
  int wr=0;
  for (char const *s=mode; *s; s++)
    if (toupper(*s)=='A' || toupper(*s)=='W')
      wr=1;

  if (wr)
    fd=-1;                 // only allow extern file openings for writing
  else
  {
       fd = spec_main_fd;
    if (fd>=0)                    // if we were able to open the main file, see if it's in there
    {
      start_offset=0;
      spec_entry *se=spec_main_sd.find(filename);
      if (se)
      {
    start_offset=se->offset;
    current_offset = 0;
    file_length=se->size;
    rbuf_start=rbuf_end=0;
      } else
      {
    close(fd);
    fd=-1;
      }
    }
  }
}

jFILE::jFILE(char const *filename, char const *access_string)      // same as fopen parameters
{
 flags=access=0;
 char const *s=access_string;
  for (; *s; s++)
    if (toupper(*s)=='R') access=O_RDONLY;

  for (s=access_string; *s; s++)
    if (toupper(*s)=='W')
    {
      if (access)
        access=O_RDWR;
      else access=O_WRONLY;
    }

  for (s=access_string; *s; s++)
  {
    if (toupper(*s)=='A')
      access|=O_APPEND|O_WRONLY;
#ifdef WIN32
    // Also check for 'b' - doesn't exist on other platforms
    if (toupper(*s)=='B')
      access|=O_BINARY;
#endif
  }

  file_length=start_offset=-1;
  current_offset = 0;

  fd=-1;
  if (search_order==SPEC_SEARCH_OUTSIDE_INSIDE)
    open_external(filename,access_string,access);

  if (fd<0)
    open_internal(filename,access_string,access);

  if (fd<0 && search_order==SPEC_SEARCH_INSIDE_OUTSIDE)
    open_external(filename,access_string,access);

  total_files_open++;
}

jFILE::~jFILE()
{
  flush_writes();
  if (fd>=0 && !(flags&JFILE_CLONED))
  {
    total_files_open--;
    if (fd != spec_main_fd)
        close(fd);
  }
}

int jFILE::unbuffered_tell()
{
//    int ret = ::lseek(fd,0,SEEK_CUR) - start_offset;
//    if (ret != current_offset)
//        fprintf(stderr,"Bad tell %d\n",current_offset);
    return current_offset;
}

int jFILE::unbuffered_read(void *buf, size_t count)
{
    unsigned long len;

    if (fd == spec_main_fd)
    {
        if (current_offset+start_offset != spec_main_offset)
            spec_main_offset = lseek(fd, start_offset+current_offset, SEEK_SET);

        len = ::read(fd,(char*)buf,count);
        spec_main_offset += len;
    }
    else
    {
      len = ::read(fd,(char*)buf,count);
    }
    current_offset += len;
    return len;
}

int jFILE::unbuffered_write(void const *buf, size_t count)
{
  long ret = ::write(fd,(char*)buf,count);
    current_offset += ret;
    return ret;
}

int jFILE::unbuffered_seek(long offset, int whence) // whence=SEEK_SET, SEEK_CUR, SEEK_END, ret=0=success
{
  long ret;

  switch (whence)
  {
    case SEEK_SET :
    { ret = lseek(fd,start_offset+offset,SEEK_SET); } break;
    case SEEK_END :
    { ret = lseek(fd,start_offset+file_length-offset,SEEK_SET); } break;
    case SEEK_CUR :
    { ret = lseek(fd,offset,SEEK_CUR); } break;
    default:
        ret = -1;
        break;
  }
  if (ret>=0)
  {
    current_offset = ret - start_offset;
    if (spec_main_fd == fd)
      spec_main_offset = ret;
    return ret;
  }
  else
    return -1;  // if a bad whence, then failure
}


uint8_t bFILE::read_uint8()
{ uint8_t x;
  read(&x,1);
  return x;
}

uint16_t bFILE::read_uint16()
{
  uint16_t x;
  read(&x,2);
  return lstl(x);
}


uint32_t bFILE::read_uint32()
{
  uint32_t x;
  read(&x,4);
  return lltl(x);
}

void bFILE::write_uint8(uint8_t x)
{
  write(&x,1);
}

void bFILE::write_uint16(uint16_t x)
{
  x=lstl(x);
  write(&x,2);
}


void bFILE::write_uint32(uint32_t x)
{
  x=lltl(x);
  write(&x,4);
}

void bFILE::write_double(double x)
{
  double a;
  write_uint32((long)(modf(x,&a)*(double)(1<<31)));
  write_uint32((long)a);
}

double bFILE::read_double()
{
  long a,b;
  a=read_uint32();
  b=read_uint32();
  return (double)b+a/(double)(1<<31);
}

spec_directory::~spec_directory()
{

  if (total)
  {
    free(data);
    free(entries);
  }
}

void spec_directory::FullyLoad(bFILE *fp)
{
    for (int i = 0; i < total; i++)
    {
        spec_entry *se = entries[i];
        free(se->data);
        se->data = malloc(se->size);
        fp->seek(se->offset, SEEK_SET);
        fp->read(se->data, se->size);
    }
}

spec_entry::spec_entry(uint8_t spec_type, char const *object_name,
                       char const *link_filename,
                       unsigned long data_size, unsigned long data_offset)
{
    type = spec_type;
    name = strdup(object_name);
    data = NULL;
    size = data_size;
    offset = data_offset;
}

spec_entry::~spec_entry()
{
    free(name);
    free(data);
}

void spec_entry::Print()
{
    printf("%15s%25s%8ld%8ld\n", spec_types[type], name, size, offset);
}

void spec_directory::calc_offsets()
{
    size_t o = SPEC_SIG_SIZE + 2;

    // calculate the size of directory info
    for (int i = 0; i < total; i++)
        o += 1 + 1 + strlen(entries[i]->name) + 1 + 1 + 8;

    // calculate offset for each entry
    for (int i = 0; i < total; i++)
    {
        entries[i]->offset = o;
        o += entries[i]->size;
    }
}

spec_entry *spec_directory::find(char const *name, int type)
{
  int i;
  spec_entry **e;
  for (i=0,e=entries; i<total; i++,e++)
    if (!strcmp((*e)->name,name) && (*e)->type==type)
      return (*e);
  return NULL;
}

spec_entry *spec_directory::find(char const *name)
{
  int i;
  spec_entry **e;
  for (i=0,e=entries; i<total; i++,e++)
    if (!strcmp((*e)->name,name))
      return (*e);
  return NULL;
}

long spec_directory::find_number(char const *name)
{
  int i;
  spec_entry **e;
  for (i=0,e=entries; i<total; i++,e++)
    if (!strcmp((*e)->name,name))
      return i;
  return -1;
}

spec_entry *spec_directory::find(int type)
{
  int i;
  spec_entry **e;
  for (i=0,e=entries; i<total; i++,e++)
    if ((*e)->type==type)
      return (*e);
  return NULL;
}

long spec_directory::type_total(int type)
{
  int i,x=0;
  spec_entry **e;
  for (i=0,e=entries; i<total; i++,e++)
    if ((*e)->type==type) x++;
  return x;
}

long spec_directory::find_number(int type)
{
  int i;
  spec_entry **e;
  for (i=0,e=entries; i<total; i++,e++)
    if ((*e)->type==type)
      return i;
  return -1;
}

void spec_directory::print()
{
  spec_entry **se;
  int i;
  printf("[   Entry type   ][   Entry name   ][  Size  ][ Offset ]\n");
  for (i=0,se=entries; i<total; i++,se++)
    (*se)->Print();
}


void spec_directory::startup(bFILE *fp)
{
  char buf[256];
  memset(buf,0,256);
  fp->read(buf,8);
  buf[9]=0;
  size=0;
  if (!strcmp(buf,SPEC_SIGNATURE))
  {
    total=fp->read_uint16();
    entries=(spec_entry **)malloc(sizeof(spec_entry *)*total);
    long start=fp->tell();

    int i;
    for (i=0; i<total; i++)
    {
      fp->read(buf,2);
      long entry_size=sizeof(spec_entry)+(unsigned char)buf[1];
      entry_size=(entry_size+3)&(~3);
      fp->read(buf,(unsigned char)buf[1]);
      fp->read(buf,9);

      size+=entry_size;
    }
    data=malloc(size);
    char *dp=(char *)data;
    fp->seek(start,SEEK_SET);
    for (i=0; i<total; i++)
    {
      spec_entry *se=(spec_entry *)dp;
      entries[i]=se;

      unsigned char len,flags,type;
      fp->read(&type,1);
      fp->read(&len,1);
      se->type=type;
      se->data = NULL;
      se->name=dp+sizeof(spec_entry);
      fp->read(se->name,len);
      fp->read(&flags,1);

      se->size=fp->read_uint32();
      se->offset=fp->read_uint32();
      dp+=((sizeof(spec_entry)+len)+3)&(~3);
    }
  }
  else
  {
    total=0;
    data=NULL;
    entries=NULL;
  }
}


spec_directory::spec_directory(bFILE *fp)
{ startup(fp); }

spec_directory::spec_directory(FILE *fp)
{
  jFILE jfp(fp);
  startup(&jfp);
}

spec_directory::spec_directory()
{
  size=0;
  total=0;
  data=NULL;
  entries=NULL;
}

/*
spec_directory::spec_directory(char *filename)
{
  jFILE *fp;
  if (filename)
  {
    fp=new jFILE(filename,"rb");
    if (!fp->open_failure())
      startup(fp);
    else
    {
      total=0;
      entries=NULL;
    }
    delete fp;
  } else printf("NULL filename to spec_directory::spec_directory\n");
}*/

int write_string(bFILE *fp, char const *st)
{
  unsigned char length=strlen(st)+1;
  if (fp->write(&length,1)!=1) return 0;
  if (fp->write(st,length)!=length) return 0;
  return 1;
}

long spec_directory::data_start_offset()
{
    /* FIXME: no need for a for loop here! */
    long i;
    for(i = 0; i < total; i++)
        return entries[i]->offset;

    // If no entries, then no data, but return where it would start anyway
    return SPEC_SIG_SIZE + 2;
}

long spec_directory::data_end_offset()
{
    /* FIXME: no need for a for loop here! */
  spec_entry **e;
  long i;
  for (i=total-1,e=entries; i>=0; i--,e++)
    return (*e)->offset+(*e)->size;

  return SPEC_SIG_SIZE+2;
}

int spec_directory::write(bFILE *fp)
{

  char sig[SPEC_SIG_SIZE];
  unsigned char flags=0;
  unsigned long offset,data_size;
  spec_entry **e;
  strcpy(sig,SPEC_SIGNATURE);

  if (fp->write(sig,sizeof(sig))!=sizeof(sig))    return 0;
  fp->write_uint16(total);


  int i;
  for (i=0,e=entries; i<total; i++,e++)
  {
    if (fp->write(&(*e)->type,1)!=1)                 return 0;
    if (!write_string(fp,(*e)->name))                return 0;
    flags=0;
    if (fp->write(&flags,1)!=1)                     return 0;

    data_size=lltl((*e)->size);
    if (fp->write((char *)&data_size,4)!=4)              return 0;
    offset=lltl((*e)->offset);
    if (fp->write((char *)&offset,4)!=4)                  return 0;

  }
  return 1;
}

jFILE *spec_directory::write(char const *filename)
{
  jFILE *fp;
  fp=new jFILE(filename,"wb");
  if (fp->open_failure()) { delete fp; return NULL; }
  if (!write(fp))
  {
    delete fp;
    return NULL;
  } else return fp;

}

uint16_t read_uint16(FILE *fp)
{
  uint16_t x;
  fread(&x,1,2,fp);
  return lstl(x);
}

uint32_t read_uint32(FILE *fp)
{
  uint32_t x;
  fread(&x,1,4,fp);
  return lltl(x);
}
void write_uint16(FILE *fp, uint16_t x)
{
  x=lstl(x);
  fwrite(&x,1,2,fp);
}

void write_uint32(FILE *fp, uint32_t x)
{
  x=lltl(x);
  fwrite(&x,1,4,fp);
}

uint8_t read_uint8(FILE *fp) { return fgetc(fp)&0xff; }
void write_uint8(FILE *fp, uint8_t x) { fputc((unsigned char)x,fp); }

void spec_directory::remove(spec_entry *e)
{
  int i;
  for (i=0; i<total && entries[i]!=e; i++);            // find the entry in the array first

  if (entries[i]==e)                                 // make sre it was found
  {
    delete e;
    total--;
    for (; i<total; i++)                               // compact the pointer array
      entries[i]=entries[i+1];
    entries=(spec_entry **)realloc(entries,sizeof(spec_entry *)*total);
  }
  else
    printf("Spec_directory::remove bad entry pointer\n");
}



void spec_directory::add_by_hand(spec_entry *e)
{
  total++;
  entries=(spec_entry **)realloc(entries,sizeof(spec_entry *)*total);
  entries[total-1]=e;
}

void spec_directory::delete_entries()   // if the directory was created by hand instead of by file
{
  int i;
  for (i=0; i<total; i++)
    delete entries[i];

  if (total)
    free(entries);
}

void note_open_fd(int fd, char const *str)
{
    total_files_open++;
}

void note_close_fd(int fd)
{
    total_files_open--;
}

void list_open_fds()
{
    printf("Total open file descriptors: %d\n", total_files_open);
}
