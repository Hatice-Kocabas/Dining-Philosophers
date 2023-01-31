
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

enum distribution {exponential_distribution, uniform_distrubition};
enum states {thinking_state, hungry_state, dining_state};

int num_phsp;
int min_think;;
int max_think;
int min_dine;
int max_dine;
int count;
enum distribution distrubition;



struct monitor{

    pthread_mutex_t *mutex;
    pthread_cond_t *status;
    enum states *phsp_state;

} chopsticks;


void initialize_monitor(int num){

    for(int i = 0; i < num; i++){
        chopsticks.phsp_state[i] = thinking_state;
    }

}



int random_time_generator(int min, int max){
    
    double dining_thinking_time = 0;

    if(max == min){
        return max;
    }
    if(distrubition == exponential_distribution) {
        while( dining_thinking_time < min||dining_thinking_time > max ){
            double rand_num = ((double)rand() / RAND_MAX);
            dining_thinking_time = (-((double)(min + max)) / 2) * log(1 - rand_num);
        }
    }
  
    else if(distrubition == uniform_distrubition){
        while(dining_thinking_time > max || dining_thinking_time < min){
            double rand_num = ((double)rand() / RAND_MAX);
            dining_thinking_time = max * rand_num;
        }
    } else  {
        exit(0);
    }

    return (int)dining_thinking_time;

}


void philosopher_is_dining(int i){

    if(chopsticks.phsp_state[(i + num_phsp - 1) % num_phsp] != dining_state){
        if(chopsticks.phsp_state[( i + 1 ) % num_phsp] != dining_state){
            if(chopsticks.phsp_state[i] == hungry_state){
                chopsticks.phsp_state[i] = dining_state;
                printf(" - Philosopher %d is dining. \n", (i + 1));
                pthread_cond_signal(&chopsticks.status[i]);
            }
        }
    }
}


int take_chopstick(int i){
    chopsticks.phsp_state[i] = hungry_state;
    
    printf(" - Philoshoper %d is hungry. \n", (i + 1));
    
    clock_t start = clock();
    philosopher_is_dining(i);
    int auto_lock = -1;
    
    if(chopsticks.phsp_state[i] != dining_state){
        
        if(chopsticks.phsp_state[(i + 1) % num_phsp] == dining_state){
        
            auto_lock = (i + 1) % num_phsp;
            pthread_cond_wait(&chopsticks.status[i], &chopsticks.mutex[(i + 1) % num_phsp]);
        
        } else{   
        
            auto_lock = i;
            pthread_cond_wait(&chopsticks.status[i], &chopsticks.mutex[i]);
        
        }

    } else{ 
        
        pthread_mutex_lock(&chopsticks.mutex[i]);
        pthread_mutex_lock(&chopsticks.mutex[(i + 1) % num_phsp]);
    
    }

    
    if(auto_lock > 0){
        
        if(auto_lock == i){
        
            pthread_mutex_lock(&chopsticks.mutex[(i + 1) % num_phsp]);
        
        } else{
          
            pthread_mutex_lock(&chopsticks.mutex[i]);
        
        }
    }

    return (clock() - start) * 1000 / CLOCKS_PER_SEC;
}



void leave_chopstick(int philosopher_number){
    
    chopsticks.phsp_state[philosopher_number] = thinking_state;

    pthread_mutex_unlock(&chopsticks.mutex[philosopher_number]);
    pthread_mutex_unlock(&chopsticks.mutex[(philosopher_number + 1) % num_phsp]);

    printf(" - Philosopher %d is thinking. \n", (philosopher_number + 1));

    philosopher_is_dining((philosopher_number + num_phsp - 1) % num_phsp);
    philosopher_is_dining((philosopher_number + 1) % num_phsp);

}


void* philosopher_start(void* arg){

    int id = *((int *)arg);
    int local_count = 0;
    int h_time_sum = 0;

    printf(" - Philosopher %d created and start to thinking. \n", (id + 1));

    while(local_count < count){
        
        usleep((useconds_t)(random_time_generator(max_think, min_think) * 1000));
        
        h_time_sum = h_time_sum + take_chopstick(id);

        int dining_time = random_time_generator(max_dine, min_dine);
        
        clock_t start = clock();
        clock_t diff;
        
        int msec;
        
        while(msec < dining_time){

            diff = clock() - start;
            msec = diff * 1000 / CLOCKS_PER_SEC;

        }

        
        leave_chopstick(id);

        local_count = local_count + 1;
    }

    
    printf(" - Philosopher %d duration of hungry state = %d\n", (id + 1), h_time_sum);

    pthread_exit(NULL);

}



int main(int argc, char *argv[]){
    
    

    num_phsp = atoi(argv[1]);

    if(num_phsp % 2 == 0 ){
        printf("The number of philosophers cannot be even number.\n"); 
        exit(0);
    }
    else if( num_phsp > 27){
        printf("The number of philosophers cannot be more than 27.\n");
    }

    min_think = atoi(argv[2]);
    max_think = atoi(argv[3]);
    min_dine = atoi(argv[4]);
    max_dine = atoi(argv[5]);

    if(max_think > 60000){
        printf("PLease enter thinking time less than 60000. \n");        
        exit(0);
    }

    if(max_dine > 60000){
        printf("PLease enter the dining time less than 60000.\n");        
        exit(0);
    }

    if(min_think < 1){
        printf("The minimum thinking time should not be less than 1.\n");        
        exit(0);    
    }

    if(min_dine < 1){
        printf("The minimum dining time should not be less than 1.\n");        
        exit(0);
    }

    if(strcmp(argv[6], "exponential") == 0){
        distrubition = exponential_distribution;
    }
    else if(strcmp(argv[6], "uniform") == 0){
        distrubition = uniform_distrubition;
    }
    else {
        printf("There are some problems on distribution parameter!!! \n");
        exit(0);
    }

    count = atoi(argv[7]);


    chopsticks.mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * num_phsp);
    chopsticks.status = (pthread_cond_t *)malloc(sizeof(pthread_cond_t) * num_phsp);
    chopsticks.phsp_state = (enum states *)malloc(sizeof(enum states) * num_phsp);

    initialize_monitor(num_phsp);

    for(int i = 0; i < num_phsp; i++){
        pthread_mutex_init(&chopsticks.mutex[i], NULL);
        pthread_cond_init(&chopsticks.status[i], NULL);
    }


    pthread_t threads[num_phsp];
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    int ids[num_phsp];
    for(int i = 0; i < num_phsp; i++){
        ids[i] = i;
    }

    for(int i = 0; i < num_phsp; i++){
        pthread_create(&threads[i], &attr, philosopher_start, (void *)&ids[i]);
    }

    
    for(int i = 0; i < num_phsp; i++){
        pthread_join(threads[i], NULL);
    }

    
    pthread_attr_destroy(&attr);

    for(int i = 0; i < num_phsp; i++){
        pthread_mutex_destroy(&chopsticks.mutex[i]);
        pthread_cond_destroy(&chopsticks.status[i]);
    }

    free(chopsticks.mutex);
    free(chopsticks.status);
    free(chopsticks.phsp_state);
    exit(0);
}
