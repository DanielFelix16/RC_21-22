#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helper.h"

unsigned int count_occurrences(char *string, char count_char)
{
    unsigned int i = 0;
    unsigned int res = 0;

    while (string[i] != '\0')
    {
        if (string[i] == count_char)
        {
            res++;
        }
        i++;
    }

    return res;
}

void print_array(char *arr)
{
    unsigned int i = 0;

    while (arr[i] != '\0')
    {
        printf("%c", arr[i]);
        i++;
    }

    printf("\n");
}