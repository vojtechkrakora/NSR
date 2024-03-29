#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <mpi.h>

#include "char_operations.h"
#include "nsr_stack.h"
#include "proc_com.h"

char *generate_string(int length, char fill_char)
{
    char *new_string = (char *) malloc(length + 1);
    memset(new_string, fill_char, length);
    new_string[length] = 0;
    return new_string;
}

void all_words_rec(char input[], nsr_strings_t *strings,
        int input_length, int idx, nsr_result_t *result)
{
    int nchars = 'z' - 'a' + 1;
    int tmp_dist = 0;

    if (idx == input_length)
        return;

    input[idx] = 'a';

    while (nchars--)
    {
        tmp_dist = get_maximum_dist(strings, input);
        if (tmp_dist < result->_max_distance)
        {
            result->_max_distance = tmp_dist;
            memcpy(result->_string, input, input_length);
            set_distances(strings, input, result);
        }

        if (idx == input_length - 1)
            printf("%s [%d]\n", input,tmp_dist);
        all_words_rec(input, strings, input_length, idx + 1, result);
        input[idx]++;
    }
}


nsr_result_t *nsr_solve(const nsr_strings_t *strings)
{
   char *tmp_str;
   char *rec_string = (char *) malloc((strings->_min_string_length+1)*sizeof(char));
   nsr_stack_t stack;
   nsr_stack_elem_t elem;
   nsr_result_t *result;
   int tmp_dist, min_dist = INT_MAX; 
   int my_rank = 0,i = 0, token = WHITE, proc_num = 0, delay_counter = 0;
   int donor = 0;
   int debug_counter = 0, token_rec = TOKEN_NOT_REC;
   double start_time = 0, end_time = 0;
   
    /* find out process rank */
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
   
   /* find out number of processes */
   MPI_Comm_size(MPI_COMM_WORLD, &proc_num);
   
   /* init donor and local counter */
   donor = (my_rank+1)%proc_num;
   
   /* Stack code */
   result = (nsr_result_t *) malloc(sizeof(nsr_result_t));
   nsr_result_init(result, strings);
   nsr_stack_init(&stack);

   tmp_str = generate_string(strings->_min_string_length, 'a');
   tmp_str[0] = 'a';/* TODO this looks redundant */
   
   
   /* Only the first process will start calculation */
   if(my_rank == 0)
   {
       MPI_Barrier(MPI_COMM_WORLD);
       start_time = MPI_Wtime();
        nsr_stack_push(&stack, -1, tmp_str, strings->_min_string_length);
   }
   

   /* All processes (except 0) are waiting for work from proc. 0 */
   if(my_rank != 0)
   {
      MPI_Barrier(MPI_COMM_WORLD);
      proc_com_ask_for_work(&stack,donor,strings,&token,result,debug_counter,
              token_rec);
      donor = acz_ahd(my_rank,donor,proc_num);
   }
   
  
   while(!nsr_stack_empty(&stack))
   {
       elem = nsr_stack_pop(&stack);
  
       /* Do not add to stack */
       if(elem._idx+1 == strings->_min_string_length)
       {
           debug_counter++;
         /* check distances */
         tmp_dist = get_maximum_dist(strings, elem._string);
         if (tmp_dist < min_dist)
         {
            min_dist = tmp_dist;
            memcpy(result->_string, elem._string, strings->_min_string_length + 1);
            result->_max_distance = tmp_dist;
         }
         if(my_rank != 0 && nsr_stack_empty(&stack))
         {
            proc_com_ask_for_work(&stack,donor,strings,&token, result,
                    debug_counter,token_rec);
            token_rec = TOKEN_NOT_REC;
            donor = acz_ahd(my_rank,donor,proc_num);
         }
         
         if(my_rank == 0 && nsr_stack_empty(&stack))
         {
             for(i = 0; i < proc_num; i++)
                if(proc_com_zero_ask_for_work(&stack,strings,i))
                    continue;
         }
         /* get next elem from stack */
         continue;
       }
       memcpy(tmp_str,elem._string,strings->_min_string_length+1);
       
       for(i = 0; i < (CHARS_IN_ALPHABET+1); i++)
       {
           tmp_str[elem._idx+1] = 'z' - i;
           nsr_stack_push(&stack,elem._idx+1,tmp_str,strings->_min_string_length);
       }
       
       proc_com_check_flag(&stack, &token,delay_counter++, 
               strings->_min_string_length +1,my_rank,proc_num,&token_rec);
   }
   
   if(my_rank != 0)
   {
       printf("Accidentaly at the end, stack empty is %d.\n",nsr_stack_empty(&stack));
       printf("But that means i got result:\n");
       printf("Result string is %s and dist is %d.\n",result->_string,
               result->_max_distance);
   }
   /* Sorry, but i needed to delete these free(tmp_string) but dont know why */
   nsr_stack_destroy(&stack);
   printf("[%d] I have done %d things.\n",my_rank,debug_counter);
   if(my_rank == 0)
   {
       proc_com_check_idle_state(my_rank,proc_num);
       proc_com_finish_processes(strings->_min_string_length,result,strings);
       set_distances(strings, result->_string, result);
   }
   free(rec_string);
   
   end_time = MPI_Wtime();
   printf("Start_time %f, end_time %f.\n",start_time, end_time);
   printf("Result time is %f.\n",end_time-start_time);
    return result;
}


nsr_result_t *mpi_nsr_solve(const nsr_strings_t *strings)
{
    return NULL; 
}


int hamming_dist(const char *str1, const char *str2)
{
    const char *shorter, *longer;
    int i, min_dist = INT_MAX, shifts, shift_dist;

    int str1_len = strlen(str1);
    int str2_len = strlen(str2);

    longer = str1;
    shorter = str2;

    if (str2_len > str1_len)
    {
        longer = str2;
        shorter = str1;
    }

    shifts = abs(str1_len - str2_len) + 1;
    for (i = 0; i < shifts; i++)
    {
        shift_dist = 0;
        str2 = shorter;
        for (str1 = longer + i; *str2 && shift_dist < min_dist; str1++, str2++)
            shift_dist += *str1 != *str2;
        if (shift_dist < min_dist)
            min_dist = shift_dist;
    }
    return min_dist;
}


int get_maximum_dist(const nsr_strings_t *strings, const char *input)
{
    int i = 0;
    int maximum_dist = 0;
    int tmp_dist = 0;

    for(i = 0; i < strings->_count; i++)
    {
        tmp_dist = hamming_dist(strings->_strings[i],input);
        if(maximum_dist < tmp_dist)
                maximum_dist = tmp_dist;
    }

    return maximum_dist;
}

void set_distances(const nsr_strings_t *strings, const char *input,
        nsr_result_t *result)
{
    int i = 0;

    for(i = 0; i < strings->_count; i++)
    {
        result->_distances[i] = hamming_dist(strings->_strings[i],
                input);
    }
}
