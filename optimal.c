#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
    int* page_table;//array for page table
    int replace_index = 0;//index for what page we replace in page table
    int page_index = 0;//variable for what page we reference
    long int page_fault = 0;
    char* file;
    char* line;
    size_t len = 0;
    int num_pages;
    int page_size;
    long int addresses = 0;//variable for how many addresses we are gonna reference
    int check = 0;//variable to check if we have a page fault or hit
    int* ref_check;//array to check farthest away page we will reference in page table
    int* references;//array for all references we will make
    long int ref_count = 0;//index for where we are in the references array
    int ref_page_count = 0;//when ref_page_count = num_pages-1 we will have a variable in ref_check that is = 0,
			   //which means that the index that variable has is the index for the farthest away page in page_table

    if(argc == 4){
      file = malloc(strlen(argv[3])+1);
      strcpy(file, argv[3]);//file name
      num_pages = atoi(argv[1]);//number of pages
      page_size = atoi(argv[2]);// page size
      page_table = malloc(sizeof(int)*num_pages);//page table
      ref_check = malloc(sizeof(int)*num_pages);//farthest page reference away
      for(int i = 0; i < num_pages; i++){
	       page_table[i] = -1;
	       ref_check[i] = 0;
      }

      FILE *f;

      printf("No physical pages = %d, page size = %d\n", num_pages, page_size);
      f = fopen(file, "r");
      if(f == NULL){
        printf("Can't open file.\n");
        exit(EXIT_FAILURE);
      }
      while(getline(&line, &len, f) != -1)
	      addresses += 1;//get number of references for malloc
      fclose(f);

      references = malloc(sizeof(int)*addresses);
      f = fopen(file, "r");
      if(f == NULL){
	       printf("Can't open file.\n");
	       exit(EXIT_FAILURE);
      }
      while(getline(&line, &len, f) != -1){
	       references[ref_count] = atoi(line)/page_size;//fill array with all references that will be made
	       ref_count += 1;
      }
      fclose(f);
      if(line)
	      free(line);

      ref_count = 0;
      for(int i = 0; i < addresses; i++){
	      for(int k = 0; k < num_pages; k++){
	         if(references[i] == page_table[k]){//if referenced page is in page table
		           check++;
		           k = num_pages;
	         }
	      }
	      if(check == 0){//page fault
	         page_fault++;
	         for(int k = 0; k < num_pages; k++){
		          if(page_table[k] == -1){//if there is an empty spot in page table
		              replace_index = k;
		              check++;
		              k = num_pages;
		          }
	         }
	         if(check == 0){//no empty spot
		           for(int k = ref_count+1; k < addresses; k++){//search for farthest away page to replace
		               if(ref_page_count == num_pages-1)//found farthest away page
			                k = addresses;
		               else{
			                for(int p = 0; p < num_pages; p++){
			                     if(page_table[p] == references[k]){//found a nearby reference for that page
				                        if(ref_check[p] == 0){//if that page is not yet referenced
				                              ref_check[p] += 1;
				                              ref_page_count += 1;
				                        }
				                        p = num_pages;
			                     }
			                 }
		              }
		      }
		        for(int k = 0; k < num_pages; k++){
		            if(ref_check[k] == 0){
			               replace_index = k;
			               k = num_pages;
		            }
		        }
	     }
	     page_table[replace_index] = references[i];
	}
	for(int k = 0; k < num_pages; k++)
	   ref_check[k] = 0;//reset the ref_check array for the next page replacement
  check = 0;
	ref_page_count = 0;
	ref_count += 1;
}

  printf("Reading memory trace from %s... Read %ld memory references\n", file, addresses);
  free(file);
  free(page_table);
  free(ref_check);
  free(references);

  printf("Result: %ld page faults.\n", page_fault);
}
else
   printf("Wrong number of arguments\n");
return 0;
}
