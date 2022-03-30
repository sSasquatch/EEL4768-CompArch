#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

int main(int argc, char **argv) {
	// check if the correct amount of values were passed
	if (argc < 5) {
		printf("Usage: <CACHE_SIZE> <ASSOC> <REPLACEMENT> <WB> <TRACE_FILE>\n");
		return -1;
	}
	
	// get all of the values passed to the program
	int cache_size = atoi(argv[1]);
	//printf("cache_size = %d\n", cache_size);
	int assoc = atoi(argv[2]);
	//printf("assoc = %d\n", assoc);
	int replc = atoi(argv[3]);
	//printf("replc = %d\n", replc);
	int wb = atoi(argv[4]);
	//printf("wb = %d\n", wb);
	char trace_file[100];
	strcpy(trace_file, argv[5]);
	//printf("trace_file = %s\n", trace_file);
	FILE *trace;
	int block_size = 64;
	
	// basic variables
	int misses = 0, hits = 0, hit = 0;
	int mem_reads = 0, mem_writes = 0;
	int i, j, set;
	char current_line[20], op;
	long long int address;
	
	// calculate the number of sets
	int num_sets = cache_size / (assoc * block_size);
	//printf("num_sets = %d\n", num_sets);
	
	int pos, miss_pos, miss_index;
	
	// initialize all of the tags
	// also check if the allocations were successful
	long long int **tags = (long long int **) malloc(num_sets * sizeof(long long int *));
	if (tags == NULL) {
		printf("tags not allocated!\n");
		return -1;
	}
	for (i = 0; i < num_sets; i++) {
		tags[i] = (long long int *) malloc(assoc * sizeof(long long int));
		if (tags[i] == NULL) {
			printf("tags[%d] not allocated!\n", i);
			return -1;
		}
	}
	
	// initialize the queue array
	// also check if the allocation was successful
	int **queue = (int **) malloc(num_sets * sizeof(int *));
	if (queue == NULL) {
		printf("queue not allocated!\n");
		return -1;
	}
	for (i = 0; i < num_sets; i++) {
		queue[i] = (int *) malloc(assoc * sizeof(int));
		if (queue[i] == NULL) {
			printf("queue[%d] not allocated!\n", i);
			return -1;
		}
	}
	for (i = 0; i < num_sets; i++) {
		for (j = 0; j < assoc; j++) {
			queue[i][j] = j;
		}
	}
		
	// initialize the dirty array
	// also check if the allocation was successful
	int **dirty = (int **) malloc(num_sets * sizeof(int *));
	if (dirty == NULL) {
		printf("dirty not allocated!\n");
		return -1;
	}
	for (i = 0; i < num_sets; i++) {
		dirty[i] = (int *) malloc(assoc * sizeof(int));
		if (dirty[i] == NULL) {
			printf("dirty[%d] not allocated!\n", i);
			return -1;
		}
	}
	for (i = 0; i < num_sets; i++) {
		for (j = 0; j < assoc; j++) {
			dirty[i][j] = 0;
		}
	}
	
	// open the trace file for read
	trace = fopen(trace_file, "r");
	if (trace == NULL) {
		printf("trace_file could not be opened!\n");
		return -1;
	}
	
	// LRU
	if (replc == 0) {
		while (fgets(current_line, 20, trace) != NULL) {
			// scan the file for the operation and address
			sscanf(current_line, "%c %llx", &op, &address);
			//printf("op = %c; address = %llx\n", op, address);
			
			// calculate the set number of the current address
			set = (address / block_size) % num_sets;
			//printf("set # for %llx is %d\n", address, set);
			
			// set hit to false before each line
			hit = 0;
			
			// go through the set looking for a hit
			for (i = 0; i < assoc; i++) {
				// if it is a hit
				if ((address / (block_size * num_sets)) == tags[set][i]) {
					//printf("%llx is a hit\n", address);
					hit = 1;
					hits++;
					
					// set queue by incrementing all of the other values in the set
					// that are below the current address
					// then set current address to 0 (mru)
					pos = queue[set][i];
					for (j = 0; j < assoc; j++) {
						if (queue[set][j] < pos) {
							queue[set][j]++;
						}
					}
					queue[set][i] = 0;
					
					// write to memory if write-through (wb = 0)
					// set dirty bit if write-back (wb = 1)
					if (op == 'W') {
						if (wb == 0) {
							mem_writes++;
						}
						else if (wb == 1) {
							dirty[set][i] = 1;
						}
					}
					else if (op == 'R') {
						//mem_reads++;
					}
				}
			}
			// if it is a miss
			if (hit == 0) {
				//printf("%llx is a miss\n", address);
				misses++;
				
				// find the index that has been there the longest
				miss_index = 0;
				for (i = 0; i < assoc; i++) {
					if (queue[set][i] == (assoc - 1)) {
						miss_index = i;
					}
				}
				//printf("miss_index = %d\n", miss_index);
				
				// check if the queue index has a dirty bit
				// if so, write to memory
				if (dirty[set][miss_index] == 1) {
					dirty[set][miss_index] = 0;
					mem_writes++;
				}
				
				// set queue by incrementing all of the other values in the set
				// that are below the current address
				// then set current address to 0 (mru)
				//miss_pos = queue[set][miss_index];
				for (i = 0; i < assoc; i++) {
					//if (queue[set][i] < miss_pos) {
						queue[set][i]++;
					//}
				}
				queue[set][miss_index] = 0;
				
				// set the tag
				tags[set][miss_index] = address / (block_size * num_sets);
				//printf("tag for %llx is %llx\n", address, tags[set][miss_index]);
				
				// write to memory if write-through (wb = 0)
				// set dirty bit if write-back (wb = 1)
				if (op == 'W') {
					//printf("got to line 187\n");
					if (wb == 0) {
						mem_writes++;
						mem_reads++;
					}
					else if (wb == 1) {
						dirty[set][miss_index] = 1;
						mem_reads++;
					}
				}
				else if (op == 'R') {
					//printf("got to line 198\n");
					mem_reads++;
				}
			}
		}
	}
	// FIFO
	else if (replc == 1) {
		while (fgets(current_line, 20, trace) != NULL) {
			// scan the file for the operation and address
			sscanf(current_line, "%c %llx", &op, &address);
			//printf("op = %c address = %llx\n", op, address);
			
			// calculate the set number of the current address
			set = (address / block_size) % num_sets;
			
			// set hit to false before each line
			hit = 0;
			
			// go through the set looking for a hit
			for (i = 0; i < assoc; i++) {
				// if it is a hit
				if ((address / (block_size * num_sets)) == tags[set][i]) {
					hit = 1;
					hits++;
					
					// set queue by incrementing all of the other values in the set
					// that are below the current address
					// then set current address to 0 (mru)
					pos = queue[set][i];
					for (j = 0; j < assoc; j++) {
						if (queue[set][j] > pos) {
							queue[set][j]--;
						}
					}
					queue[set][i] = assoc - 1;
					
					// write to memory if write-through (wb = 0)
					// set dirty bit if write-back (wb = 1)
					if (op == 'W') {
						if (wb == 0) {
							mem_writes++;
						}
						else if (wb == 1) {
							dirty[set][i] = 1;
						}
					}
					else if (op == 'R') {
						//mem_reads++;
					}
				}
			}
			// if it is a miss
			if (hit == 0) {
				misses++;
				
				// find the index that is place 0
				miss_index = 0;
				for (i = 0; i < assoc; i++) {
					if (queue[set][i] == 0) {
						miss_index = i;
					}
				}
				
				// check if the queue index has a dirty bit
				// if so, write to memory
				if (dirty[set][miss_index] == 1) {
					dirty[set][miss_index] = 0;
					mem_writes++;
				}
				
				// set queue by decrementing all of the other values in the set
				// then set the 
				//miss_pos = queue[set][miss_index];
				for (i = 0; i < assoc; i++) {
					//if (queue[set][i] > 0) {
						queue[set][i]--;
					//}						
				}
				queue[set][miss_index] = assoc - 1;
				
				// set the tag
				tags[set][miss_index] = address / (block_size * num_sets);
				
				// write to memory if write-through (wb = 0)
				// set dirty bit if write-back (wb = 1)
				if (op == 'W') {
					if (wb == 0) {
						mem_writes++;
						mem_reads++;
					}
					else if (wb == 1) {
						dirty[set][miss_index] = 1;
						mem_reads++;
					}
				}
				else if (op == 'R') {
					mem_reads++;
				}
			}
		}
	}
	
	// print out the stats
	//printf("# misses = %d\n", misses);
	//printf("# hits = %d\n", hits);
	printf("Total Miss Ratio: %f\n", (float) misses / (hits + misses));
	printf("# Writes: %d\n", mem_writes);
	printf("# Reads: %d\n", mem_reads);
	
	for (i = 0; i < num_sets; i++) {
		free(tags[i]);
		free(queue[i]);
		free(dirty[i]);
	}
	free(tags);
	free(queue);
	free(dirty);
	
	fclose(trace);
	
	return 0;
}