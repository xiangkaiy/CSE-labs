#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>

#include <map>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"
using namespace std;

class lock_server_cache {
 private:
  int nacquire;
  pthread_mutex_t mutexsum;
  pthread_cond_t condition;
  
  //key is the lock's id and value is the state of the lock---
  //0:no client hold the lock
  //1:there is a client holding the lock,and there is no other client waiting for it
  //2:there is a client holding the lock,and there is a client waiting for it
  map<int,int> serverLockMap; 
  
  //the key is the lock's id,and the value is the port of the client that holding the lock
  map<int,vector<string> > clientPortMap;
 public:
  lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  int acquire(lock_protocol::lockid_t, std::string id, int &);
  int release(lock_protocol::lockid_t, std::string id, int &);
};

#endif
