#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>
#include <map>
#include <stdlib.h>
#include <unistd.h>
using namespace std;

class yfs_client {
  extent_client *ec;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:
  static std::string filename(inum);
  static inum n2i(std::string);
 public:

  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);
  bool createfile(inum parent,inum file,const char* filename);
  int fileindir(inum dir,const char* filename,int &fileid);
  std::vector<std::string> split(std::string str,std::string pattern);
  /*bool getfileidbyname(inum dirid,const char* filename,inum &fileid);*/
  int getallitembydir(inum dirid,map<inum,string> &result);
  void setnewattr(inum fileid,struct stat newattr);
  void readfile(inum fileid,string &buf,off_t off,size_t size);
  void writefile(inum,const char*,off_t,size_t);
};

#endif 
