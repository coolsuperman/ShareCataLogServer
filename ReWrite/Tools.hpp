#pragma once
#include<sys/types.h>
#include<iostream>
#include<sys/socket.h>
#include<string>
#include<arpa/inet.h>
#include<pthread.h>
#include<queue>
#include<sys/signal.h>
#include<unordered_map>
#include<fcntl.h>
#include<dirent.h>
#include<sys/stat.h>
#include<sstream>
#include<cstring>
#include<unistd.h>
#include<vector>

const int MAX_BUFF=4096;
const std::string FLODER="Web";
const int MAX_PATH=256;
const int MAX_HTTPHD = 4096;

std::unordered_map<std::string,std::string> err_exp={
  {"200","OK"},
  {"400","Bad Request"},
  {"403","Forbidden"},
  {"404","Not Found"},
  {"405","Method Not Allowed"},
  {"413","Request Entity Too Large"},
  {"500","Internal Server Error"},
};

std::unordered_map<std::string,std::string> type={
  {"txt","text/plain"},
  {"html","text/html"},
  {"htm","text/html"},
  {"jpg","image/jpeg"},
  {"png","image/png"},
  {"zip","application/zip"},
  {"mp3","audio/mpeg"},
  {"unknow","application/octet-stream"}
};


class RequestInfo{
  public:
    std::string _method;
    std::string _version;
    std::string _path_info;
    std::string _path_phys;
    std::string _query_string;
    std::unordered_map<std::string,std::string> hd_list;//存储键值对；
    struct stat _st;//获取文件信息；
    std::string _err_code="200";
  public:
    void SetError(std::string err){
      _err_code = err;
    }
    bool RequestIsCGI(){
      if((_method=="GET"&&(_query_string.size()!=0))||(_method=="POST"))
        return true;
      return false;
    }
};

class Tools{
  public:
    static int Split(std::string& src,const std::string search,std::vector<std::string>& list) {
      int num = 0;
      size_t start = 0;
      size_t end = 0;
      while(start<src.size()){
        end = src.find(search,start);
        if(end==std::string::npos)
          break;
        list.push_back(src.substr(start,end-start));
        num++;
        start = end+search.size();
      }
      if(start<src.size()){
        list.push_back(src.substr(start));
        num++;
      }
      return num;
    }
    static std::string GetErr_exp(std::string code){
      auto pos = err_exp.find(code);
      if(pos==err_exp.end())
        return "Unknow Error";
      return pos->second;
    }
    static bool IsDir(RequestInfo& info){
      if(info._st.st_mode & S_IFDIR){
        if(info._path_info.back()!='/')
          info._path_info.push_back('/');
        return true;
      }
      return false;
    }
    static void TimeToGMT(time_t t,std::string& GMT){//转化为格林威治时间
      struct tm* mt = gmtime(&t);
      char tmp[128]={0};
      int len = strftime(tmp,127,"%a, %d %b %Y %H:%M:%S GMT",mt);
      GMT.assign(tmp,len);
    }
    static int64_t StrToDigit(std::string& str){
      int64_t num;
      std::stringstream ss;
      ss<<str;
      ss>>num;
      return num;
    }
    static void DigitToStr(int64_t num,std::string& str){
      std::stringstream ss;
      ss<<num;
      str=ss.str();
    }
    static void MakeETag(int64_t size,int64_t in,int64_t lmodf,std::string& etag){
      std::stringstream ss;
      ss<<"\""<<std::hex<<in<<"-"<<std::hex<<size<<"-"<<std::hex<<lmodf<<"\"";
      etag = ss.str();
    }
    static void GetType(const std::string& file,std::string &mime){
      size_t pos=file.find_last_of(".");
      if(pos==std::string::npos){
        mime=type["unknow"];
        return;
      }
      std::string suffix = file.substr(pos+1);
      auto it = type.find(suffix);
      if(it==type.end()){
        mime=type["unknow"];
        return;
      }
      mime = it->second;
    }
};
