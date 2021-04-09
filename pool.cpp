#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>
#include <list>
using std::list;
typedef void* (*Task)(void *);
struct thread_task{
    Task task;
    void *arg;
};

struct thread_pool{
    pthread_mutex_t list_mutex;
    pthread_cond_t  list_ready;

    list<thread_task> thread_list;

    pthread_t* thread_id;
    
    int max_thread_num;
    int list_size;
    
    thread_pool(int maxnum = 10) : max_thread_num(maxnum), list_size(0){
        pthread_mutex_init(&list_mutex,0);
        pthread_cond_init(&list_ready,0);
        thread_list.clear();
        thread_id = new pthread_t[maxnum];
    }
};


void pool_init(thread_pool * pool, int max_thread_num){
    pool = new thread_pool(max_thread_num);
}