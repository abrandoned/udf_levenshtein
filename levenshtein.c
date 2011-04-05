#ifdef STANDARD
  #include <string.h>
  #include <stdio.h>
  #include <stdlib.h>

  #ifdef __WIN__
    typedef unsigned __int64 ulonglong;
    typedef __int64 longlong;
  #else
    typedef unsigned long long ulonglong;
    typedef long long longlong;
  #endif /* __WIN__*/
#else
  #include <my_global.h>
  #include <my_sys.h>
#endif

#include <mysql.h>
#include <m_ctype.h>
#include <m_string.h>

#ifndef TRUE
  #define TRUE 1  
  #define FALSE 0
#endif

// Used for buffer allocator vs heap
#define LV_BUF_SIZE 256
#define LV_PAD_SIZE 2

// Used to swap calc and work grids while looping
#define LV_SWAP(t, a, b) { t = a; a = b; b = t;}
#define LV_MIN(a, b, c) ((a <= b) ? ((a <= c) ? a : c) : ((b <= c) ? b : c)); 

#ifdef HAVE_DLOPEN
// using a structure that will be filled on init with the variables that will makeup the distance constraints
typedef struct{
  char* a; 
  char* b; 
  int a_length;
  int b_length;
  longlong maximum_allowable_distance;
} LevenConstraints;

my_bool levenshtein_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
  // Check for arguments passed and determine max allowable distance
  switch(args->arg_count){
    case 2:
        if(args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT)
        {
          strcpy(message, "2 arg levenshtein() requires 2 string");
          return 1;
        }
      break;
    case 3:
        if(args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT || args->arg_type[2] != INT_RESULT)
        {
          strcpy(message, "3 arg levenshtein() requires 2 strings and an int");
          return 1;
        }        
      break;
    default:
        strcpy(message, "levenshtein() requires 2 or 3 arguments");
        return 1;
      break;
  }
  
  // Maximum length of return is 4 digits and won't be NULL
  initid->max_length = 4;
  initid->maybe_null = 0; 
  initid->const_item = 0;
  
  // Allocate the memory for working on the distance in shared workspace
  char *alloc_mem = malloc(sizeof(int) * (2 * LV_BUF_SIZE + (2 * LV_PAD_SIZE)));
    
  if (alloc_mem == NULL)
  {
    strcpy(message, "Cannot allocate memory for levenshtein()");
    return 1;
  }
   
  memset(alloc_mem, '\0', (sizeof(int) * (2 * LV_BUF_SIZE + (2 * LV_PAD_SIZE))));
  // store shared memory with mysql to hand it off to the executing function
  initid->ptr = alloc_mem;
  return 0;
}

void levenshtein_deinit(UDF_INIT *initid)
{
  if (initid->ptr != NULL)
  {
    free(initid->ptr);
  }
}

longlong levenshtein_intern(LevenConstraints* leven)
{
  // Check the minimum distance to keep from calculating anything when it isn't really needed
  // Efficient use of the levenshtein call includes a maximum_allowable_distance (or distance threshold)
  if ((leven->b_length - leven->a_length) > leven->maximum_allowable_distance || (leven->a_length == 0 || leven->b_length == 0))
  {
    return (longlong)leven->b_length;
  }
  
  // no reason to run levenshtein when equal
  if (leven->a_length == leven->b_length && strcmp(leven->a, leven->b) == 0)
  {
    return (longlong)0;
  }
  
  leven->a_length++;
  leven->b_length++;

  int x, *grid_odd, *grid_even, i, j, cost, row_min, distance, *work_grid, *calc_grid, *tmp;
  unsigned int broke_max = FALSE;
  
  grid_even = malloc(sizeof(int) * (leven->a_length));
  grid_odd = malloc(sizeof(int) * (leven->a_length));
  
  if(grid_even == NULL || grid_odd == NULL)
  {
    return (longlong)9999;   // error occured - cannot allocate memory
  }  

  work_grid = grid_odd;
  calc_grid = grid_even;

  for(x = 0; x < leven->a_length; x++)
	  grid_even[x] = x;
  	  
  for(i = 1; i < leven->b_length; i++)
  {     
    row_min = work_grid[0] = calc_grid[0] + 1;
    
    for(j = 1; j < leven->a_length; j++)
    {
      cost = (leven->a[j-1] == leven->b[i-1]) ? 0 : 1;
      work_grid[j] = LV_MIN(calc_grid[j]+1, work_grid[j-1]+1, calc_grid[j-1] + cost);
      row_min = (work_grid[j] < row_min) ? work_grid[j] : row_min;            
    }
    
    if(row_min > leven->maximum_allowable_distance)
    { 
      broke_max = TRUE; 
      break;
    }
    
    LV_SWAP(tmp, work_grid, calc_grid);
  }
  
  distance = (broke_max == TRUE) ? (leven->b_length - 1) : calc_grid[leven->a_length-1];  
  
  free(grid_odd);
  free(grid_even);  
  
  return (longlong) distance;
}

longlong levenshtein_extern(char* a, char* b, longlong max_distance)
{
  int a_len = (a == NULL) ? 0 : strlen(a);
  int b_len = (b == NULL) ? 0 : strlen(b);

  LevenConstraints* leven = malloc(sizeof(LevenConstraints));
  leven->a = (a_len > b_len) ? b : a;
  leven->b = (a_len > b_len) ? a : b;
  leven->a_length = strlen(leven->a);
  leven->b_length = strlen(leven->b);
  
  longlong distance = leven->b_length;
  
  if(max_distance < 0)
  {
    max_distance = (a_len > b_len) ? a_len : b_len;;
  }

  leven->maximum_allowable_distance = max_distance;
  
  distance = levenshtein_intern(leven);  
  free(leven);
  
  return distance;
}

longlong levenshtein(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
  longlong distance = 9999;

  if (args->args[0] != NULL && args->args[1] != NULL)
  {
    unsigned int used_heap = FALSE;
    int shortest_arg = (args->lengths[0] < args->lengths[1]) ? 0 : 1;
    int longest_arg = (shortest_arg == 0) ? 1 : 0;
        
    LevenConstraints* leven = malloc(sizeof(LevenConstraints));
          
    if(args->lengths[0] < LV_BUF_SIZE && args->lengths[1] < LV_BUF_SIZE)
    {
      // If the string lengths are < LV_BUF_SIZE then they can use the allocation already provided in init
      char* shared_buffera = &initid->ptr[0];
      char* shared_bufferb = &initid->ptr[LV_BUF_SIZE + LV_PAD_SIZE - 1];
      memset(shared_buffera, '\0', args->lengths[shortest_arg]+1);
      memset(shared_bufferb, '\0', args->lengths[longest_arg]+1);
      memcpy(shared_buffera, args->args[shortest_arg], args->lengths[shortest_arg]);
      memcpy(shared_bufferb, args->args[longest_arg], args->lengths[longest_arg]);
      leven->a = shared_buffera;
      leven->b = shared_bufferb;
    }
    else
    {
      // Need to dynamically allocate space for the arguments in this row (may take longer)
      // note: in simple benchmarks the dynamic allocation only adds 8-10 seconds over a corpus of 48 million words (i.e. impact is minimal)
      char* h_bufa = calloc(args->lengths[shortest_arg]+1, sizeof(char));
      char* h_bufb = calloc(args->lengths[longest_arg]+1, sizeof(char));
      memcpy(h_bufa, args->args[shortest_arg], args->lengths[shortest_arg]);
      memcpy(h_bufb, args->args[longest_arg], args->lengths[longest_arg]);    
      leven->a = h_bufa;
      leven->b = h_bufb;
      used_heap = TRUE;
    }
    
    leven->a_length = args->lengths[shortest_arg];
    leven->b_length = args->lengths[longest_arg];
    
    if (args->arg_count == 3)
    {
      leven->maximum_allowable_distance = *((longlong*)args->args[2]);
    }
    else
    {
      leven->maximum_allowable_distance = (leven->a_length > leven->b_length) ? leven->a_length : leven->b_length;
    }
    
    distance = levenshtein_intern(leven);
    
    if (used_heap == TRUE)
    {
      free(leven->a);
      free(leven->b);
    }
    
    free(leven);
  }
  else
  {
    int a_len = (args->args[0] == NULL) ? 0 : args->lengths[0];
    int b_len = (args->args[1] == NULL) ? 0 : args->lengths[1];
    distance = (a_len > b_len) ? a_len : b_len;
  }
  
  if(distance == 9999)
  {
    // an error occurred, either can't allocate memory or something went very wrong
    // setting error to 1 tells mysql to not call this udf for subsequent rows in the query (something is wrong)
    *error = 1;
  }
  
  return distance;
}

#endif /* HAVE_DLOPEN */
