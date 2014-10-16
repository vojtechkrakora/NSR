/******************************************************************************/
/* MI-PPR.2 - NSR                                                             */
/****************                                                             */
/* @author krakovoj                                                           */
/* @author tatarsan                                                           */
/******************************************************************************/
#include <stdio.h>
#include <tgmath.h>
#include <stdlib.h>
#include <limits.h>

#include "char_operations.h"
#include "nsr_string.h"
#include "nsr_io.h"

/*
int main(int argc, char *argv[]) {
    int result;
    char * input2 = generate_string(3);

    printf("Generated char is %s \n", input2);
    result = log(TESTING_INPUT_SIZE);
    printf("log(%d) = %d\n", TESTING_INPUT_SIZE, result);

    all_words_rec(input2,3,0);

    printf("%d\n", hamming_dist("abcd", "aec"));

    free(input2);
    return 0;
}
*/

int main(int argc, char **argv)
{
   FILE *input;
   nsr_strings_t *strings;
   nsr_result_t *result;
   int i = 0;
   strings = (nsr_strings_t *) malloc(sizeof(nsr_strings_t));
   result = (nsr_result_t *) malloc(sizeof(nsr_result_t));
   if (argc != 2)
   {
      fprintf(stderr, "usage: %s input_file\n", argv[0]);
      return EXIT_FAILURE;
   }

   input = fopen(argv[1], "r");

   nsr_read_strings(input, strings);
   char * compareString = generate_string(strings->_min_string_length);
   nsr_result_init(result, strings);
   nsr_strings_print(strings);

   all_words_rec(compareString,strings,strings->_min_string_length,0,INT_MAX,result);
   printf("Result string is \'%s\' with total distance %d.\n",result->_string,
           result->_total_distance);
   for(i = 0; i < strings->_count; i++)
   {
       printf(" hamming_dist(%s,%s) = %d\n",strings->_strings[i],
               result->_string,result->_distances[i]);
   }

   nsr_strings_destroy(strings);
   free(strings);
   free(result->_distances);
   free(result->_string);
   free(result);

   fclose(input);

   return EXIT_SUCCESS;
}
