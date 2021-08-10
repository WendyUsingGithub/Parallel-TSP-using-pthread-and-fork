/* gcc tsp_pthread.c -lpthread -lm -o tsp_pthread */
/* time ./tsp_pthread testcase/pr1002.txt > output/pthread_1002.txt 2 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <string.h>

struct ARG {
    int id;
    int threadNum;
    int cityNum; 
    double** distMatrix; 
    int *route; 
    double *length; 
    int *swap1;
    int *swap2;
};

struct CITY {
    double x;
    double y;
};

/* tsp related funtions */
void buildDistMatrix(int cityNum, struct CITY *cities, double **distMatrix);
double calLength(int cityNum, double **distMatrix, int *route);
void tsp2opt(int *route, int reverseStart, int reverseEnd);

/* pthread related funtions */
void *greedySearch_pthread(void *argument);
int findBestThread(double *lengths, int threadNum, int *swap1s, int *swap2s);

/* utility functions */
int copyRoute(int cityNum, int *destRoute, int *srcRoute);

/* global variable for synchronization */
int threadFinish = 0;
pthread_cond_t cv;
pthread_mutex_t lock;

int main(int argc, char *argv[]) 
{
    FILE * file = NULL;
    int threadNum = atoi(argv[2]);
    pthread_t *threads = malloc(sizeof(pthread_t) * threadNum);
    char fileName[20] = "";
    double **distMatrix;

    /* problem info */
    int cityNum = 0;
    double optimalLength = 0;
    struct CITY *cities = NULL;

    /* variables for search */
    double length = 0;
    double lastLength = 0;
    int bestThread = 0;
    int *bestRoute = NULL;

    /* pthread arguments */
    struct ARG *args = NULL;
    double *lengths = NULL;
    int **routes = NULL;
    int *swap1s = NULL;
    int *swap2s = NULL;

    strcpy(fileName, argv[1]);
    file = fopen(argv[1] , "r");
    if(file == NULL) return 0; 
    
    fscanf(file, "%d", &cityNum);
    fscanf(file, "%lf", &optimalLength);

    cities = (struct CITY *)malloc(sizeof(struct CITY) * cityNum);
    bestRoute = (int *)malloc(sizeof(int) * cityNum);
    distMatrix = (double **)malloc(sizeof(double *) * cityNum);
    for(int i = 0; i < cityNum; i++) {
        distMatrix[i] = (double *)malloc(sizeof(double) * cityNum);
    }

    /* pthread arguments */
    args = (struct ARG *)malloc(sizeof(struct ARG) * threadNum);
    lengths = (double *)malloc(sizeof(double) * threadNum);
    routes = (int **)malloc(sizeof(int *) * threadNum);
    for(int i = 0; i < threadNum; i++) {
        routes[i] = (int *)malloc(sizeof(int) * cityNum);
    }
    swap1s = (int *)malloc(sizeof(int) * threadNum);
    swap2s = (int *)malloc(sizeof(int) * threadNum);

    for(int i = 0; i < cityNum; i++) {
        fscanf(file, "%lf", &cities[i].x);
        fscanf(file, "%lf", &cities[i].y);
    }
    fclose(file);

    buildDistMatrix(cityNum, cities, distMatrix);

    /* initialize route as 0-1-2-3-4--- */
    for(int i = 0; i < cityNum; i++) {
        bestRoute[i] = i;
    } 
    lastLength = length = calLength(cityNum, distMatrix, bestRoute); 
    printf("length %f\n", length);

    /* package argument for pthread */
    for(int i = 0; i < threadNum; i++) {    
        /* no update during search */
        args[i].id = i;
        args[i].threadNum = threadNum;
        args[i].cityNum = cityNum;
        args[i].distMatrix = distMatrix;

        /* should be update during search */
        args[i].route = routes[i];
        args[i].length = &lengths[i];
        args[i].swap1 = &swap1s[i];
        args[i].swap2 = &swap2s[i];
    }

    /* greedy search until no length improvement */
    while(1) {
        threadFinish = 0;
        lastLength = length;

        /* update argument for pthread */
        for(int i = 0; i < threadNum; i++) {
            copyRoute(cityNum, routes[i], bestRoute);
            lengths[i] = length;
        }

        for(int i = 0; i < threadNum; i++) {
            pthread_create(&threads[i], NULL, greedySearch_pthread, (void*)(&args[i]));
        }

        /* synchronize all threads */
        pthread_mutex_lock(&lock);
        pthread_cond_wait(&cv, &lock);
        pthread_mutex_unlock(&lock);

        for(int i = 0; i < threadNum; i++) {
            pthread_join(threads[i], NULL);  
        }

        bestThread = findBestThread(lengths, threadNum, swap1s, swap2s);
        copyRoute(cityNum, bestRoute, routes[bestThread]);
        length = lengths[bestThread];
        printf("length %f\n", length);
        if(lastLength - length < 1) break;
    }

    free(threads);
    free(cities);
    free(bestRoute);    
    for(int i = 0; i < cityNum; i++) {
        free(distMatrix[i]);
    }
    free(distMatrix);
    free(args);
    free(lengths);
    for(int i = 0; i < threadNum; i++) {
        free(routes[i]);
    }
    free(routes);
    free(swap1s);
    free(swap2s);
    return 0;
}

int copyRoute(int cityNum, int *destRoute, int *srcRoute) 
{
    for(int i = 0; i < cityNum; i++) {
        destRoute[i] = srcRoute[i];
    }
}

void buildDistMatrix(int cityNum, struct CITY *cities, double **distMatrix) 
{
    double square = 0; 
    for(int i = 0; i < cityNum; i++) {
        distMatrix[i][i] = 0;
    }
    
    for(int i = 0; i < cityNum; i++) {
        for(int j = i + 1; j < cityNum; j++) {
            double x1 = cities[i].x;
            double x2 = cities[j].x;
            double y1 = cities[i].y;
            double y2 = cities[j].y;
            double dx = x1 - x2;
            double dy = y1 - y2;

            square = dx * dx + dy * dy;
            distMatrix[i][j] = sqrt((double)square);
            distMatrix[j][i] = distMatrix[i][j];
        }
    }
}

double calLength(int cityNum, double **distMatrix, int *route) 
{
    double length = 0;
    for(int i = 0; i < cityNum - 1; i++) {
        length = length + distMatrix[route[i]][route[i+1]]; 
    }
    length = length + distMatrix[route[cityNum - 1]][route[0]];
    return length;
}

int findBestThread(double *lengths, int threadNum, int *swap1s, int *swap2s) 
{
    int bestId = 0;
    int bestSwap1 = 0;
    int bestSwap2 = 0;
    double bestLength = lengths[0];

    for(int i = 1; i < threadNum; i++) {
        if(lengths[i] < bestLength) {
            bestLength = lengths[i];
            bestId = i;
        }
    }

    bestSwap1 = swap1s[bestId];
    bestSwap2 = swap2s[bestId];

    /* if multiple routes have same length , choose the one with smaller swap index */
    for(int i = 0; i < threadNum; i++) {
        if(i == bestId) continue;
        if(lengths[i] == bestLength) {
            if(swap1s[i] < bestSwap1 || (swap1s[i] == bestSwap1 && swap2s[i] < bestSwap2)) {
                bestSwap1 = swap1s[i];
                bestSwap2 = swap2s[i];
                bestId = i;
            }
        }
    }
    return bestId;
}

/* reverse every possible segment of a route to greedy search this route */
void *greedySearch_pthread(void *argument) 
{
    struct ARG *arg = (struct ARG *)argument;
    int id = arg -> id;
    int cityNum = arg->cityNum;
    double** distMatrix = arg->distMatrix;
    int *route = arg->route;
    double *length = arg->length;
    int threadNum = arg->threadNum;
    int *swap1 = arg->swap1;
    int *swap2 = arg->swap2;

    double delta = 0;
    double bestDelta = 0;
    int start = 0;
    int end = 0;
    int beforeStart = 0; 
    int afterEnd = 0; 
    int bestStart = 0;
    int bestEnd = 0;
    
    /* reverse a segment of a route */
    for(int start = id; start < cityNum; start = start + threadNum) {
        for(int end = start + 1; end < cityNum; end++) {

            beforeStart = start - 1;
            afterEnd = end + 1;
            if(beforeStart == -1) beforeStart = cityNum - 1;
            if(afterEnd == cityNum) afterEnd = 0;

            delta = -distMatrix[route[beforeStart]][route[start]] + distMatrix[route[beforeStart]][route[end]] 
                    -distMatrix[route[end]][route[afterEnd]] + distMatrix[route[start]][route[afterEnd]];
            
            if(delta < bestDelta) {
                if (!(start == 0 && end == cityNum-1)) {
                    bestDelta = delta;
                    bestStart = start;
                    bestEnd = end;
                }
            }
        }
    }

    *swap1 = bestStart;
    *swap2 = bestEnd;

    /* update route */
    tsp2opt(route, bestStart, bestEnd);
    *length = *length + bestDelta;

    pthread_mutex_lock(&lock);
    threadFinish++;
    if(threadFinish == threadNum) pthread_cond_signal(&cv);
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

/* update the route with a segment reverse */
void tsp2opt(int *route, int reverseStart, int reverseEnd) 
{
    int tmp;
    int i = reverseStart;
    int j = reverseEnd;

    while(1) {
        if(i < j) {
            tmp = route[i];
            route[i] = route[j];
            route[j] = tmp;
			i++;
			j--;
        }
        else {
            break;
        }
    }
}
