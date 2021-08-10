/* gcc outputData.c -lm -o outputData */
/* time ./outputData ../testcase/pr1002.txt > outputData.txt */
/* generate city position data after optimization */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

struct CITY {
    double x;
    double y;
};

void buildDistMatrix(int city_num, struct CITY *cities, double **distMatrix);
double calLength(int city_num, double **distMatrix, int *route);
void tsp2opt(int *route, int best_reverse_start, int best_reverse_end);
double greedySearch(int cityNum, double** distMatrix, int *route, double length);

int main(int argc, char *argv[]) 
{
    FILE * file = NULL;
    char fileName[20] = "";
    double **distMatrix = NULL;
    int cityNum = 0;
    double optimalLength = 0;
    struct CITY *cities = NULL;
    double length = 0;
    double lastLength = 0;
    int *route = NULL;

    strcpy(fileName, argv[1]);
    file = fopen(argv[1] , "r");
    if(file == NULL) return 0; 
    
    fscanf(file, "%d", &cityNum);
    fscanf(file, "%lf", &optimalLength);

    cities = (struct CITY *)malloc(sizeof(struct CITY) * cityNum);
    route = (int *)malloc(sizeof(int) * cityNum);
    distMatrix = (double **)malloc(sizeof(double *) * cityNum);
    for(int i = 0; i < cityNum; i++) {
        distMatrix[i] = (double *)malloc(sizeof(double) * cityNum);
    }

    for(int i = 0; i < cityNum; i++) {
        fscanf(file, "%lf", &cities[i].x);
        fscanf(file, "%lf", &cities[i].y);
    }
    fclose(file);

    buildDistMatrix(cityNum, cities, distMatrix);

    /* initialize route as 0-1-2-3-4--- */
    for(int i = 0; i < cityNum; i++) {
        route[i] = i;
    } 

    lastLength = length = calLength(cityNum, distMatrix, route); 

    while(1) {
        lastLength = length;
        length = greedySearch(cityNum, distMatrix, route, length); 
        if(lastLength - length < 1) break;
    }

    for(int i = 0; i < cityNum; i++) {
        printf("%lf %lf\n", cities[route[i]].x, cities[route[i]].y);
    } 

    free(cities);
    free(route) ;
    for(int i = 0; i < cityNum; i++) {
        free(distMatrix[i]);
    }
    free(distMatrix);
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

double greedySearch(int cityNum, double** distMatrix, int *route, double length) 
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
    for(int start = 0; start < cityNum; start++) {
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

    /* update route */
    tsp2opt(route, bestStart, bestEnd);
    return length + bestDelta;
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


