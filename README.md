#Conway's Game of Life simulation using two Parallel Programming Approaches

## Data Parallelism 
TO RUN:
```
make
./gol_data numThreads gridSize numIterations -d
```
**Please use the -d flag to draw each and every iteration.**

The data parallelism approach assings threads to process a specific section of the grid.

Parallel Computation Algorithm:
nThreads = 5
gridSize = 100

There will be a total of 5 threads created and since 100 % 5 == 0, we can assign grdSize / nThreads rows to each thread.
Incases where the gridSize % nThreads != 0, the algorithm assings the first nThreads - (gridSize % nThreads) to process girdSize / nThreads rows, and 
the leftover (girdSize % nThreads) threads will process (gridSize / nThreads) + 1 rows.

## Data Parallelism 
TO RUN:
```
make
./gol_data gridSize numIterations -d
```
**Please use the -d flag to draw each and every iteration.**

Parallel Computation Algorithm

The task parallel apprach to COGL entails creating three threads and two queues, where we have queues communicatiing with each other using signals.
- thread1_processGOL will calculate the neighbors for every cell on the gird, and if the cell lives it will add it to the live queue, and if the cell dies it will add it
to the dead queue. This thread will then signal the respective thread to wake and process the newly added node to the queue.

- thread2_processDeadCells: To preserve resources, it waits to receive a signal from the thread1_processGOL and when a node has been added to liveQueue this thread
wakes up and process it by updating the next iterations board.

- thread2_processDeadCells: To preserve resources, it waits to receive a signal from the thread1_processGOL and when a node has been added to deadQueue this thread
wakes up and process it by updating the next iterations board.



