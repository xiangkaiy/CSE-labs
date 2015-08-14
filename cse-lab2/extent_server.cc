// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

extent_server::extent_server() 
{
  pthread_mutex_init(&mutexsum,NULL);
}


int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  pthread_mutex_lock(&mutexsum);
  file_map[id] = buf;//fileattr_map
  extent_protocol::attr putattr;
  putattr.ctime = putattr.mtime = time(0);
  putattr.size = buf.size();
  fileattr_map[id] = putattr; 
  pthread_mutex_unlock(&mutexsum);
  cout<<"now fileordir id: "<<id<<"buf: "<<buf<<endl;
  return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  pthread_mutex_lock(&mutexsum);
  buf = file_map[id];
  if(fileattr_map.count(id) == 1)
  {
    fileattr_map[id].atime = time(0);
  }
  else
  {
    return extent_protocol::NOENT;
  }
  pthread_mutex_unlock(&mutexsum);
  return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  // You fill this in for Lab 2.
  // You replace this with a real implementation. We send a phony response
  // for now because it's difficult to get FUSE to do anything (including
  // unmount) if getattr fails.
  pthread_mutex_lock(&mutexsum);
  a = fileattr_map[id];
  pthread_mutex_unlock(&mutexsum);
  return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  pthread_mutex_lock(&mutexsum);
  file_map.erase(id);
  fileattr_map.erase(id);
  pthread_mutex_unlock(&mutexsum);
  return extent_protocol::OK;
}

