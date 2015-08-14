// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "lock_client.h"
// The calls assume that the caller holds a lock on the extent

extent_client::extent_client(std::string dst)
{
	pthread_mutex_init(&mutexsum,NULL);
	pthread_cond_init(&condition,NULL);
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
	pthread_mutex_lock(&mutexsum);
  extent_protocol::status ret = extent_protocol::OK;
  int count = extent_cache.count(eid);
  if(count == 0)
  {
	//pthread_mutex_unlock(&mutexsum);
	ret = cl->call(extent_protocol::get, eid, buf);
	extent_protocol::attr putattr;
	int result = cl->call(extent_protocol::getattr,eid,putattr);
	//pthread_mutex_lock(&mutexsum);
	extent_cache[eid] = buf;
	attr_cache[eid] = putattr;
	//extent_modify[eid] = 0;
  }
  if(count == 1)
  {
	buf = extent_cache[eid];
	attr_cache[eid].atime = time(0);
  }
  //cout<<"client get extent_id:"<<eid<<" and data is:"<<buf<<" time:"<<time(0)<<endl;
  //print_extent_cache(extent_cache);
  pthread_mutex_unlock(&mutexsum);
  return ret;
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, extent_protocol::attr &attr)
{
	pthread_mutex_lock(&mutexsum);
  extent_protocol::status ret = extent_protocol::OK;
  int count = attr_cache.count(eid);
  if(count == 0)
  {
	//pthread_mutex_unlock(&mutexsum);
	ret = cl->call(extent_protocol::getattr, eid, attr);
	string buf;
	int result = cl->call(extent_protocol::get,eid,buf);
	//pthread_mutex_lock(&mutexsum);
	extent_cache[eid] = buf;
	attr_cache[eid] = attr;
	//extent_modify[eid] = 0;
  }
  if(count == 1)
  {
	attr = attr_cache[eid];
  }
 // cout<<"client get attr of extent_id:"<<eid<<" time:"<<time(0)<<endl;
  //print_extent_cache(extent_cache);
  pthread_mutex_unlock(&mutexsum);
  return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
	pthread_mutex_lock(&mutexsum);
	//cout<<"client put extent_id:"<<eid<<" and data is:"<<buf<<" time:"<<time(0)<<endl;
	//print_extent_cache(extent_cache);
  extent_protocol::status ret = extent_protocol::OK;
  int r;
  //ret = cl->call(extent_protocol::put, eid, buf, r);
  extent_cache[eid] = buf;
  
  //to add the attr of the extent
  extent_protocol::attr putattr;
  putattr.ctime = putattr.mtime = time(0);
  putattr.size = buf.size();
  attr_cache[eid] = putattr;
  //extent_modify[eid] = 1;
  pthread_mutex_unlock(&mutexsum);
  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
	pthread_mutex_lock(&mutexsum);
	//cout<<"client remove extent_id:"<<eid<<" time:"<<time(0)<<endl;
	//print_extent_cache(extent_cache);
  extent_protocol::status ret = extent_protocol::OK;
  //int r;
  //ret = cl->call(extent_protocol::remove, eid, r);
  attr_cache.erase(eid);
  extent_cache.erase(eid);
  //extent_modify[eid] = 1;
  pthread_mutex_unlock(&mutexsum);
  return ret;
}

extent_protocol::status
extent_client::flush(extent_protocol::extentid_t eid)
{
	pthread_mutex_lock(&mutexsum);
	
	extent_protocol::status ret = extent_protocol::OK;
	int p = extent_cache.count(eid);
	int attr_p = attr_cache.count(eid);
	
	int r;
	//cout<<eid<<" in extent count:"<<p<<endl;
	//cout<<eid<<" in attr count:"<<attr_p<<endl;
	if(p == 1)
	{
		//int need_writeback = extent_modify[eid];
		//if(need_writeback == 1)
		//{
			string buf = extent_cache[eid];// data to write back
			//cout<<"client flush extent_id:"<<eid<<" and data is:"<<buf<<" time:"<<time(0)<<endl;
			attr_cache.erase(eid);
			extent_cache.erase(eid);
			ret = cl->call(extent_protocol::put, eid, buf, r);
		//}
	}
	if(p == 0)
	{
		//cout<<"client flush extent_id(remove):"<<eid<<" time:"<<time(0)<<endl;
		//pthread_mutex_unlock(&mutexsum);
		ret = cl->call(extent_protocol::remove, eid, r);
	}
	pthread_mutex_unlock(&mutexsum);
	return ret;
}






