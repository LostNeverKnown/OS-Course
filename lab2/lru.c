#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
    int* page_table;
    int replace_index = 0;//index for what page will be replaced
    int page_index;//variable for what page is referenced
    long int page_fault = 0;
    char* file;
    char* line;
    size_t len = 0;
    int num_pages;
    int page_size;
    long int addresses = 0;
    int check = 0;//check if referenced page is already in page table
    int* page_table_check;//array that represents a counter for pages in page table that tells us how long ago they were used

    if(argc == 4){
      file = malloc(strlen(argv[3])+1);
      strcpy(file, argv[3]);//file name
      num_pages = atoi(argv[1]);//number of pages
      page_size = atoi(argv[2]);// page size
      page_table = malloc(sizeof(int)*num_pages);//page table
      page_table_check = malloc(sizeof(int)*num_pages);//recently used table check
      for(int i = 0; i < num_pages; i++){
	       page_table[i] = -1;
	       page_table_check[i] = 0;
      }

      FILE *f;

      printf("No physical pages = %d, page size = %d\n", num_pages, page_size);
      f = fopen(file, "r");
      if(f == NULL){
        printf("Can't open file.\n");
        exit(EXIT_FAILURE);
      }

      while(getline(&line, &len, f) != -1){
        page_index = atoi(line)/page_size;//variable to what page that address is for
        for(int i = 0; i < num_pages; i++){
          if(page_index == page_table[i]){//check if page is in page table
            check++;
	          page_table_check[i] = 1;//page referenced is recently used
	          i = num_pages;
	  }
        }
        if(check == 0){//page is not in page table
          page_fault++;
	      for(int i = 0; i < num_pages; i++){
	        if(page_table[i] == -1){//there is an empty spot in page table
		          replace_index = i;
		          i = num_pages;
	    }
	    else{
	        if(i == 0)
		        replace_index = i;
	       else{
		         if(page_table_check[i] < page_table_check[replace_index])//if page on index i is less recently used than the page on current replace index
			          replace_index = i;
	        }
	     }
	  }
          page_table[replace_index] = page_index;
	        page_table_check[replace_index] = 1;//the new page is "recently used"
        }
	for(int i = 0; i < num_pages; i++)
	   page_table_check[i] -= 1;//all pages are less recently used except the new page, but the new page will also get -1 which doesnt matter for this to work
  check = 0;
	addresses += 1;
}

      fclose(f);
      printf("Reading memory trace from %s... Read %ld memory references\n", file, addresses);
      free(file);
      free(page_table);
      free(page_table_check);
      if(line)
	free(line);

      printf("Result: %ld page faults.\n", page_fault);
    }
    else
      printf("Wrong number of arguments\n");
    return 0;
}
