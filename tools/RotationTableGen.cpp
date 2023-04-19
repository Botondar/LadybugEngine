#include <cmath>
#include <cstdio>

#define PI 3.14159265359

int ShuffleTable[16] = 
{
    3, 7, 14, 12,
    0, 9, 15, 4, 
    8, 5, 11, 2,
    13, 6, 10, 1,
};

int main()
{
    for (int i = 0; i < 16; i++)
    {
        printf("vec2(%f, %f),\n", 
               cos(2.0 * ShuffleTable[i] * PI / 16.0), 
               sin(2.0 * ShuffleTable[i] * PI / 16.0));
    }
    return 0;
}
