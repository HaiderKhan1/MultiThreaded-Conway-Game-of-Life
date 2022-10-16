#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <pthread.h>
#include <sys/time.h>


int threads, size, iters, draw, unlock;
int directions[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
int **board, **newBoard;
struct Info {
    int start,rows;
};

pthread_barrier_t finishIter;
pthread_barrier_t setIter;


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

///////////////////////////////////////////////////////////////////////////////


void* execThread(void *arg) {
    struct Info *info = (struct Info*)arg;
    int startIdx = (*info).start;
    int rows = (*info).rows;

    while (unlock) {

        for (int row = startIdx; row < startIdx + rows; row++) {
            for (int col = 0; col < size; col++) {
                
                // calculate all the live cells surrounding current cell
                int liveCells = 0;

                for (int i = 0; i < 8; i++) {
                    int r = row + directions[i][0];
                    int c = col + directions[i][1];

                    if ((r < size && r >= 0) && (c < size && c >= 0) && board[r][c] == 1) {
                        liveCells+=1;
                    }
                }

                // curr cell dies, if alive and is under or over populated
                if (board[row][col] == 1 && (liveCells < 2 || liveCells > 3)) {
                    newBoard[row][col] = 0;
                }

                // curr cell comes to life if there more then 3 live neighbors
                if (board[row][col] == 0 && liveCells == 3) {
                    newBoard[row][col] = 1;
                }
            }
        }

        pthread_barrier_wait(&finishIter);
        pthread_barrier_wait(&setIter);
    }

    free(arg);
    return NULL;
}


double calcTime(struct timeval start){
    
    long long		startusec, endusec;
    struct timeval	end;
    
    gettimeofday(&end, NULL);
    startusec = start.tv_sec * 1000000 + start.tv_usec;
    endusec = end.tv_sec * 1000000 + end.tv_usec;
    return (double)(endusec - startusec) / 1000000.0;
}

int main(int argc, char *argv[]) {

    if (argc < 4 || argc > 5) {
        printf("Sorry Input Not Recognized\n");
        printf("Expected Usage: ./gol_data nThreads gridSize nIterations <-d>\n");
        return 0;
    }

    if (argc == 5) {
        if (strcmp("-d", argv[4]) != 0) {
            printf("Please input -d if you wish to draw the boards\n");
            printf("Expected Usage: ./gol_data nThreads gridSize nIterations <-d>\n");
        }
    }

    threads = atoi(argv[1]);
    size = atoi(argv[2]);
    iters = atoi(argv[3]);
    draw = argc == 5 ? 1 : 0;
   

    board = initalizeBoard();
    newBoard = initalizeBoard(); 
    fillBoard(board);
    copyBoard(newBoard, board); // for the first state, we want to replicate board with new board


    pthread_t tid[threads];
    int rows = size / threads;
    int firstN = threads - (size % threads);
    pthread_barrier_init(&finishIter, NULL, threads+1);
    pthread_barrier_init(&setIter, NULL, threads+1);
    struct timeval	start;
	double	elapsed;
    struct Info *info;

    if (draw == 1) {
        printf("------ Inital Board ------\n");
        printBoard(newBoard);
    }

    unlock = 1;

    gettimeofday(&start, NULL);

    // the first n = (nThreads - firstN) threads will process rows number of rows
    for (int i = 0; i < firstN ; i++) {
        info = malloc(sizeof(struct Info));
        (*info).start = i * rows;
        (*info).rows = rows;
        pthread_create(&tid[i], NULL, execThread, (void*)info);
    }

    // the last (size % threads) threads will process rows + 1 number of rows
    int offset = 0;
    for (int i = firstN; i < threads; i++) {
        info = malloc(sizeof(struct Info));
        (*info).start = (i * rows) + offset;
        (*info).rows = rows + 1;
        offset++;
        pthread_create(&tid[i], NULL, execThread, (void*)info);
    }
    
    for (int iter = 0; iter < iters; iter++) {
        
        pthread_barrier_wait(&finishIter);

        copyBoard(board, newBoard);

        if (draw == 1) {
            printf("------ State: %d ------\n", iter+1);
            printBoard(newBoard);
        }

        if (iter + 1 == iters) {
            unlock = 0;
        }

        pthread_barrier_wait(&setIter);
    }



    for (int i = 0; i < threads; i++) {
        pthread_join(tid[i], NULL);
    }

    elapsed = calcTime(start);
	printf("Execution time = %.4f seconds\n", elapsed);
    
    pthread_barrier_destroy(&finishIter);
    pthread_barrier_destroy(&setIter);
    freeBoard(board);
    freeBoard(newBoard);

}

