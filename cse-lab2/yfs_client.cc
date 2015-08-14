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
  ec->put(0x00000001,"");
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
yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;

  printf("getfile %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
  printf("getfile %016llx -> sz %llu\n", inum, fin.size);

 release:

  return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;

  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  return r;
}

bool yfs_client::createfile(inum parent,inum file,const char* fname)
{
	string originalbuf;//to store the new file's information
	if(isdir(parent) == false || isfile(file) == false)
	{
		return false;
	}
	if(ec->get(parent,originalbuf) != extent_protocol::OK)
	{
		return false;
	}
	string buf;
	string add(fname);
	if(originalbuf != "")
	{
		originalbuf += " ";
	}
	buf = originalbuf + filename(file) + " " + add;
	if(ec->put(parent,buf) == extent_protocol::OK)
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::vector<std::string> yfs_client::split(std::string str,std::string pattern)
{
	string::size_type pos;
	vector<string> result;
	str+=pattern;
	unsigned int size = str.size();

	for(unsigned int i=0;i<size;i++)
	{
		pos = str.find(pattern,i);
		if(pos<size)
		{
			string s = str.substr(i,pos-i);
			result.push_back(s);
			i = pos + pattern.size() - 1;
		}
	}
	return result;
}

int yfs_client::fileindir(inum dir,const char* filename,int &fileid)
{
	fileid = -1;
	string fname(filename);
	string dirbuf;
	if(ec->get(dir,dirbuf) != extent_protocol::OK)
	{
		return extent_protocol::IOERR;
	}
	vector<string> splitstr = split(dirbuf," ");
	bool flag = false;
	for(int i=0;i!=splitstr.size();i++)
	{
		if(splitstr[i] == fname)
		{
			flag = true;
			fileid = n2i(splitstr[i-1]);
			return extent_protocol::OK;
		}
	}
 	
	if(flag == false)
	{
		return extent_protocol::NOENT;
	}
}

int yfs_client::getallitembydir(inum dirid,map<inum,string> &result)
{
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
			vector<string> split_str = split(dirbuf," ");
			if(split_str.size() == 0)
			{
				r = IOERR;
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
	}
	return r;
}

void yfs_client::setnewattr(inum fileid,struct stat newattr)
{
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
			for(int i=0;i!=newattr.st_size-oldsize;i++)
			{
				oldbuf += "\0";
			}
			ec->put(fileid,oldbuf);
		}
	}
	
}

void yfs_client::readfile(inum fileid,string &buf,off_t off,size_t size)
{
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
}

void yfs_client::writefile(inum fileid,const char* buf,off_t off,size_t size)
{
	unsigned int oldsize;
	extent_protocol::attr a;
	ec->getattr(fileid,a);
	oldsize = a.size;
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
}
