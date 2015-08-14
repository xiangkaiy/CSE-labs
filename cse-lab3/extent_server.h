// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"
#include <pthread.h>
#include "lock_protocol.h"
using namespace std;

class extent_server {
 protected:
  enum lock_state{unlocked=0,locked};
  pthread_mutex_t mutexsum;
  pthread_cond_t condition;
 public:
  extent_server();
  map<extent_protocol::extentid_t,string> file_map;
  map<extent_protocol::extentid_t,extent_protocol::attr> fileattr_map;
  map<extent_protocol::extentid_t,lock_state> lockmap;
  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
};

#endif 







