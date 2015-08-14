// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <algorithm>
using namespace std;

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);
  release_user = new lock_release_user();
  release_user->ex_client = ec;
  mylockclient= new lock_client_cache(lock_dst,release_user);
  inum root = 0x00000001;
  mylockclient->acquire(root);
  ec->put(root,"");
  mylockclient->release(root);
  srand((unsigned int)(time(NULL)));
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string
yfs_client::filename(inum inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}

bool
yfs_client::isdir(inum inum)
{
  return ! isfile(inum);
}

int
yfs_client::getfileinfo(inum inum, fileinfo &fin)
{
  int r = OK;
  mylockclient->acquire(inum);
  //printf("getfile %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK)
  {
    r = -1;
    //goto release;
  }
  else
  {
	fin.atime = a.atime;
	fin.mtime = a.mtime;
	fin.ctime = a.ctime;
	fin.size = a.size;
  }
  //printf("getfile %016llx -> sz %llu\n", inum, fin.size);
  mylockclient->release(inum);
  return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;
  mylockclient->acquire(inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) 
  {
    r = -1;
    //goto release;
  }
  else
  {
	din.atime = a.atime;
	din.mtime = a.mtime;
	din.ctime = a.ctime;
  }
 //release:
  mylockclient->release(inum);
  return r;
}

int yfs_client::createfile(inum parent,inum file,const char* fname)
{
	int ret;
	mylockclient->acquire(parent);
	mylockclient->acquire(file);
	string originalbuf;//to store the new file's information
	inum existid=-1;
	ec->get(parent,originalbuf);
	if(fileindir(parent,fname,existid) == extent_protocol::OK)
	{
		if(existid != -1)
		{
			ret = -2;
		}
		else//the file doesn't exist in parent
		{
			string buf;
			string add(fname);
			buf = originalbuf + filename(file) + " " + add + " ";
			if(ec->put(parent,buf) == extent_protocol::OK && ec->put(file,"") == extent_protocol::OK)
			{
				ret =  1;
			}
		}
	}
	mylockclient->release(file);
	mylockclient->release(parent);
	return ret;
}

std::vector<std::string> yfs_client::split(std::string str,std::string pattern)
{
	vector<string> result;
	string::size_type pos;
	str+=pattern;
	unsigned int size = str.size();

	for(unsigned int i=0;i<size;i++)
	{
		pos = str.find(pattern,i);
		if(pos<size)
		{
			string s = str.substr(i,pos-i);
			if(s.empty() == false)
			{
				result.push_back(s);
			}
			i = pos + pattern.size() - 1;
		}
	}
	return result;
}

int yfs_client::fileindir(inum dir,const char* filename,inum &fileid)
{
	
	fileid = -1;
	string fname(filename);
	string dirbuf;
	ec->get(dir,dirbuf);
	
	if(dirbuf != "")
	{
	vector<string> splitstr = split(dirbuf," ");
	for(int i=0;i!=splitstr.size();i++)
	{
		if(i%2==1 && splitstr[i] == fname)
		{
			
			fileid = n2i(splitstr[i-1]);
			
		}
	}
	}
	return extent_protocol::OK;
}
int yfs_client::fileindir2(inum dir,const char* filename,inum &fileid)
{
	mylockclient->acquire(dir);
	fileid = -1;
	string fname(filename);
	string dirbuf;
	ec->get(dir,dirbuf);
	
	if(dirbuf != "")
	{
	vector<string> splitstr = split(dirbuf," ");
	for(int i=0;i!=splitstr.size();i++)
	{
		if(i%2==1 && splitstr[i] == fname)
		{
			
			fileid = n2i(splitstr[i-1]);
			break;
		}
	}
	}
	mylockclient->release(dir);
	return extent_protocol::OK;
}

int yfs_client::getallitembydir(inum dirid,map<inum,string> &result)
{
	mylockclient->acquire(dirid);
	int r = OK;
	if(isdir(dirid) == false)
	{
		r = IOERR;
	}
	else
	{
		string dirbuf;
		if(ec->get(dirid,dirbuf) != extent_protocol::OK)
		{
			r = IOERR;
		}
		else
		{
			if(dirbuf != "")
			{
			vector<string> split_str = split(dirbuf," ");
			if(split_str.size() == 0)
			{
				r = IOERR;
				mylockclient->release(dirid);
				return r;
			}
			inum item_id;
			string item_name;
			for(unsigned int i=0;i!=split_str.size();i++)
			{
				if(i%2 == 0)
				{
					item_id = n2i(split_str[i]);
				}
				else
				{
					item_name = split_str[i];
					result[item_id] = item_name;
				}
			}
			}
			else
			{
				mylockclient->release(dirid);
				return -1;
			}
		}
	}
	mylockclient->release(dirid);
	return r;
}

void yfs_client::setnewattr(inum fileid,struct stat newattr)
{
	mylockclient->acquire(fileid);
	string oldbuf;
	if(ec->get(fileid,oldbuf) == extent_protocol::OK)
	{
		unsigned int oldsize=0;
		extent_protocol::attr a;
	 	if(ec->getattr(fileid,a) == extent_protocol::OK)
		{
			oldsize = a.size;
		}
		if(oldsize>=newattr.st_size)
		{
			oldbuf = oldbuf.substr(0,newattr.st_size);
			ec->put(fileid,oldbuf);
		}
		if(oldsize<newattr.st_size)
		{
			string null_str(newattr.st_size-oldsize,'\0');
			oldbuf += null_str;
			ec->put(fileid,oldbuf);
		}
	}
	mylockclient->release(fileid);
}

void yfs_client::readfile(inum fileid,string &buf,off_t off,size_t size)
{
	mylockclient->acquire(fileid);
	unsigned int filebufsize;
	extent_protocol::attr a;
	ec->getattr(fileid,a);
	filebufsize = a.size;
	string oldbuf;
	ec->get(fileid,oldbuf);
	if(off < filebufsize)
	{
		buf = oldbuf.substr(off,size);
	}
	else
	{
		buf="";
	}
	mylockclient->release(fileid);
}

void yfs_client::writefile(inum fileid,const char* buf,off_t off,size_t size)
{
	mylockclient->acquire(fileid);
	unsigned int oldsize;
	extent_protocol::attr a;
	ec->getattr(fileid,a);
	//oldsize = a.size;
	string oldbuf;
	ec->get(fileid,oldbuf);
	oldsize = oldbuf.size();
	if(off<oldsize)
	{
		//string remain = oldbuf.substr(0,off);
		string modifystr(buf,size);
		string newbuf;
		if(off+size < oldsize)
		{
			for(int i=off;i!=off+size;i++)
			{
				oldbuf[i] = modifystr[i-off];
			}
			newbuf = oldbuf;
		}
		else
		{
			string remain = oldbuf.substr(0,off);
			string addstr(buf,size);
			newbuf = remain + addstr;
		}
		ec->put(fileid,newbuf);
	}
	else
	{
		oldbuf.resize(off,'\0');
		string addstr(buf,size);
		string newbuf = oldbuf + addstr;
		ec->put(fileid,newbuf);
	}
	mylockclient->release(fileid);
}

int yfs_client::createdir(inum parent,inum dir,const char* dirname)
{
	mylockclient->acquire(parent);
	mylockclient->acquire(dir);
	int result = -1;
		inum existid = -1;
		fileindir(parent,dirname,existid);
		if(existid != -1)//already exist
		{
			result =  -2;
		}
		else
		{
			string originalbuf;
			ec->get(parent,originalbuf);
			string buf;
			string add(dirname);
			buf = originalbuf + filename(dir) + " " + add + " ";
			if(ec->put(parent,buf) == extent_protocol::OK&&ec->put(dir,"") == extent_protocol::OK)
			{
				result =  1;
			}
		}
	mylockclient->release(dir);
	mylockclient->release(parent);
	return result;
}

int yfs_client::unlinkfile(inum parent,const char* fname)
{
	mylockclient->acquire(parent);
	inum fileid = -1;
	fileindir(parent,fname,fileid);
	if(fileid == -1)//not exist
	{
		mylockclient->release(parent);
		return 0;
	}
	mylockclient->acquire(fileid);
	string parentdirbuf;
	ec->get(parent,parentdirbuf);
	vector<string> splitresult = split(parentdirbuf," ");
	string parentdirbuf_new="";
	for(int i=0;i!=splitresult.size();i++)
	{
		if(i%2==0&&splitresult[i] != filename(fileid))
		{
			parentdirbuf_new +=(splitresult[i]+" "+splitresult[i+1] + " ");
		}
	}
	if(ec->put(parent,parentdirbuf_new) == extent_protocol::OK &&ec->remove(fileid)== extent_protocol::OK)
	{
		mylockclient->release(fileid);
		mylockclient->release(parent);
		return 1;
	}
	mylockclient->release(fileid);
	mylockclient->release(parent);
	return 1;
}
