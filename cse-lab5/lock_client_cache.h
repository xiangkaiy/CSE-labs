// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"
#include <map>
#include "extent_client.h"
using namespace std;

// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 5.
class lock_release_user {
 public:
  extent_client *ex_client;
  void dorelease(lock_protocol::lockid_t);
  ~lock_release_user() {};
};

class lock_client_cache : public lock_client {
 private:
  class lock_release_user *lu;
  int rlock_port;
  std::string hostname;
  std::string id;
  
  pthread_mutex_t mutexsum;
  pthread_cond_t condition1;
  pthread_cond_t retryCondition;
 
  /*
  0: knows nothing about this lock
  1: client owns the lock and no thread has it
  2: client owns the lock and a thread has it
  3: the client is acquiring ownership
  4: the client is releasing ownership
  */
  map<int,int> lockCacheList;//key is the lockid and value is the lock's state,
  map<int,int> threadCount;//the number of thread that waiting for the lock
 public:
  lock_client_cache(std::string xdst, class lock_release_user *l = 0);
  virtual ~lock_client_cache() {};
  lock_protocol::status acquire(lock_protocol::lockid_t);
  lock_protocol::status release(lock_protocol::lockid_t);
  rlock_protocol::status revoke_handler(lock_protocol::lockid_t, 
                                        int &);
  rlock_protocol::status retry_handler(lock_protocol::lockid_t, 
                                       int &);
};


#endif
