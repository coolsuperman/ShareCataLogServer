#include"HttpRequest.hpp"

enum _boundry_type{
  BOUNDRY_NO = 0,
  BOUNDRY_FIRST,
  BOUNDRY_MIDDLE,
  BOUNDRY_LAST,
  BOUNDRY_BAK//假设是Boundry
};

class Upload{
  private:
    int64_t content_len;
    int _file_fd;
    std::string _f_boundry;
    std::string _m_boundry;
    std::string _l_boundry;
  private:
    int MatchBoundry(char* buf,int blen,int* boundryPos){
      //----boundry;
      //first_boundry:------boundry;
      //middle_boundry:\r\n------boundry\r\n
      //last_boundry:\r\n------boundry--\r\n
      //从起始位置匹配first_boundry
      if(!memcmp(buf,_f_boundry.c_str(),_f_boundry.length()))
        return BOUNDRY_FIRST;
      for(int i = 0;i<blen;++i){
        //字符串剩余长度大于boundry的长度，则全部匹配
        if((size_t)(blen-i)>_m_boundry.size()){
          if(!memcmp(buf+i,_m_boundry.c_str(),_m_boundry.length()))
            return BOUNDRY_MIDDLE;
          else if(!memcmp(buf+i,_l_boundry.c_str(),_l_boundry.length()))
            return BOUNDRY_LAST;
        }else{
          //否则，如果剩余长度小于boundry长度，防止出现半个boundry
          //所以进行部分匹配
          int cmp_len=(blen-i);
          if(!memcmp(buf+i,_l_boundry.c_str(),cmp_len))
            return BOUNDRY_BAK;
          if(!memcmp(buf+i,_m_boundry.c_str(),cmp_len))
            return BOUNDRY_BAK;
        }
      }
      return BOUNDRY_NO;
    }
  public:
    Upload():_file_fd(-1) {}
    bool InitUpload(){//初始化boundry信息；
      std::string len=getenv("Content-Length");
      if(len.size()==0){
        fprintf(stderr,"Have No Content-Length!!\n");
        return false;
      }
      content_len = Tools::StrToDigit(len);
      std::string type=getenv("Content-Type");
      if(type.size()==0){
        fprintf(stderr,"Have No Content-Type!!\n");
        return false;
      }
      std::string boundry_sep="boundry=";
      size_t pos = type.find(boundry_sep);
      if(pos==std::string::npos){
        fprintf(stderr,"content type have no boundry!!\n");
        return false;
      }
      std::string boundry=type.substr(pos+boundry_sep.size());
      _f_boundry="--"+boundry;
      _m_boundry="\r\n"+_f_boundry+"\r\n";
      _l_boundry="\r\n"+_f_boundry+"--";
      return true;
    }
    bool ProcessUpload(){//对正文进行处理，将文件数据进行存储（处理上传）
      int64_t tlen = 0,blen=0;//已读取多少；当前buffer数据长度；
      char buf[MAX_BUFF];
      while(tlen<content_len){
        int len = read(0,buf+blen,MAX_BUFF-blen);
        blen+=len;//当前buf中数据长度
        int boundry_pos,content_pos;
        if(MatchBoundry(buf,blen,&boundry_pos)==BOUNDRY_FIRST){
          //1.匹配到firstBoundry，获取头部，从中获取文件名
          //2.若获取文件名成功，则创建文件，打开文件
          //3.将头信息从buf移除，上下数据进行下一步匹配
          if(GetFileName(buf,&content_pos)){
            CreateFile();
            memmove(buf,buf+content_pos,blen-content_pos);
          }
        }
        while(1){
          if(MatchBoundry(buf,blen,&boundry_pos)!=BOUNDRY_MIDDLE)
            break;
          //匹配middle_boundry成功
          //1.将boundry之前数据写入文件，将数据从buf中移除,关闭文件
          //2.看boundry头中是否有文件名--雷同first_boundry;
          WriteFile(buf,boundry_pos);//
          CloseFile();
          blen-=boundry_pos;//
          memmove(buf,buf+content_pos,blen);
        }
        if(MatchBoundry(buf,blen,&boundry_pos)==BOUNDRY_LAST){
          //匹配Boundry_LAST成功
          //1.将boundry之前的数据写入文件
          //2.关闭文件
          //3.上传文件处理完毕，退出
          WriteFile(buf,boundry_pos);//
          CloseFile();
          return true;
        }
        if(MatchBoundry(buf,blen,&boundry_pos)==BOUNDRY_BAK){
          //1.将类似boundry位置之前的数据写入文件
          //2.移除之前的数据
          //3.剩下的数据不动，重新继续接收数据，补全后继续匹配
          WriteFile(buf,boundry_pos);//
          blen-=boundry_pos;//
          memmove(buf,buf+content_pos,blen);
        }
        if(MatchBoundry(buf,blen,&boundry_pos)==BOUNDRY_NO){
          WriteFile(buf,blen);//
          blen=0;//
        }
        tlen +=len;
      }
      return true;
    }
};

int main(){
  std::string rsp_body;
  rsp_body="<html><body><h1>SUCCESS!</h1></body></html><br/>";
  std::cout<<rsp_body;
  fflush(stdout);
  return 0;
}

