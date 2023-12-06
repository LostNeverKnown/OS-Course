#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
    int* page_table;
    int first_index = 0;//index for the page that is first in and will be first out
    int page_index;//variable for what page in referenced
    long int page_fault = 0;
    char* file;
    char* line;
    size_t len = 0;
    int num_pages;
    int page_size;
    long int addresses = 0;
    int check = 0;//variable to check for when a page is not in the page_table

    if(argc == 4){
      file = malloc(strlen(argv[3])+1);
      strcpy(file, argv[3]);
      num_pages = atoi(argv[1]);
      page_size = atoi(argv[2]);
      page_table = malloc(sizeof(int)*num_pages);
      for(int i = 0; i < num_pages; i++)
	     page_table[i] = -1;

      FILE *f;

      printf("No physical pages = %d, page size = %d\n", num_pages, page_size);
      f = fopen(file, "r");
      if(f == NULL){
        printf("Can't open file.\n");
        exit(EXIT_FAILURE);
      }

      while(getline(&line, &len, f) != -1){//gets address from file
        page_index = atoi(line)/page_size;//what page is referenced
        for(int i = 0; i < num_pages; i++){
          if(page_index == page_table[i])//check if the page is already in page table
            check++;
        }
        if(check == 0){//not in page table
          page_fault++;
          page_table[first_index] = page_index;//replace the page that was first in
          first_index++;//next place in page_table array is the first in now
          first_index = first_index%num_pages;//so index does not go beyond page_table
        }
        check = 0;
	      addresses += 1;
      }
      fclose(f);
      printf("Reading memory trace from %s... Read %ld memory references\n", file, addresses);
      free(file);
      free(page_table);
      if(line)
	     free(line);

      printf("Result: %ld page faults.\n", page_fault);
    }
    else
      printf("Wrong number of arguments\n");
    return 0;
}
