// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"


lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  rpcs *rlsrpc = new rpcs(0);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);

  const char *hname;
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlsrpc->port();
  id = host.str();
  
  pthread_mutex_init(&mutexsum,NULL);
  pthread_cond_init(&condition1,NULL);
  pthread_cond_init(&retryCondition,NULL);
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
  int ret = lock_protocol::OK;
  pthread_mutex_lock(&mutexsum);
  map<int, int>::iterator iter=lockCacheList.find(lid);
  if(iter != lockCacheList.end())
  {
	int state = iter->second;
	if(state == 0)
	{
		lockCacheList[lid] = 3;
		threadCount[lid] = 0;
		pthread_mutex_unlock(&mutexsum);
		int r;
		ret = cl->call(lock_protocol::acquire, lid, id, r);
		pthread_mutex_lock(&mutexsum);
		if(ret == rlock_protocol::OK)
		{
			lockCacheList[lid] = 2;
		}
	}
	if(state == 1 || state == 5)
	{
		iter->second = 2;
	}
	if(state == 2||state == 3)
	{		
		threadCount[lid]++;
		while(iter->second != 1)
		{
			pthread_cond_wait(&condition1,&mutexsum);
		}
		threadCount[lid]--;
		iter->second = 2;
	}
  }
  else
  {
    lockCacheList[lid] = 3;
	threadCount[lid] = 0;
	pthread_mutex_unlock(&mutexsum);
	int r;
	ret = cl->call(lock_protocol::acquire, lid, id, r);
	pthread_mutex_lock(&mutexsum);
	if(ret == rlock_protocol::OK)
	{
		lockCacheList[lid] = 2;
	}
  }
  
  pthread_mutex_unlock(&mutexsum);
  return lock_protocol::OK;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
  pthread_mutex_lock(&mutexsum);
  lockCacheList[lid] = 1;
  pthread_cond_signal(&condition1);
  if(threadCount[lid] == 0)
  {
	lockCacheList[lid] = 5;
	pthread_cond_signal(&condition1);
  }

  //cout<<id<<" released lock:"<<lid<<" and its state is:"<<lockCacheList[lid]<<endl;
  pthread_mutex_unlock(&mutexsum);
  return lock_protocol::OK;
}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid,int &)
{
//cout<<"server is revoking "<<id<<" for lockid:"<<lid<<endl;
	int ret = rlock_protocol::OK;
	pthread_mutex_lock(&mutexsum);
	
	while(lockCacheList[lid] != 5)
	{
		pthread_cond_wait(&condition1,&mutexsum);
	}
	//pthread_mutex_unlock(&mutexsum);
	lu->dorelease(lid);
	//pthread_mutex_lock(&mutexsum);
	lockCacheList[lid] = 0;
	pthread_mutex_unlock(&mutexsum);
	return ret;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, int &)
{
  int ret = rlock_protocol::OK;
  return ret;
}

void lock_release_user::dorelease(lock_protocol::lockid_t lid)
{
	ex_client->flush(lid);
}



