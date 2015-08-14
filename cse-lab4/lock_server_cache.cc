// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"


lock_server_cache::lock_server_cache()
{
	pthread_mutex_init(&mutexsum,NULL);
	pthread_cond_init(&condition,NULL);
}


int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, 
                               int &)
{
  pthread_mutex_lock(&mutexsum);
  lock_protocol::status ret = lock_protocol::OK;
  map<int,int>::iterator p = serverLockMap.find(lid);
  clientPortMap[lid].push_back(id);
  if(p!=serverLockMap.end())
  {
	int size = clientPortMap[lid].size();
	if(size > 1)
	{
		handle h(clientPortMap[lid][size-2]);
		rpcc *cl = h.safebind();
		if(cl)
		{
			int ret = lock_protocol::OK;
			int r=0;
			pthread_mutex_unlock(&mutexsum);
			//cout<<id<<" is waiting for "<<clientPortMap[lid][size-2]<<" to be revoked!"<<endl;
			ret = cl->call(rlock_protocol::revoke,lid,r);
			//cout<<clientPortMap[lid][size-2]<<"has return the lock:"<<lid<<"to the server"<<endl;
			//cout<<id<<"has got the lock:"<<lid<<endl;
			pthread_mutex_lock(&mutexsum);
			clientPortMap[lid].erase(clientPortMap[lid].begin());
		}
	}
  }
  else
  {
	serverLockMap[lid] = 1;
  }
  pthread_mutex_unlock(&mutexsum);
  return ret;
}

int 
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, 
         int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  return ret;
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  tprintf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}

