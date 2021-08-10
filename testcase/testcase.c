/* parsing data from tsplib */

#include <stdio.h>
#include <stdlib.h>

int main() 
{
    char str[20];
    int city_num;
    FILE * file;
    file = fopen( "pr1002.txt" , "r");

    if(file == NULL) return 0; 
    
    city_num = 1002;
    printf("%d\n", city_num);

    for(int i = 0; i < city_num; i++) {
        fscanf(file, "%s", str);
        fscanf(file, "%s", str);
        printf("%s ", str);
        fscanf(file, "%s", str);
        printf("%s \n", str);
    }
    fclose(file);
}
