#include "kernel/types.h"
#include "user/user.h"

#define NUM_PHILOSOPHERS 5
#define NUM_CYCLES 3

struct din_shared {
  volatile int forks[NUM_PHILOSOPHERS];
};

void philosopher(int id, struct din_shared *s) {
  int left = id;
  int right = (id + 1) % NUM_PHILOSOPHERS;
  int first_fork = (left < right) ? left : right;
  int second_fork = (left < right) ? right : left;
  
  for (int i = 0; i < NUM_CYCLES; i++) {
    printf("Philosopher %d: THINKING (cycle %d)\n", id, i+1);
    for (volatile int j = 0; j < 10000; j++);
    
    printf("Philosopher %d: HUNGRY\n", id);
    while (s->forks[first_fork] == 1) for (volatile int j = 0; j < 1000; j++);
    s->forks[first_fork] = 1;
    for (volatile int j = 0; j < 5000; j++);
    
    while (s->forks[second_fork] == 1) { 
      s->forks[first_fork] = 0;
      for (volatile int j = 0; j < 1000; j++); 
      while (s->forks[first_fork] == 1) for (volatile int j = 0; j < 1000; j++);
      s->forks[first_fork] = 1;
    }
    s->forks[second_fork] = 1;
    
    printf("Philosopher %d: EATING\n", id);
    for (volatile int j = 0; j < 20000; j++);
    
    s->forks[left] = 0;
    s->forks[right] = 0;
    printf("Philosopher %d: finished eating\n", id);
    for (volatile int j = 0; j < 20000; j++);
  }
  printf("Philosopher %d: DONE\n", id);
  exit(0);
}

int main(int argc, char *argv[]) {
  printf("dining: starting (philosophers=%d, cycles=%d)\n", NUM_PHILOSOPHERS, NUM_CYCLES);
  
  struct din_shared *s = (struct din_shared *)shm_get();
  if ((uint64)s == 0 || (uint64)s == 0xFFFFFFFFFFFFFFFF) { printf("dining: failed to get shared memory\n"); exit(1); }
  
  for (int i = 0; i < NUM_PHILOSOPHERS; i++) s->forks[i] = 0;
  
  for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
    int pid = fork();
    if (pid == 0) {
      s = (struct din_shared *)shm_get(); // Map for child
      if ((uint64)s == 0 || (uint64)s == 0xFFFFFFFFFFFFFFFF) exit(1);
      philosopher(i, s);
    }
  }
  
  for (int i = 0; i < NUM_PHILOSOPHERS; i++) wait(0);
  printf("dining: all philosophers completed successfully! NO DEADLOCK!\n");
  exit(0);
}
