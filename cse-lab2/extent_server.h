// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"
#include <pthread.h>
using namespace std;

class extent_server {
 protected:
  pthread_mutex_t mutexsum;
 public:
  extent_server();
  map<extent_protocol::extentid_t,string> file_map;
  map<extent_protocol::extentid_t,extent_protocol::attr> fileattr_map;
  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
};

#endif 







