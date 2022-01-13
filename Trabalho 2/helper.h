#ifndef _HELPER__H_
#define _HELPER__H_

/**
 * @brief Counts how many times a char occurs on an array of chars
 * 
 * @param string array of chars to iterate
 * @param count_char char to count occurrences
 * @return unsigned int number of occurences 
 */
unsigned int count_occurrences(char *string, char count_char);

/**
 * @brief Prints an array of chars
 * 
 * @param arr array of chars to print
 */
void print_array(char *arr);

#endif