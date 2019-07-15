#include"Tools.hpp"
#include"MySQL.hpp"
//通用头部组织包含：首行、Date:,Connection:,Transfer-Encoding:,Etag:,Last-Modified;

class ResponseBasic{//响应基类
  public:
    ResponseBasic(int sock):_cli_sock(sock)
  {}
    virtual bool ProccessRun(RequestInfo& info)=0;//组织响应模块；
    virtual bool Response(RequestInfo& info)=0;//流程运行模块；
    virtual bool RspBody(RequestInfo& info)=0;//组建正文模块；
    void CommonHeader(RequestInfo& info){//最大程度上组建通用头部，减少代码冗余；
      _rsp_header=info._version+" "+info._err_code+" "+Tools::GetErr_exp(info._err_code)+"\r\n";
      _rsp_header+="Date: "+_date+"\r\n";
      _rsp_header+="Connection: close\r\n";
      if(info._version=="HTTP/1.1")
        _rsp_header+="ETag: "+_etag+"\r\n";
      _rsp_header+="Last-Modified: "+_lmod+"\r\n";
    } 
    bool InitResponse(RequestInfo& req_info){
      std::cerr<<"Initing "<<std::endl;
      SQL.InitSQL("localhost","root","199805","HttpSever");
      Tools::MakeETag(req_info._st.st_size,req_info._st.st_ino,req_info._st.st_mtime,_etag);
      time_t t = time(NULL);//Date;
      Tools::TimeToGMT(t,_date);
      Tools::TimeToGMT(req_info._st.st_mtime,_lmod);
      Tools::DigitToStr(req_info._st.st_size,_fsize);
      Tools::GetType(req_info._path_phys,_ftype);
      return true;
    }
  protected:
    bool SendData(const std::string &buf){
      if(send(_cli_sock,buf.c_str(),buf.size(),0)<0)
        return false;
      return true;
    }
    bool SendCData(const std::string &buf){
      if(buf.empty())
        SendData("0\r\n\r\n");
      std::stringstream ss;
      ss<<std::hex<<buf.size()<<"\r\n";
      SendData(ss.str());
      ss.clear();
      SendData(buf);
      SendData("\r\n");
      return true;
    }
  protected:
    mySQL SQL;
    int _cli_sock;
    std::string _rsp_header;
    std::string _rsp_body;
    std::string _end;
    std::string _etag;//ETag字段用于唯一标识文件是否被修改
    std::string _lmod;//最后一次修改时间；
    std::string _date;//系统当前时间；
    std::string _fsize;//文件大小；
    std::string _ftype;//文件类型；
    std::string _cont_len;//正文长度；
};
class Err_Send : public ResponseBasic{//错误码发送衍生类
  public:
    Err_Send(int socket):ResponseBasic(socket){}
    virtual bool RspBody(RequestInfo& info)override{
      _rsp_body="<html><body><h1>"+info._err_code+"<h1></body></html>";
      return true;
    }
    virtual bool ProccessRun(RequestInfo& info)override{
      CommonHeader(info);
      Tools::DigitToStr(_rsp_body.size(),_fsize);
      _rsp_header+="Content-Length: "+_fsize+"\r\n\r\n";
      RspBody(info);
      std::cerr<<"Response:"<<std::endl<<_rsp_header<<std::endl;
      SendData(_rsp_header);
      std::cerr<<_rsp_body<<std::endl;
      SendCData(_rsp_body);
      SendCData(_end);
      return true;
    }
    virtual bool Response(RequestInfo& info)override{
      std::cerr<<"Err Response  "<<info._err_code<<std::endl;
      InitResponse(info);
      return ProccessRun(info);
    }
};


class File_List : public ResponseBasic{//文件目录衍生类
  public:
    File_List(int socket):ResponseBasic(socket){}
    static int filt(const struct dirent* dir){//scandir的过滤策略；
      if(strcmp(dir->d_name,".")==0)
        return 0;//标识不添加到列表；
      return 1;//添加；
    }
    virtual bool RspBody(RequestInfo& info)override{
      _rsp_body = "<html><head>";//前置说明；
      _rsp_body += "<title>Home/Catalog</title>";
      _rsp_body +="<meta charset = 'utf-8'>";
      _rsp_body +="</head><body>";
      _rsp_body +="<h1>[Path]:"+info._path_info+"</h1>";
      _rsp_body +="<form action='/upload' method='POST' enctype='multipart/form-data'>";
      _rsp_body +="<input type='file' name='FileUpLoad' />";
      _rsp_body +="<input type='submit' value='上传' / >";
      _rsp_body +="</form>";
      _rsp_body +="<hr /><ol>";
      //获取目录下每一个文件，组织处html信息，chunked传输;
      std::cerr<<_rsp_body<<std::endl;
      SendCData(_rsp_body);

      std::string file_html;
      //struct dirent **p_dirent = NULL;
      //int num = scandir(info._path_phys.c_str(),&p_dirent,filt,alphasort);//过滤规则：不做过滤，默认字母排序
      SQL.Select("root");
      //int num=SQL.Data.size();
      for(auto it:SQL.Data){
        file_html+= "<li>";//有序列表元素
        //std::string path = info._path_phys;
        //std::string file = path+"/"+p_dirent[i]->d_name;
        //struct stat st;
        //if(stat(file.c_str(),&st)<0)//获取文件信息；
          //continue;
        std::string mtime=it.find("add_time")->second;
        std::string mime=it.find("Mime")->second;
        std::string fsize=it.find("Bytes")->second;
        //std::cout<<"st_mtime:"<<st.st_mtime<<"-"<<"st_mime:"<<p_dirent[i]->d_name<<"-"<<"st_fsize:"<<st.st_size<<std::endl;
       //Tools::TimeToGMT(st.st_mtime,mtime);
        //Tools::GetType(p_dirent[i]->d_name,mime);
        //Tools::DigitToStr(st.st_size,fsize);
        file_html+="<strong><a href='"+ info._path_info;
        file_html+=it.find("Name")->second;
        file_html+="'>";
        file_html+=it.find("Name")->second;
        file_html+= "</a></strong>";
        file_html+="<br/ ><small>";
        file_html+="Modf: "+mtime+"<br />";
        file_html+=mime+"  Size:"+fsize+"B";
        file_html+="<br /><br /></small></li>";
      }
      std::cerr<<file_html<<std::endl;
      SendCData(file_html);
      _rsp_body = "</ol><hr /></body></html>";
      return true;
    }
    virtual bool ProccessRun(RequestInfo& info)override{
      CommonHeader(info);
      _rsp_header+="Transfer-Encoding: chunked\r\n";
      _rsp_header+="Content-Type: text/html;charset=UTF-8\r\n\r\n";
      std::cerr<<_rsp_header<<std::endl;
      SendData(_rsp_header);
      RspBody(info);
      std::cerr<<_rsp_body<<std::endl;
      SendCData(_rsp_body);
      SendCData(_end);
      return true;
    }
    virtual bool Response(RequestInfo& info)override{
      std::cerr<<"Reaponse:"<<std::endl;
      InitResponse(info);
      return ProccessRun(info);
    }
};

class CGI_Upload : public ResponseBasic{//上传文件衍生类
  public:
    CGI_Upload(int socket):ResponseBasic(socket){}
    virtual bool RspBody(RequestInfo& info)override{
      return true;
    }
    virtual bool ProccessRun(RequestInfo& info)override{
      int in[2],out[2];
      if(pipe(in)||pipe(out)){
        info._err_code="500";
        return false;
      }
      int pid = fork();
      if(pid<0){
        info._err_code = "500";
        return false;
      }else if(pid==0){
        setenv("METHOD",info._method.c_str(),1);
        setenv("VERSION",info._version.c_str(),1);
        setenv("PATH_INFO",info._path_info.c_str(),1);
        setenv("QUERY_STRING",info._query_string.c_str(),1);
        for(auto it=info.hd_list.begin();it!=info.hd_list.end();it++){
          setenv(it->first.c_str(),it->second.c_str(),1);
        }
        close(in[1]);
        close(out[0]);
        dup2(in[0],0);//通过标准输入输出向父进程接收和发送数据
        dup2(out[1],1);
        execl(info._path_phys.c_str(),info._path_phys.c_str(),NULL);
        exit(0);
      }
      close(in[0]);
      close(out[1]);
      //1.向子进程通过匿名管道发数据；
      auto it = info.hd_list.find("Content-Length");
      if(it!=info.hd_list.end()){
        char buf[MAX_BUFF]={0};
        long content_len = Tools::StrToDigit(it->second);
        int tlen=0;
        while(tlen<content_len){
          int len=MAX_BUFF>(content_len-tlen)?(content_len):MAX_BUFF;
          int rlen=recv(_cli_sock,buf,len,0);
          if(rlen<0){
            return false;
          }
          if(write(in[1],buf,rlen)<0)
            return false;
          tlen+=rlen;
        }
      }
      CommonHeader(info);
      _rsp_header+="Content-Type: text/html\r\n\r\n";
      SendData(_rsp_header);
      while(1){
        char buf[MAX_BUFF]={0};
        int rlen=read(out[0],buf,MAX_BUFF);
        if(rlen==0){
          break;
        }
        std::cerr << "recv child buf:"<<buf << std::endl;
        send(_cli_sock,buf,rlen,0);
      }
      close(in[1]);
      close(out[0]);
      Shmat shm;//获取子进程传来的文件名；
      shm.shmid=shm.getShm(1024);
      shm.Recv();
      filepath=shm.Data;
      shm.destroyShm(shm.shmid);
      //std::cerr<<filename<<std::endl;
      return true;
    }
    virtual bool Response(RequestInfo& info)override{
      InitResponse(info);
      ProccessRun(info);
      if(stat(filepath.c_str(),&info._st)<0){
        std::cerr<<"Update Filestat Failed!"<<std::endl;
        exit(-2);
      }
      Tools::TimeToGMT(info._st.st_mtime,_lmod);//将该文件信息放入数据库中；
      Tools::DigitToStr(info._st.st_size,_fsize);
      char tmp[MAX_BUFF]={0};
      realpath(filepath.c_str(),tmp);
      fileRealpath=tmp;
      Tools::GetType(fileRealpath,_ftype);
      std::string filename=filepath.substr(filepath.find_last_of('/')+1);
      std::cerr<<filename<<std::endl;
      std::string ins="'"+filename+"','"+_ftype+"','"+_fsize+"','"+_lmod+"','"+filepath+"'";
      SQL.Insert(ins,"root");
      return true;
    }
  protected:
    std::string filepath;
    std::string fileRealpath;
};

class File_Download : public ResponseBasic{//文件下载衍生类
  public:
    File_Download(int socket):ResponseBasic(socket){}
    virtual bool RspBody(RequestInfo& info)override{
      int fd = open(info._path_phys.c_str(),O_RDONLY);
      if(fd<0){
        info._err_code="400";
        return false;
      }
      char tmp[MAX_BUFF];
      int rlen =0;
      while((rlen=read(fd,tmp,MAX_BUFF))>0){
        //std::cerr<<tmp<<std::endl;
        send(_cli_sock,tmp,rlen,0);
      }
      close(fd);
      //_rsp_body="<html><body><h1>DownLoad Seccess!</h1></body></html><br/>";
      //SendData(_rsp_body);
      return true;
    }
    virtual bool ProccessRun(RequestInfo& info)override{
      CommonHeader(info);
      _rsp_header+="Content-Length: "+_fsize+"\r\n";
      _rsp_header+="Content-Type: "+_ftype+"\r\n";
      _rsp_header+="Accept-Ranges: bytes\r\n\r\n";
      std::cerr<<_rsp_header<<std::endl;
      SendData(_rsp_header);
      return RspBody(info);
    }
    virtual bool Response(RequestInfo& info)override{
      InitResponse(info);
      return ProccessRun(info);
    }
};

class Part_Download : public ResponseBasic{
  public:
    Part_Download(int socket):ResponseBasic(socket){}
    virtual bool RspBody(RequestInfo& info)override{
      int fd=open(info._path_phys.c_str(),O_RDONLY);
      if(fd<0){
        info._err_code="400";
        return false;
      }
      lseek(fd,start,SEEK_SET);
      int64_t blen=end-start+1,rlen,flen=0;
      char tmp[MAX_BUFF]={0};
      while((rlen=read(fd,tmp,MAX_BUFF))>0){
        if(rlen+flen>blen){
          send(_cli_sock,tmp,blen-flen,0);
          break;
        }else{
          flen+=rlen;
          send(_cli_sock,tmp,rlen,0);
        }
      }
      close(fd);
      return true;
    }
//通用头部组织包含：首行、Date:,Connection:,Transfer-Encoding:,Etag:,Last-Modified;
    virtual bool ProccessRun(RequestInfo& info)override{
      CommonHeader(info);
      _rsp_header+="Content-Type: "+_ftype+"\r\n";
      std::string len;
      Tools::DigitToStr(end-start+1,len);
      _rsp_header+="Content-Range: bytes "+Tools::DigitToStr(start)+"-"+Tools::DigitToStr(end)+"/"+_fsize+"\r\n";
      _rsp_header+="Content-Length: "+len+"\r\n";
      _rsp_header+="Accept-Ranges: bytes\r\n\r\n";
      SendData(_rsp_header);
      return RspBody(info);
    }
    virtual bool Response(RequestInfo& info)override{
      InitResponse(info);
      info._err_code="206";
      for(int i=0;i<info._part;i++){
        if(ProccessRun(info)==false)
          return false;
      }
      info._count=0;
      return true;
    }
    void GetRange(RequestInfo& info){
      std::string range=info._part_list[info._count];
      info._count++;
      size_t pos=range.find("-");
      if(pos==0){
        end=Tools::StrToDigit(_fsize)-1;
        start=end-Tools::StrToDigit(range.substr(pos+1));
      }else if(pos==range.size()-1){
        end=Tools::StrToDigit(_fsize)-1;
        start=Tools::StrToDigit(range.substr(0,range.size()-1));
      }else{
        start=Tools::StrToDigit(range.substr(0,pos));
        end=Tools::StrToDigit(range.substr(pos+1));
      }
    }
  protected:
      std::string range;
      int start=0,end=0;
};
