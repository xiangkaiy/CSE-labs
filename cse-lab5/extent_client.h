// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include "extent_protocol.h"
#include "rpc.h"
#include <map>
using namespace std;

class extent_client {
 private:
  rpcc *cl;
  pthread_mutex_t mutexsum;
  pthread_cond_t condition;
 public:
  map<extent_protocol::extentid_t,string> extent_cache;//to cache the extent
  map<extent_protocol::extentid_t,extent_protocol::attr> attr_cache;//to cache the attr;
  //map<extent_protocol::extentid_t,int> extent_modify;//to show the extent is need to write back or not; 1 is need 0 is not
  extent_client(std::string dst);

  extent_protocol::status get(extent_protocol::extentid_t eid, 
			      std::string &buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				  extent_protocol::attr &a);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status remove(extent_protocol::extentid_t eid);
  extent_protocol::status flush(extent_protocol::extentid_t eid);
};

#endif 

