// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0)
{
  pthread_mutex_init(&mutexsum,NULL);
  pthread_cond_init(&condition,NULL);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  map<int, int>::iterator iter;
  
  pthread_mutex_lock(&mutexsum);
  iter = lock_map.find(lid);
  
  if(lock_map.end() != iter)
  { 
    if(iter->second == 1)
    {
      while(iter->second == 1)  
      {
        pthread_cond_wait(&condition,&mutexsum);
      }
    }
    
    iter->second = 1;
  } else {
    lock_map.insert(map<int, int>::value_type(lid, 1));
    iter = lock_map.find(lid);
  }
  pthread_mutex_unlock(&mutexsum);
  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  map<int, int>::iterator iter;
  pthread_mutex_lock(&mutexsum);

  iter = lock_map.find(lid);
  if(lock_map.end() != iter)
  {
    iter->second = 0;
    pthread_cond_signal(&condition);
  }
  pthread_mutex_unlock(&mutexsum);
  return ret;
}
