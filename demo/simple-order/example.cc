#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

const int TIME_TRIGGER_1 = 300;
const int TIME_TRIGGER_2 = 
  #ifdef TRIGGERTHEBUG 
    600 
  #else 
    0 
  #endif
;

int data = 0;

void *add(void *v) {
  usleep(TIME_TRIGGER_1);
  data = 2;
  return NULL;
}

int main() {
  pthread_t tid;
  pthread_create(&tid, NULL, add, NULL);

  usleep(TIME_TRIGGER_2);
  data = 1;

  pthread_join(tid, NULL);
  printf("Data: %d\n", data);
  assert(data == 2);
}