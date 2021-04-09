#include <pthread.h>
#include <list>
#include <iostream>
#include <string>
using namespace std;

int k;
pthread_mutex_t mutex;
pthread_cond_t cond;

void *route(void * args){
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex);
    k++;
    cout << k << endl;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main(){
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    pthread_t p1;
    for (int i = 0 ; i < 10 ; ++i){
        pthread_create(&p1, NULL, route, NULL);
    }
   


    int x = 0;
    while(cin >> x){
        if(x > 1){
            pthread_cond_signal(&cond);
        } else {
            pthread_cond_broadcast(&cond);
        }
    }

    return 0;
}