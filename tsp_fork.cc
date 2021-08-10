/* g++ tsp_fork.cc -lm -o tsp_fork */
/* time ./tsp_fork testcase/pr1002.txt 2 > output/fork_1002.txt */

#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <math.h>
using namespace std;

struct CITY {
    double x;
    double y;
};

/* shared memory for synchronization */
static int *finish;
static int *passGranted;
static double **distMatrix;
static int **routes;
static int *swap1s;
static int *swap2s;
static double *lengths;

void buildDistMatrix(int cityNum, struct CITY *cities, double **distMatrix);
double calLength(int cityNum, double **distMatrix, int *route);
void greedySearch_fork(int id, int cityNum, double **distMatrix, int *route, double *length, int childNum, int *swap1, int *swap2);
void tsp2opt(int *route, int reverseStart, int reverseEnd);
int copyRoute(int cityNum, int *destRoute, int *srcRoute);
int findBestchild(double *lengths, int childNum, int *swap1s, int *swap2s);
int canPass(int childNum, int *finish);

int main(int argc, char *argv[]) 
{
    FILE * file = NULL;
    int childNum = atoi(argv[2]);
    char fileName[20] = "";
    int cityNum = 0;
    struct CITY *cities = NULL;
    int *route = NULL;
    double optimalLength = 0;
    double length = 0;
    double lastLength = 0;

    /* variables for fork */
    pid_t pid = 0;
    int id = 0;
    int isMain = 1;
    int bestId = 0;

    strcpy(fileName, argv[1]);
    file = fopen(argv[1] , "r");
    if(file == NULL) return 0; 
    
    fscanf(file, "%d", &cityNum);
    fscanf(file, "%lf", &optimalLength);

    /* shared memory variables & will not be updated */
    distMatrix = (double **)mmap(NULL, cityNum * sizeof(double *), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    for(int i = 0; i < cityNum; i++) {
        distMatrix[i] = (double *)mmap(NULL, cityNum * sizeof(double), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    }

    /* shared memory variables & will be updated */
    routes = (int **)mmap(NULL, childNum * sizeof(int *), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    for(int i = 0; i < childNum; i++) {
        routes[i] = (int *)mmap(NULL, cityNum * sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    }

    lengths = (double *)mmap(NULL, childNum * sizeof(double), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    finish = (int *)mmap(NULL, childNum * sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    passGranted = (int *)mmap(NULL, childNum * sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    swap1s = (int *)mmap(NULL, childNum * sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    swap2s = (int *)mmap(NULL, childNum * sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    /* main process local variables */
    cities = (struct CITY *)malloc(sizeof(struct CITY) * cityNum);
    for(int i = 0; i < cityNum; i++) {
        fscanf(file, "%lf", &cities[i].x);
        fscanf(file, "%lf", &cities[i].y);
    }
    fclose(file);

    buildDistMatrix(cityNum, cities, distMatrix);

    route = (int *)malloc(sizeof(int) * cityNum);
    /* initialize route as 0-1-2-3-4--- */
    for(int i = 0; i < cityNum; i++) {
        route[i] = i; /* initialize route as 0-1-2-3-4--- */
    } 
    lastLength = length = calLength(cityNum, distMatrix, route); 
    printf("length %f\n", length);

    /* initialize shared memory variables */
    for(int i = 0; i < childNum; i++) {
        copyRoute(cityNum, routes[i], route);
        lengths[i] = length;
    }

    for(int i = 0; i < childNum; i++) {
        finish[i] = 0;
        passGranted[i] = 0;
    }

    for (int i = 0; i < childNum; i++) {
        pid = fork();
        if(pid == 0) { 
            /* child process */
            id = i;
            isMain = 0;
            break;
        }
        if(pid > 0) {
            /* parent process */
            /* do nothing */
        }
        if(pid == -1) {
            /* fail to create child process */
            cout << "Error in fork()" << endl; 
            return 1; 
        }
    }

    if(isMain) {
        /* parent process */
        while(1) {

            /* wait until all child process finish */
            while(!canPass(childNum, finish)) {
                /* do nothing */
            }

            /* what you wanna do in parent process */
            lastLength = length;
            bestId = findBestchild(lengths, childNum, swap1s, swap2s);
            copyRoute(cityNum, route, routes[bestId]);
            length = lengths[bestId];
            
            for(int i = 0; i < childNum; i++) {
                copyRoute(cityNum, routes[i], route);
                lengths[i] = length;
            }
            printf("length %f\n", length);

            /* reset synchronize variable */
            for(int j = 0; j < childNum; j++) {
                finish[j] = 0;
                passGranted[j] = 1;
            }

            /* terminal condition */
            if(lastLength - length < 1) return 0;
        }
    }
    else {
        /* child process */
        while(1) {

            /* what you wanna do in child process */
            greedySearch_fork(id, cityNum, distMatrix, routes[id], &lengths[id], childNum, &swap1s[id], &swap2s[id]); 
            finish[id] = 1;

            /* wait until all child process finish */
            while(!passGranted[id]) {
                /* do nothing */
            }

            /* reset synchronize variable */
            passGranted[id] = 0;
        }
    }

    for(int i = 0; i < cityNum; i++) {
        munmap(distMatrix[i], cityNum * sizeof(double));
    }
    munmap(distMatrix, cityNum * sizeof(double *));
    for(int i = 0; i < childNum; i++) {
        munmap(routes[i], cityNum * sizeof(int));
    }
    munmap(routes, childNum * sizeof(int *));
    munmap(lengths, childNum * sizeof(double));
    munmap(finish, childNum * sizeof(int));
    munmap(passGranted, childNum * sizeof(int));
    munmap(swap1s, childNum * sizeof(int));
    munmap(swap2s, childNum * sizeof(int));
    return 0;
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

void greedySearch_fork(int id, int cityNum, double **distMatrix, int *route, double *length, int childNum, int *swap1, int *swap2) 
{
    double delta = 0;
    double bestDelta = 0;
    int start = 0;
    int end = 0;
    int beforeStart = 0; 
    int afterEnd = 0; 
    int bestStart = 0;
    int bestEnd = 0;
    
    /* reverse a segment of a route */
    for(int start = id; start < cityNum; start = start + childNum) {
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

int copyRoute(int cityNum, int *destRoute, int *srcRoute) 
{
    for(int i = 0; i < cityNum; i++) {
        destRoute[i] = srcRoute[i];
    }
    return 0;
}

int findBestchild(double *lengths, int childNum, int *swap1s, int *swap2s) 
{
    int bestId = 0;
    int bestSwap1 = 0;
    int bestSwap2 = 0;
    double bestLength = lengths[0];

    for(int i = 1; i < childNum; i++) {
        if(lengths[i] < bestLength) {
            bestLength = lengths[i];
            bestId = i;
        }
    }

    bestSwap1 = swap1s[bestId];
    bestSwap2 = swap2s[bestId];

    /* if multiple routes have same length , choose the one with smaller swap index */
    for(int i = 0; i < childNum; i++) {
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

int canPass(int childNum, int *finish) {
    int sum = 0;
    for(int i = 0; i < childNum; i++) {
        if(finish[i] > 0) sum ++;
    }
    if(sum == childNum) return 1;
    else return 0;
}
