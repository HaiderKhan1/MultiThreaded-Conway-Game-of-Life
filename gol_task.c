#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <pthread.h>
#include <sys/time.h>

/*** Code Credits:
 * The code for the queue implementation was taken from the 
 * messagequeueCondition.c example
 * 
*/

int size, iters, draw, unlock;
int directions[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
int **board, **newBoard;

//////////////////////////// Board Operations ///////////////////////////////////////////////////
void freeBoard(int** grid) {
    for (int i = 0; i < size; i++) {
        free(grid[i]);
    }

    free(grid);
}

void printBoard(int** grid) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            printf("%d ", grid[i][j]);
        }

        printf("\n");
    }
}

void fillBoard(int** arr) {
    srand(time(NULL));

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            int rnd = rand();
            arr[i][j] = (rnd > RAND_MAX/2) ? 1 : 0;
        }
    }
    
    return;
}

int** initalizeBoard() {
    int** grid = (int**)malloc(size * sizeof(int*));
    for (int i = 0; i < size; i++) {
        grid[i] = (int*)malloc(size * sizeof(int));
    }
    
    //TODO: check malloc success

    return grid;  
}

// we want to copy the new board to the old board to start a new state
void copyBoard(int** old, int** new) {

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            old[i][j] = new[i][j];
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////

//////////////////////////// Queue Operations //////////////////////////////////////

typedef struct {
    int x;
    int y;
} Coordinate;


//q node
typedef struct node {
    Coordinate coordinate;
    struct node* next;
} Node;

//queue - a singly linked list
//Remove from head, add to tail
typedef struct {
    Node* head;
    Node* tail;
    // the q will have a mutex to lock critical section
    pthread_mutex_t mutex;

    //Add a condition variable
    pthread_cond_t cond;
} Queue;

//Create a queue and initilize its mutex and condition variable
Queue* createQueue()
{
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->head = q->tail = NULL;
    // Initialize the mutex lock
    pthread_mutex_init(&q->mutex, NULL);

    //Initialize the condition variable
    pthread_cond_init(&q->cond, NULL);
    return q;
}

////////////////////////////////////////////////////////////////////////////////

//////////////////////////// Thread Info //////////////////////////////////////
#define NUM_THREADS 3

void* th1_runGOL(void* args);
void* th2_processLiveQ(void* args);
void* th3_processDeadQ(void* args);

typedef struct {
    Queue* liveQueue;
    Queue* deadQueue;
} TH1_ARGS;

////////////////////////////////////////////////////////////////////////////////

double calcTime(struct timeval start){
    
    long long		startusec, endusec;
    struct timeval	end;
    
    gettimeofday(&end, NULL);
    startusec = start.tv_sec * 1000000 + start.tv_usec;
    endusec = end.tv_sec * 1000000 + end.tv_usec;
    return (double)(endusec - startusec) / 1000000.0;
}

int main(int argc, char *argv[]) {

    if (argc < 3 || argc > 4) {
        printf("Sorry Input Not Recognized\n");
        printf("Expected Usage: ./gol_task gridSize nIterations <-d>\n");
        return 0;
    }

    if (argc == 4) {
        if (strcmp("-d", argv[3]) != 0) {
            printf("Please input -d if you wish to draw the boards\n");
            printf("Expected Usage: ./gol_task gridSize nIterations <-d>\n");
        }
    }

    size = atoi(argv[1]);
    iters = atoi(argv[2]);
    draw = argc == 4 ? 1 : 0;
    
    struct timeval	start;
	double	elapsed;

    pthread_t tid[NUM_THREADS];
    Queue* liveQ = createQueue();
    Queue* deadQ = createQueue();
    TH1_ARGS th1_args;
    th1_args.liveQueue = liveQ;
    th1_args.deadQueue = deadQ;

    board = initalizeBoard();
    newBoard = initalizeBoard(); 
    fillBoard(board);
    copyBoard(newBoard, board); // for the first state, we want to replicate board with new board

    if (draw == 1) {
        printf("------ Inital Board ------\n");
        printBoard(newBoard);
    }

    gettimeofday(&start, NULL);

    for (int iter = 0; iter < iters; iter++) {
                
        unlock = 1;
        pthread_create(&tid[0], NULL, th1_runGOL, (void*)&th1_args);
        pthread_create(&tid[1], NULL, th2_processLiveQ, (void*)liveQ);
        pthread_create(&tid[2], NULL, th3_processDeadQ, (void*)deadQ);
        

        for (int i = 0; i < NUM_THREADS; i++) {
            if (i != 0) {
                pthread_cond_signal(&liveQ->cond);
                pthread_cond_signal(&deadQ->cond);
            }

            pthread_join(tid[i], NULL);
        }

        copyBoard(board, newBoard);
        
        if (draw == 1) {
            printf("------ State: %d ------------------\n", iter+1);
            printBoard(newBoard);
        }
    }

    elapsed = calcTime(start);
	printf("Execution time = %.4f seconds\n", elapsed);
    
    freeBoard(board);
    freeBoard(newBoard);
    free(liveQ);
    free(deadQ);
    return 0;

}

void* th1_runGOL(void* arg) {
    TH1_ARGS* args = (TH1_ARGS*)arg;

    for (int row = 0; row < size; row++) {
        for (int col = 0; col < size; col++) {
            
            //calculate all the live cells surrounding current cell
            int liveCells = 0;

            for (int i = 0; i < 8; i++) {
                int r = row + directions[i][0];
                int c = col + directions[i][1];

                if ((r < size && r >= 0) && (c < size && c >= 0) && board[r][c] == 1) {
                    liveCells+=1;
                }
            }

            Node* node = (Node*)malloc(sizeof(Node));
            node->coordinate.x = row;
            node->coordinate.y = col;
            node->next = NULL;
            
            // curr cell comes to life if there are three live neighbors
            // curr continues to live if has 2 or 3 live neighbors
            if ((board[row][col] == 0 && liveCells == 3) || (board[row][col] == 1 && (liveCells == 2 || liveCells == 3))) {
                
                // acquire a the mutex lock to ensure only th1 is in the critical 
                pthread_mutex_lock(&args->liveQueue->mutex);
                
                // if the q is non empty, append after tail
                if (args->liveQueue->tail != NULL) {
                    args->liveQueue->tail->next = node;
                    args->liveQueue->tail = node;
                } else {
                // the queue is empty, so add the first node
                    args->liveQueue->head = args->liveQueue->tail = node;
                }
                
                //Signal the th3 to wake up and process this newly added node
                pthread_cond_signal(&args->liveQueue->cond);


                //release the lock so the th2 can process it
                pthread_mutex_unlock(&args->liveQueue->mutex);


            } else {
                
                // acquire a the mutex lock to ensure only th1 is in the critical 
                pthread_mutex_lock(&args->deadQueue->mutex);
                
                // if the q is non empty, append after tail
                if (args->deadQueue->tail != NULL) {
                    args->deadQueue->tail->next = node;
                    args->deadQueue->tail = node;
                } else {
                // the queue is empty, so add the first node
                    args->deadQueue->head = args->deadQueue->tail = node;
                }

                //Signal the th3 to wake up and process this newly added node
                pthread_cond_signal(&args->deadQueue->cond);
                
                //release the lock so the th3 can process it
                pthread_mutex_unlock(&args->deadQueue->mutex);

            }
        }
    }
    
    unlock = 0;
    pthread_cond_signal(&args->liveQueue->cond);
    pthread_cond_signal(&args->deadQueue->cond);
    
    return NULL;
}


void* th2_processLiveQ(void *arg) {
    Queue *liveQ = (Queue*)arg;
  

    while (unlock || liveQ->head) {

        pthread_mutex_lock(&liveQ->mutex);

        while (liveQ->head == NULL) {
            pthread_cond_wait(&liveQ->cond, &liveQ->mutex);

            if (!unlock && liveQ->head == NULL) {
                pthread_mutex_unlock(&liveQ->mutex);
                return NULL;
            }
        }

        int row = liveQ->head->coordinate.x;
        int col = liveQ->head->coordinate.y;
        newBoard[row][col] = 1;

        //remove the head node
        Node* oldHead = liveQ->head;
        liveQ->head = liveQ->head->next;

        if (liveQ->head == NULL) {
            liveQ->tail = NULL;
        }

        free(oldHead);

        //release the lock on the q
        pthread_mutex_unlock(&liveQ->mutex);
    }

    return NULL;
}

void* th3_processDeadQ(void *arg) {
    Queue *deadQ = (Queue*)arg;

    while (unlock || deadQ->head) {

        pthread_mutex_lock(&deadQ->mutex);

        while (deadQ->head == NULL) {
            pthread_cond_wait(&deadQ->cond, &deadQ->mutex);

            if (!unlock && deadQ->head == NULL) {
                pthread_mutex_unlock(&deadQ->mutex);
                return NULL;
            }
        }

        int row = deadQ->head->coordinate.x;
        int col = deadQ->head->coordinate.y;
        newBoard[row][col] = 0;

        //remove the head node
        Node* oldHead = deadQ->head;
        deadQ->head = deadQ->head->next;

        if (deadQ->head == NULL) {
            deadQ->tail = NULL;
        }

        free(oldHead);

        //release the lock on the q
        pthread_mutex_unlock(&deadQ->mutex);
    }

    return NULL;
}
