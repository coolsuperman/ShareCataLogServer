#include"Tools.hpp"

typedef bool(*Header)(int);

class Task{
  private:
    int socket;
    Header TaskHeader;
  public:
    Task(int s=0,Header TH=NULL)
      :socket(s),TaskHeader(TH)
    {}
    void SetTask(int s,Header TH){
      socket = s;
      TaskHeader = TH;
    }
    bool Run(){
      return TaskHeader(socket);
    }
};

class ThreadPool{
  private:
    size_t MAX_pthread;
    size_t NOW_pthread;
    std::queue<Task> Queue;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool quit = false;
  private:
    void QueueLock(){
      pthread_mutex_lock(&lock);
    }
    void QueueUnlock(){
      pthread_mutex_unlock(&lock);
    }
    void ThreadSleep(){
      if(quit){
        NOW_pthread--;
        QueueUnlock();
        pthread_exit(NULL);
      }
      pthread_cond_wait(&cond,&lock);
    }
    void ThreadWakeUp(int i = 0){
      if(i){
        pthread_cond_signal(&cond);
        return;
      }
      pthread_cond_broadcast(&cond);
    }
    bool IsFull(){
      return NOW_pthread>=MAX_pthread?1:0;
    }
    bool IsEmpty(){
      return !(Queue.size());
    }
    void TaskGet(Task& s){
      s = Queue.front();
      Queue.pop();
    }
  public:
    ThreadPool(int max=10)
      :MAX_pthread(max)
    {}
    ~ThreadPool(){
      pthread_cond_destroy(&cond);
      pthread_mutex_destroy(&lock);
    }
    static void* work(void* arg){
      pthread_detach(pthread_self());
      ThreadPool* serv =(ThreadPool*)arg;
      while(1){
       serv->QueueLock();
        while(serv->IsEmpty()){//不用if：防止下一次时间片来临时不再判断是否为空直接下去
          serv->ThreadSleep();
        }
        Task t;
        serv->TaskGet(t);
        serv->QueueUnlock();
        t.Run();
      }
    }
    void PoolInit(){
      pthread_t tpid[MAX_pthread];
      for(size_t i = 0;i<MAX_pthread;++i){
        pthread_create(tpid+i,NULL,work,(void*)this);
      }
    }
    void AddTask(const Task& s){
      QueueLock();
      Queue.push(s);
      ThreadWakeUp(1);
      QueueUnlock();
    }
    bool StopPool(){
      quit = true;
      while(NOW_pthread){
        ThreadWakeUp();
      }
      return true;
    }
};
