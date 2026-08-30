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

#if (defined(__MACH__) || !defined(__APPLE__))
#   include <sys/stat.h>
#endif

#include <fcntl.h>
#include <string.h>

#include "common.h"

#include "cache.h"
#include "lisp.h"
#include "video.h"
#include "dprint.h"
#include "lcache.h"
#include "status.h"
#include "game.h"
#include "lisp_gc.h"
#include "level.h"
#include "status.h"
#include "crc.h"
#include "dev.h"
#include "specache.h"
#include "netface.h"

#define touch(x) { (x)->last_access=last_access++; \
           if ((x)->last_access<0) { normalize(); (x)->last_access=1; } }

CrcManager crc_manager;

int past_startup=0;

int crc_man_write_crc_file(char const *filename)
{
  return crc_manager.write_crc_file(filename);
}

int CrcManager::write_crc_file(char const *filename)  // return 0 on failure
{
  char msg[100];
  snprintf(msg, 100, "%s", symbol_str("calc_crc"));  // this may take some time, show the user a status indicator
  if (stat_man) stat_man->push(msg,NULL);

  int i,total=0;
  for (i=0; i<total_files; i++)
  {
    int failed=0;
    get_crc(i,failed);

    if (failed)
    {
      jFILE *fp=new jFILE(get_filename(i),"rb");
      if (!fp->open_failure())
      {
    set_crc(i,crc_file(fp));
    total++;
      }
      delete fp;
    } else total++;
    if (stat_man)
      stat_man->update(i*100/total_files);
  }
  if (stat_man) stat_man->pop();
  jFILE *fp=new jFILE(NET_CRC_FILENAME,"wb");
  if (fp->open_failure())
  {
    delete fp;
    return 0;
  }

  fp->write_uint16(total);
  total=0;
  for (i=0; i<total_files; i++)
  {
    uint32_t crc;
    int failed=0;
    crc=get_crc(i,failed);
    if (!failed)
    {
      fp->write_uint32(crc);
      uint8_t len=strlen(get_filename(i))+1;
      fp->write_uint8(len);
      fp->write(get_filename(i),len);
      total++;
    }
  }
  delete fp;
  return 1;
}

int CrcManager::load_crc_file(char const *filename)
{
  bFILE *fp=open_file(filename,"rb");
  if (fp->open_failure())
  {
    delete fp;
    return 0;
  } else
  {
    short total=fp->read_uint16();
    int i;
    for (i=0; i<total; i++)
    {
      char name[256];
      uint32_t crc=fp->read_uint32();
      uint8_t len=fp->read_uint8();
      fp->read(name,len);
      set_crc(get_filenumber(name),crc);
    }
    delete fp;
  }
  return 1;
}

void CrcManager::clean_up()
{
  for (int i=0; i<total_files; i++)
    delete files[i];
  if (total_files)
    free(files);
  total_files=0;
  files=NULL;
}

CrcedFile::~CrcedFile()
{
  free(filename);
}

CrcedFile::CrcedFile(char const *name)
{
  filename = strdup(name);
  crc_calculated=0;
}

CrcManager::CrcManager()
{
  total_files=0;
  files=NULL;
}

int CrcManager::get_filenumber(char const *filename)
{
  for (int i=0; i<total_files; i++)
    if (!strcmp(filename,files[i]->filename)) return i;
  total_files++;
  files=(CrcedFile **)realloc(files,total_files*sizeof(CrcedFile *));
  files[total_files-1]=new CrcedFile(filename);
  return total_files-1;
}

char *CrcManager::get_filename(int filenumber)
{
  CHECK(filenumber>=0 && filenumber<total_files);
  return files[filenumber]->filename;
}

uint32_t CrcManager::get_crc(int filenumber, int &failed)
{
  CHECK(filenumber>=0 && filenumber<total_files);
  if (files[filenumber]->crc_calculated)
  {
    failed=0;
    return files[filenumber]->crc;
  }
  failed=1;
  return 0;
}

void CrcManager::set_crc(int filenumber, uint32_t crc)
{
  CHECK(filenumber>=0 && filenumber<total_files);
  files[filenumber]->crc_calculated=1;
  files[filenumber]->crc=crc;
}

void CacheList::unmalloc(CacheItem *i)
{
  switch (i->type)
  {
    case SPEC_CHARACTER2 :
    case SPEC_CHARACTER : delete ((figure *)i->data);   break;
    case SPEC_FORETILE : delete ((foretile *)i->data);  break;
    case SPEC_BACKTILE : delete ((backtile *)i->data);  break;
    case SPEC_IMAGE    : delete ((image *)i->data);     break;
    case SPEC_EXTERN_SFX : delete ((sound_effect *)i->data); break;
    case SPEC_PARTICLE : delete ((part_frame *)i->data); break;
    case SPEC_EXTERNAL_LCACHE : if (i->data) free(i->data); break;
    case SPEC_PALETTE : delete ((char_tint *)i->data); break;
    default :
      printf("Trying to unmalloc unknown type\n");
  }
  i->data=NULL;
  i->last_access=-1;
}



void CacheList::prof_init()
{
  if (prof_data)
    free(prof_data);

  prof_data=(int *)malloc(sizeof(int)*total);
  memset(prof_data,0,sizeof(int)*total);
}

static int c_sorter(const void *a, const void *b)
{
  return cache.compare(*(int *)a,*(int *)b);
}

int CacheList::compare(int a, int b)
{
  if (prof_data[a]<prof_data[b])
    return 1;
  else if (prof_data[a]>prof_data[b])
    return -1;
  else return 0;
}


int CacheList::prof_size()
{
  int size=0;     // count up the size for a spec enrty
  size+=2;        // total filenames
  int i;
  for (i=0; i<crc_manager.total_filenames(); i++)
      size+=strlen(crc_manager.get_filename(i))+2;    // filename + 0 + size of string

  size+=4;       // number of entries saved

  for (i=0; i<total; i++)
    if (list[i].last_access>0)       // don't save unaccessed counts
      size+=2+4+1;                   // filenumber & offset & type

  return size;
}


void CacheList::prof_write(bFILE *fp)
{
  if (prof_data)
  {
    int *ordered_ids=(int *)malloc(sizeof(int)*total);
    int i;
    for (i=0; i<total; i++) ordered_ids[i]=i;
    qsort(ordered_ids,total,sizeof(int),c_sorter);

    if (fp)
    {
      fp->write_uint16(crc_manager.total_filenames());
      for (i=0; i<crc_manager.total_filenames(); i++)
      {
    int l=strlen(crc_manager.get_filename(i))+1;
        fp->write_uint8(l);
    fp->write(crc_manager.get_filename(i),l);
      }

      int tsaved=0;
      for (i=0; i<total; i++)
        if (list[i].last_access>0) tsaved++;
      fp->write_uint32(tsaved);

      for (i=0; i<total; i++)
      {
    int id=ordered_ids[i];
        if (list[id].last_access>0)       // don't save unaccessed counts
    {
      fp->write_uint8(list[id].type);    // save type, if type changed on reload
                                        // don't cache in-> its a different refrence
      fp->write_uint16(list[id].file_number);
      fp->write_uint32(list[id].offset);
    }
      }
    }

    free(ordered_ids);

  } else dprintf("Cache profiling was not initialized\n");
}

void CacheList::prof_uninit()
{
  if (prof_data)
  {
    free(prof_data);
    prof_data=NULL;
  }
}

int *sorted_id_list;


static int s_offset_compare(const void *a, const void *b)
{
  return cache.offset_compare(*(int *)a,*(int *)b);
}

int CacheList::offset_compare(int a, int b)
{
  if (list[a].offset<list[b].offset)
    return -1;
  else if (list[a].offset>list[b].offset)
    return 1;
  else if (list[a].file_number<list[b].file_number)
    return -1;
  else if (list[a].file_number>list[b].file_number)
    return 1;
  else return 0;
}


int CacheList::search(int *sarray, uint16_t filenum, int32_t offset)
{
  int x1=0,x2=total-1;
  int split;
  do
  {
    split=(x1+x2)/2;
    CacheItem *e=list+sarray[split];

    if (e->offset<offset)      // search to the right
      x1=split+1;
    else if (e->offset>offset)
      x2=split-1;
    else if (e->file_number<filenum)
      x1=split+1;
    else if (e->file_number>filenum)
      x2=split-1;
    else return sarray[split];
  } while (x1<=x2);
  return -1;
}

static int load_chars()  // returns 0 if cache filled
{
  int i;
  for (i=0; i<total_objects; i++)
  {
    if (figures[i]->get_cflag(CFLAG_NEED_CACHE_IN))
    {
      figures[i]->set_cflag(CFLAG_CACHED_IN,0);
      figures[i]->cache_in();
      figures[i]->set_cflag(CFLAG_NEED_CACHE_IN,0);
    }
  }
  return 1;

}

void CacheList::note_need(int id)
{
  if (list[id].last_access<0)
    list[id].last_access=-2;
  else
    list[id].last_access=2;
}

void CacheList::preload_cache_object(int type)
{
  if (type<0xffff)
  {
    if (!figures[type]->get_cflag(CFLAG_NEED_CACHE_IN))  // see if it's already marked
    {
      figures[type]->set_cflag(CFLAG_NEED_CACHE_IN,1);
      void *cache_fun=figures[type]->get_fun(OFUN_GET_CACHE_LIST);

      if (cache_fun)
      {
    LSpace *sp = LSpace::Current;
    LSpace::Current = &LSpace::Perm;

    void *call_with=NULL;
    push_onto_list(LNumber::Create(type),call_with);

    void *CacheList = ((LSymbol *)cache_fun)->EvalFunction(call_with);
    PtrRef r1(CacheList);

    if (CacheList && lcar(CacheList))
    {
      void *obj_list=lcar(CacheList);
      while (obj_list)
      {
        int t=lnumber_value(CAR(obj_list));
        if (t<0 || t>=total_objects)
          lbreak("Get cache list returned a bad object number %d\n",t);
        else
          preload_cache_object(t);
        obj_list=CDR(obj_list);
      }
    }
    if (CacheList && lcdr(CacheList))
    {
      void *id_list=lcar(lcdr(CacheList));
      while (id_list)
      {
        int id=lnumber_value(CAR(id_list));
        if (id<0 || id>=total)
          lbreak("Get cache list returned a bad id number %d\n",id);
        else if (list[id].last_access<0)
          list[id].last_access=-2;
        else list[id].last_access=2;

        id_list=CDR(id_list);
      }
    }
    LSpace::Current=sp;

      }
    }
  }
}

void CacheList::preload_cache(level *lev)
{
  game_object *f;
  int i;
  for (i=0; i<total_objects; i++)                       // mark all types as not needing loading
    figures[i]->set_cflag(CFLAG_NEED_CACHE_IN,0);

  for (f=lev->first_object(); f; f=f->next)               // go through each object and get requested items to cache in
    preload_cache_object(f->otype);


  int j;
  uint16_t *fg_line;
  for (j=0; j<lev->foreground_height(); j++)
  {
    fg_line=lev->get_fgline(j);
    for (i=0; i<lev->foreground_width(); i++,fg_line++)
    {
      int id=foretiles[fgvalue(*fg_line)];
      if (id>=0 && id<nforetiles)
      {
    if (list[id].last_access<0)
          list[id].last_access=-2;
    else list[id].last_access=2;
      }
    }
  }

  uint16_t *bg_line;
  for (j=0; j<lev->background_height(); j++)
  {
    bg_line=lev->get_bgline(j);
    for (i=0; i<lev->background_width(); i++,bg_line++)
    {
      int id=backtiles[bgvalue(*bg_line)];
      if (id>=0 && id<nbacktiles)
      {
    if (list[id].last_access<0)
          list[id].last_access=-2;
    else list[id].last_access=2;
      }
    }
  }

  load_chars();
}

void CacheList::load_cache_prof_info(char *filename, level *lev)
{
  int j;
  for (j=0; j<this->total; j++)
    if (list[j].last_access>=0)      // reset all loaded cache items to 0, all non-load to -1
      list[j].last_access=0;

  preload_cache(lev);                // preliminary guesses at stuff to load

  int load_fail=1;
  bFILE *fp=open_file(filename,"rb");
  if (!fp->open_failure())
  {
    spec_directory sd(fp);
    spec_entry *se=sd.find("cache profile info");   // see if the cache profile info is in the file
    if (se)
    {
      fp->seek(se->offset,0);

      char name[255];
      int tnames=0;
      int *fnum_remap;    // remaps old filenumbers into current ones

      tnames=fp->read_uint16();
      if (tnames)                     /// make sure there isn't bad info in the file
      {
    fnum_remap=(int *)malloc(sizeof(int)*tnames);

    int i;
    for (i=0; i<tnames; i++)
    {
      fp->read(name,fp->read_uint8());
      fnum_remap[i]=-1;                    // initialize the map to no-map

      int j;
      for (j=0; j<crc_manager.total_filenames(); j++)
        if (!strcmp(crc_manager.get_filename(j),name))
          fnum_remap[i]=j;
    }

    int tsaved = fp->read_uint32();


    int *priority=(int *)malloc(tsaved*sizeof(int));
    memset(priority,0xff,tsaved*sizeof(int));   // initialize to -1
    int tmatches=0;

    sorted_id_list=(int *)malloc(sizeof(int)*total);
    for (j=0; j<total; j++) sorted_id_list[j]=j;
    qsort(sorted_id_list,total,sizeof(int),s_offset_compare);

    for (i=0; i<tsaved; i++)
    {
      fp->read_uint8(); // read type
      short file_num=fp->read_uint16();
      if (file_num>=tnames)  // bad data?
        file_num=-1;
      else file_num=fnum_remap[file_num];

      uint32_t offset=fp->read_uint32();

      // search for a match
      j=search(sorted_id_list,file_num,offset);
      if (j!=-1)
      {
        if (list[j].last_access<0)  // if not loaded
          list[j].last_access=-2;      // mark as needing loading
        else list[j].last_access=2;   // mark as loaded and needing to stay that way
        priority[i]=j;
        tmatches++;
      }
    }

    free(sorted_id_list);            // was used for searching, no longer needed

    for (j=0; j<total; j++)
      if (list[j].last_access==0)
        unmalloc(list+j);             // free any cache entries that are not accessed at all in the level


    ful=0;
    int tcached=0;
    for (j=0; j<total; j++)    // now load all of the objects until full
    {
//      stat_man->update(j*70/total+25);
      if (list[j].file_number>=0 && list[j].last_access==-2)
      {
        list[j].last_access=-1;
        if (!ful)
        {
          switch (list[j].type)
          {
        case SPEC_BACKTILE : backt(j); break;
        case SPEC_FORETILE : foret(j); break;
        case SPEC_CHARACTER :
        case SPEC_CHARACTER2 : fig(j); break;
        case SPEC_IMAGE : img(j); break;
        case SPEC_PARTICLE : part(j); break;
        case SPEC_EXTERN_SFX : sfx(j); break;
        case SPEC_EXTERNAL_LCACHE : lblock(j); break;
        case SPEC_PALETTE : ctint(j); break;
          }
          tcached++;
        }
      }
    }
    load_fail=0;
//    if (full())
//      dprintf("Cache filled while loading\n");

    if (tsaved>tmatches)
      tmatches=tsaved+1;

    last_access=tmatches+1;
    for (i=0; i<tsaved; i++)      // reorder the last access of each cache to reflect prioirties
    {
      if (priority[i]!=-1)
      {
        if (list[priority[i]].last_access!=-1)            // make sure this wasn't the last item
          list[priority[i]].last_access=tmatches--;
      }
    }

    free(priority);
    free(fnum_remap);


      }
    }
  }

  if (load_fail) // no cache file, go solely on above gueses
  {
    int j;
    for (j=0; j<total; j++)    // now load all of the objects until full, don't free old stuff
    {
//      stat_man->update(j*70/total+25);

      if (list[j].file_number>=0 && list[j].last_access==-2)
      {
    list[j].last_access=-1;
    if (!ful)
    {
      switch (list[j].type)
      {
        case SPEC_BACKTILE : backt(j); break;
        case SPEC_FORETILE : foret(j); break;
        case SPEC_CHARACTER :
        case SPEC_CHARACTER2 : fig(j); break;
        case SPEC_IMAGE : img(j); break;
        case SPEC_PARTICLE : part(j); break;
        case SPEC_EXTERN_SFX : sfx(j); break;
        case SPEC_EXTERNAL_LCACHE : lblock(j); break;
        case SPEC_PALETTE : ctint(j); break;
      }
    }
      }
    }
    if (full())
      dprintf("Cache filled while loading\n");
  }
  delete fp;
}


void CacheList::prof_poll_start()
{
  poll_start_access=last_access;
}

void CacheList::prof_poll_end()
{
  if (prof_data)
  {
    int i=0;
    for (; i<total; i++)
    {
      if (list[i].last_access>=poll_start_access)
        prof_data[i]++;
    }
  }
}

void CacheList::unreg(int id)
{
    if (list[id].file_number >= 0)
    {
        unmalloc(&list[id]);
        list[id].file_number = -1;
    }
    else
        printf("Error : trying to unregister free object\n");
}

CacheList::CacheList()
{
    // Start out with a decent sized cache buffer because it's going to
    // get allocated anyway.
    total = 0;
    list = NULL;
    last_registered = -1;
    fp = NULL;
    last_access = 1;
    used = ful = 0;
    last_dir = NULL;
    last_file = -1;
    prof_data = NULL;
}

CacheList::~CacheList()
{
}

void CacheList::empty()
{
  for (int i=0; i<total; i++)
  {
    if (list[i].file_number>=0 && list[i].last_access!=-1)
      unmalloc(&list[i]);
  }
  free(list);
  if (fp) delete fp;
  if (last_dir) delete last_dir;

  if (prof_data)
  {
    delete prof_data;
    prof_data=NULL;
  }

  total=0;                    // reinitalize
  list=NULL;
  last_registered=-1;
  fp=NULL;

  last_access=1;
  used=ful=0;
  last_dir=NULL;
  last_file=-1;
  prof_data=NULL;
}

void CacheList::locate(CacheItem *i, int local_only)
{
//  dprintf("cache in %s, type %d, offset %d\n",crc_manager.get_filename(i->file_number),i->type,i->offset);
  if (i->file_number!=last_file)
  {
    if (fp) delete fp;
    if (last_dir) delete last_dir;
    if (local_only)
      fp=new jFILE(crc_manager.get_filename(i->file_number),"rb");
    else
      fp=open_file(crc_manager.get_filename(i->file_number),"rb");


    if (fp->open_failure())
    {
      printf("Ooch. Could not open file %s\n",crc_manager.get_filename(i->file_number));
      delete fp;
      exit(0);
    }

    last_offset=-1;
    last_dir=new spec_directory(fp);
    last_file=i->file_number;
  }
  if (i->offset!=last_offset)
  {
    fp->seek(i->offset,SEEK_SET);
    last_offset=i->offset;
  }
  used=1;
}

int CacheList::AllocId()
{
    if (prof_data)
    {
        the_game->show_help("new id allocated, cache profiling turned off\n");
        prof_uninit();
    }

    // See if we previously allocated an id, if so check the next spot in
    // the array otherwise we will have to scan the whole list for a free
    // id and possibly grow the list.
    int ret = last_registered + 1;
    if (ret >= total || list[ret].file_number < 0)
    {
        // Scan list for a free id
        CacheItem *ci = list;
        ret = -1;
        for (int i = 0; i < total && ret < 0; i++, ci++)
            if (ci->file_number < 0)
                ret = i;

        if (ret < 0) // if no free id then make list bigger
        {
            int add_size = 20;
            list = (CacheItem *)realloc(list,
                                  (sizeof(CacheItem) * (total + add_size)));
            for (int i = 0; i < add_size; i++)
            {
                list[total + i].file_number = -1; // mark new entries as new
                list[total + i].last_access = -1;
                list[total + i].data = NULL;
            }
            ret = total;
            // If new id's have been added, old prof_data size won't work
            if (prof_data)
            {
                free(prof_data);
                prof_data = NULL;
            }
            total += add_size;
        }
    }
    last_registered = ret;
    return ret;
}

int CacheList::reg_object(char const *filename, LObject *object,
                          int type, int rm_dups)
{
    // See if we got a object with a filename included. Otherwise,
    // it's a string.
    if (item_type(object) == L_CONS_CELL)
    {
        filename = lstring_value(lcar(object));
        object = lcdr(object);
    }

    return reg(filename, lstring_value(object), type, rm_dups);
}

extern int total_files_open;

int CacheList::reg(char const *filename, char const *name, int type, int rm_dups)
{
    int fn = crc_manager.get_filenumber(filename);
    int offset = 0;

    if (type == SPEC_EXTERN_SFX)
    {
        // If an extern sound effect then just make sure it's there. If sound
        // is disabled, ignore the load error, just pretend it's all OK.
        bFILE *check = open_file(filename, "rb");
        if (!check->open_failure())
        {
            char buf[4];
            check->read(buf, 4);
            if (memcmp(buf, "RIFF", 4))
            {
                printf("File %s is not a WAV file\n", filename);
                exit(0);
            }
        }
        else if (sound_avail)
        {
            printf("Unable to open file '%s' for reading\n", filename);
            exit(0);
        }
        delete check;
    }
    else
    {
        // If a classic spec item, look for it in the archive.
        spec_directory *sd = sd_cache.get_spec_directory(filename);

        if (!sd)
        {
            printf("Unable to open file %s for item %s\n", filename, name);
            exit(0);
        }

        spec_entry *se = NULL;
        if (type != -1)
            se = sd->find(name, type);
        if (!se)
            se = sd->find(name);
        if (!se)
        {
            printf("No such item %s in file %s\n", name, filename);
            exit(0);
        }

        if (type >= 0 && type != se->type &&
             ((type != SPEC_CHARACTER2 && type != SPEC_CHARACTER)
               || (se->type != SPEC_CHARACTER && se->type != SPEC_CHARACTER2)))
        {
            printf("Item %s of file %s should be type %s\n",
                   name, filename, spec_types[type]);
            exit(0);
        }

        type = se->type;
        offset = se->offset;
    }

    // Check whether there is another entry pointing to the same
    // file and offset, and return it as a shortcut.
    if (rm_dups)
    {
        for (int i = 0; i < total; i++)
            if (list[i].file_number == fn && list[i].offset == offset)
                return i;
    }

    int id = AllocId();

    CHECK(id < total && list[id].file_number < 0);

    list[id].file_number = fn;
    list[id].last_access = -1;
    list[id].data = NULL;
    list[id].offset = offset;
    list[id].type = type;

    return id;
}


void CacheList::normalize()
{
  int j;
  CacheItem *ci=list;
  last_access=-1;
  for (j=0; j<total; j++,ci++)
  {
    if (ci->last_access>=0)
      ci->last_access=ci->last_access>>16;        // shift everything over by 16
    if (ci->last_access>last_access)            //  and find new largest timestamp
      last_access=ci->last_access;
  }
  last_access++;
}

backtile *CacheList::backt(int id)
{
  CacheItem *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");

  if (me->last_access>=0)
  {
    touch(me);
    return (backtile *)me->data;
  }
  else
  {
    touch(me);
    locate(me);
    me->data=(void *)new backtile(fp);
    last_offset=fp->tell();
    return (backtile *)me->data;
  }
}


foretile *CacheList::foret(int id)
{
  CacheItem *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");

  if (me->last_access>=0)
  {
    touch(me);
    return (foretile *)me->data;
  }
  else
  {
    touch(me);
    locate(me);
    me->data=(void *)new foretile(fp);
    last_offset=fp->tell();
    return (foretile *)me->data;
  }
}

figure *CacheList::fig(int id)
{
  CacheItem *me=list+id;
//  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");
  if (me->last_access>=0)
  {
    touch(me);
    return (figure *)me->data;
  }
  else
  {
    touch(me);
    locate(me);
    me->data=(void *)new figure(fp,me->type);
     last_offset=fp->tell();
    return (figure *)me->data;
  }
}

image *CacheList::img(int id)
{
  CacheItem *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");
  if (me->last_access>=0)
  {
    touch(me);
    return (image *)me->data;
  }
  else
  {
    touch(me);                                           // hold me, feel me, be me!
    locate(me);
    image *im=new image(fp);
    me->data=(void *)im;
    last_offset=fp->tell();

    return (image *)me->data;
  }
}

sound_effect *CacheList::sfx(int id)
{
  CacheItem *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");
  if (me->last_access>=0)
  {
    touch(me);                                           // hold me, feel me, be me!
    return (sound_effect *)me->data;
  }
  else
  {
    touch(me);                                           // hold me, feel me, be me!
    char *fn=crc_manager.get_filename(me->file_number);
    me->data=(void *)new sound_effect(fn);
    return (sound_effect *)me->data;
  }
}


part_frame *CacheList::part(int id)
{
  CacheItem *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");
  if (me->last_access>=0)
  {
    touch(me);                                           // hold me, feel me, be me!
    return (part_frame *)me->data;
  }
  else
  {
    touch(me);
    locate(me);
    me->data=(void *)new part_frame(fp);
    last_offset=fp->tell();
    return (part_frame *)me->data;
  }
}


LObject *CacheList::lblock(int id)
{
  CacheItem *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");
  return (LObject *)me->data;
}

CacheList cache;

void CacheList::free_oldest()
{
  int32_t old_time = last_access;
  CacheItem *ci=list,*oldest=NULL;
  ful=1;

  for (int i = 0; i < total; i++, ci++)
  {
    if (ci->data && ci->last_access < old_time)
    {
      oldest = ci;
      old_time = ci->last_access;
    }
  }
  if (oldest)
  {
    dprintf("mem_maker : freeing %s\n",spec_types[oldest->type]);
    unmalloc(oldest);
  }
  else
  {
    close_graphics();
    printf("Out of memory, please remove any TSR's device drivers you can\n");
    exit(0);
  }
}


void CacheList::show_accessed()
{
  int old=last_access,new_old_accessed;
  CacheItem *ci,*new_old;

  do
  {
    new_old_accessed=-1;
    new_old=NULL;
    ci=list;
    for (int i=0; i<total; i++,ci++)
    {
      if (ci->last_access<old && ci->last_access>0 && ci->last_access>new_old_accessed)
      {
    new_old_accessed=ci->last_access;
        new_old=ci;
      }
    }
    if (new_old)
    {
      ci=new_old;
      old=ci->last_access;
      printf("type=(%20s) file=(%20s) access=(%6ld)\n",spec_types[ci->type],
         crc_manager.get_filename(ci->file_number),
         (long int)ci->last_access);
    }
  } while (new_old);
}

int CacheList::loaded(int id)
{
  CacheItem *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id");
  if (me->last_access>=0)
    return 1;
  else return 0;
}

char_tint *CacheList::ctint(int id)
{
  CacheItem *me=list+id;
  CONDITION(id<total && id>=0 && me->file_number>=0,"Bad id" && me->type==SPEC_PALETTE);
  if (me->last_access>=0)
  {
    touch(me);
    return (char_tint *)me->data;
  }
  else
  {
    touch(me);
    locate(me);
    me->data=(void *)new char_tint(fp);
    last_offset=fp->tell();
    return (char_tint *)me->data;
  }
}

