#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_THREADS 10
#define NS_PER_SEC 1000000000

typedef struct Train{
    int id;
    char direction; //"E","e","W","w"
    int priority; //high=1 low =0
    int loading_time; //seconds
    int crossing_time; //seconds
    struct timespec arrival_time; //when train finishes loading
    struct Train* next; //pointer to next train in queue
}Train;

typedef struct Queue{
    Train* head;
    Train* tail;
    pthread_mutex_t mutex;
}Queue;

Train* next_train();
Queue eastQueue ={NULL, NULL};
Queue westQueue ={NULL, NULL};



pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t trainready = PTHREAD_COND_INITIALIZER;
pthread_mutex_t main_track_mutex = PTHREAD_MUTEX_INITIALIZER;

char last_direction = '\0'; //last direction that used the main track
int trains_in_same_direction =0; //number of trains that have passed in the same direction
struct timespec start_time;
int total_trains =0;
int trains_finished =0;
FILE *output_file; 

int compare_trains( Train *t1, Train *t2); //function declaration

void print_time(const char* format, int train_id,const char* dir){
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    double elapsed = (current_time.tv_sec - start_time.tv_sec) + 
                     (current_time.tv_nsec - start_time.tv_nsec) /(double)NS_PER_SEC;


    int hours = (int)(elapsed/3600);
    int minutes = (int)((elapsed - hours*3600)/60);
    int seconds = (int)(elapsed - hours*3600 - minutes*60);
    int tenths = (int)((elapsed - (int)elapsed)*10);

    fprintf(output_file, "%02d:%02d:%02d.%d ", hours, minutes, seconds, tenths);
    fprintf(output_file, format, train_id, dir);
    fprintf(output_file, "\n");
    fflush(output_file);

    printf("%02d:%02d:%02d.%d ", hours, minutes, seconds, tenths);
    printf(format, train_id, dir);
    printf("\n");
    fflush(output_file);
}

void enqueue(Queue *q, Train *t){
    t->next = NULL;
    if(q->head==NULL){
        q->head =t;
        q->tail = t;
        return;
    }
    // } else {
    //     q->tail->next = t;
    //     q->tail = t;
    // }

    //insert based on priority and arrival time

    Train* current = q->head;
    Train *previous = NULL;
    while(current!=NULL){
        if(t->priority >current->priority){
            //higher priority, insert here
            if(previous==NULL){
                //insert at head
                t->next = q->head;
                q->head = t;
            } else {
                previous->next = t;
                t->next = current;
            }
            return;
        } else if(t->priority == current->priority){
            //same priority, check arrival time
            if(compare_trains(t, current)<0){
                //earlier arrival, insert here
                if(previous==NULL){
                    //insert at head
                    t->next = q->head;
                    q->head = t;
                } else {
                    previous->next = t;
                    t->next = current;
                }
                return;
            }
        }
        previous = current;
        current = current->next;
    }
    q->tail->next = t;
    q->tail = t;
}

Train* dequeue(Queue *q){
    if(q->head==NULL) return NULL;
    Train* t = q->head;
    q->head = q->head->next;
    if(q->head==NULL) q->tail = NULL;
    return t;
}

void *train_thread(void *train_void){
    Train *t = (Train*)train_void; // ENSURES EACH THREAD WILL POINT TO TRAIN STRUCT

    usleep(t->loading_time *100000); // Simulate loading time

    clock_gettime(CLOCK_MONOTONIC, &t->arrival_time); // Record arrival time

    

    pthread_mutex_lock(&queue_mutex);
    // Add train to appropriate queue
    if(t->direction=='E' || t->direction=='e'){
        //pthread_mutex_lock(&eastQueue.mutex);
        enqueue(&eastQueue, t);// list(&east_queue, t);
        print_time("Train %2d is ready to go %4s", t->id, "East");
        //pthread_mutex_unlock(&eastQueue.mutex);

    } else {
        //pthread_mutex_lock(&westQueue.mutex);
        enqueue(&westQueue, t);
        print_time("Train %2d is ready to go %4s", t->id, "West");
        //pthread_mutex_unlock(&westQueue.mutex);
    }

    pthread_cond_signal(&trainready); // Signal that a train is ready
    pthread_mutex_unlock(&queue_mutex);

    return NULL;


}


void * dispatcher_thread(void *arg){
    while(1){
        pthread_mutex_lock(&queue_mutex);
        // Wait until a train is ready
        while(eastQueue.head==NULL && westQueue.head==NULL && trains_finished < total_trains){
            pthread_cond_wait(&trainready, &queue_mutex);
        }
        if(eastQueue.head==NULL && westQueue.head==NULL && trains_finished >= total_trains){
            pthread_mutex_unlock(&queue_mutex);
            break; // All trains have been dispatched
        }

        // Determine which train to dispatch next
        Train *next = next_train();
        

        if(next){
            // Dispatch the train (simulate crossing)
            pthread_mutex_unlock(&queue_mutex); // Unlock before crossing

            //acquire main track
            pthread_mutex_lock(&main_track_mutex);

            const char* dir = (next->direction=='E' || next->direction=='e') ? "East" : "West";
            print_time("Train %2d is ON the main track going %4s", next->id, dir);
            usleep(next->crossing_time *100000); // Simulate crossing time
            print_time("Train %2d is OFF the main track after going %4s", next->id, dir);

            char next_direction = (next->direction=='E' || next->direction=='e') ? 'E' : 'W';
            char last_dir = (last_direction=='E' || last_direction=='e') ? 'E' : 'W';   

            if(last_direction != '\0' && next_direction == last_dir){
                trains_in_same_direction++;
            } else {
                trains_in_same_direction =1;
            }

            last_direction = next->direction; // Update last direction
            trains_finished++;
            pthread_mutex_unlock(&main_track_mutex);
            //free(next); // Free train memory after crossing
            free(next);
        } else {
            pthread_mutex_unlock(&queue_mutex); // Unlock if no train to dispatch
        }
        // Small delay to prevent busy waiting
        //usleep(100000);
    }
    return NULL;

          
}



int compare_trains( Train *t1, Train *t2){
    if(t1->priority > t2->priority){
        return -1; //t1 has higher priority
    } else if(t2->priority > t1->priority){
        return 1; //t2 has higher priority
    } else {
        //same priority, first come first serve
        if(t1->arrival_time.tv_sec < t2->arrival_time.tv_sec){
            return -1; //t1 arrived first
        } else if(t2->arrival_time.tv_sec < t1->arrival_time.tv_sec){
            return 1; //t2 arrived first
        } else {
            if(t1->arrival_time.tv_nsec < t2->arrival_time.tv_nsec){
                return -1; //t1 arrived first
            } else {
                return 1; //t2 arrived first
            }
        }
    }
}

Train* next_train(){
    //pthread_mutex_lock(&eastQueue.mutex);
    //pthread_mutex_lock(&westQueue.mutex);

    Train *east = eastQueue.head;
    Train *west = westQueue.head;

    //Train *toGo = NULL;

    if(east==NULL && west==NULL){
        //no trains
        //pthread_mutex_unlock(&eastQueue.mutex);
        //pthread_mutex_unlock(&westQueue.mutex);
        return NULL;
    }
    if(east==NULL) return dequeue(&westQueue);
    if(west==NULL) return dequeue(&eastQueue);

    //starvation prevention
    if(trains_in_same_direction >=2){
        char last_dir_normalized = (last_direction=='E'|| last_direction =='e') ? 'E' : 'W';
        
        if(last_dir_normalized == 'E' && west!=NULL){ //last was east, so now west
            trains_in_same_direction =1;
            return dequeue(&westQueue);
        } else if(last_dir_normalized == 'W' && east!=NULL){ //last was west, so now east
            trains_in_same_direction =1;
            return dequeue(&eastQueue);
        }
    }

    //both queues have trains
    char east_dir = (east->direction=='E' || east->direction=='e') ? 'E' : 'W';
    char west_dir = (west->direction=='W' || west->direction=='w') ? 'W' : 'E';

    //same direction
    if(east_dir == west_dir){
        Train* selected;
        if(compare_trains(east, west)<=0){
            selected = dequeue(&eastQueue);
        } else {
            selected = dequeue(&westQueue);
        }
        
        char selected_direction = (selected->direction=='E' || selected->direction=='e') ? 'E' : 'W';
        char last_dir = (last_direction=='E' || last_direction=='e') ? 'E' : 'W';

        if(last_dir!='\0' && selected_direction == last_dir){
            trains_in_same_direction++;
        } else {
            trains_in_same_direction =1;
        }
        return selected;
    } else {
        //different directions
        if(east->priority != west->priority){
            Train* selected;
            if(east->priority > west->priority){
                selected = dequeue(&eastQueue);
            } else {
                selected = dequeue(&westQueue);
            }
            char selected_direction = (selected->direction=='E' || selected->direction=='e') ? 'E' : 'W';
            char last_dir = (last_direction=='E' || last_direction=='e') ? 'E' : 'W';

            if(last_dir!='\0' && selected_direction == last_dir){
                trains_in_same_direction++;
            } else {
                trains_in_same_direction =1;
            }
            return selected;
        } 
            //same priority, first come first serve
            Train* selected;
            if(last_direction == '\0'){
                selected = dequeue(&westQueue); //default west
            
            }else{
                char last_dir = (last_direction=='E' || last_direction=='e') ? 'E' : 'W';

                if(east_dir != last_dir){
                    selected = dequeue(&eastQueue);
                } else {
                    selected = dequeue(&westQueue);
                }
            }
            trains_in_same_direction =1;
            return selected;

    }

    

}




//////START FORM HERE////// 


int main(int argc, char *argv[]){

    if(argc!=2){
        printf("Usage: %s <train_file>\n", argv[0]);
        return 1;
    }


    output_file = fopen("output.txt", "w");
    if(!output_file){
        perror("Failed to open output file");
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if(!file){
        perror("Failed to open file");
        fclose(output_file);
        return 1;
    }

    Train *trains[100];
    total_trains =0;
    char dir;
    int load_time, cross_time;


    while(fscanf(file, " %c %d %d", &dir, &load_time, &cross_time) ==3){
        Train *t = (Train*)malloc(sizeof(Train));
        t->id = total_trains;
        t->direction = dir;
        t->priority = (dir=='E' || dir=='W') ? 1 : 0; //east high priority
        t->loading_time = load_time;
        t->crossing_time = cross_time;
        t->next = NULL;
        trains[total_trains] = t;
        total_trains++;
    }
    fclose(file);

    //start time
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    //initialize mutexes and condition variables

    pthread_mutex_init(&queue_mutex, NULL);
    //pthread_cond_init(&main_track_convar, NULL);
    pthread_mutex_init(&main_track_mutex, NULL);
    pthread_cond_init(&trainready, NULL);

    
    pthread_t *train_threads = malloc(sizeof(pthread_t) * total_trains);
    //create train threads
    for(int i =0; i<total_trains;i++){
        pthread_create(&train_threads[i], NULL, train_thread, trains[i]);
    }

    pthread_t dispatcher;
    pthread_create(&dispatcher, NULL, dispatcher_thread, NULL); 

    for(int i =0; i<total_trains;i++){
        pthread_join(train_threads[i], NULL);
    }
    pthread_join(dispatcher, NULL);

    free(train_threads);
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&main_track_mutex);
    pthread_cond_destroy(&trainready);

    fclose(output_file);

    

    return 0;

    
}
