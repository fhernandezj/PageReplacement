#include<stdio.h>
#include<stdlib.h>
#include<string.h>

struct Node {
	unsigned data;
	struct Node* link;
	_Bool dirty;
};

//Definition of the global variables needed
//for the execution of the algorithm
struct Node* ptr = NULL;
struct Node* last = NULL;

struct Node* disk_ptr = NULL;

struct Node* ptr_A = NULL;
struct Node* last_A = NULL;
struct Node* ptr_B = NULL;
struct Node* last_B = NULL;
struct Node* ptr_clean = NULL;
struct Node* last_clean = NULL;
struct Node* ptr_dirty = NULL;
struct Node* last_dirty = NULL;

struct Node* memory_ptr = NULL;

int NUM_FRAMES = 3;
char ALG[50];
_Bool PRINT=0;
FILE * file;

int frames = 0;
int frames_A = 0;
int frames_B = 0;

char fifo[] = "fifo";
char lru[] = "lru";
char vms[] = "vms";
char debug[] = "debug";
char quiet[] = "quiet";

int total_oper = 0;
int reads = 0;
int writes = 0;
int hits = 0;
int misses = 0;

//Method to enqueue an element to a given queue
void Enqueue(unsigned x, _Bool dirty, struct Node **ptr, struct Node **last) {
	struct Node* temp = (struct Node*)malloc(sizeof(struct Node));
	temp->data = x;
	temp->link = NULL;
	temp->dirty = dirty;
	if(*ptr == NULL && *last == NULL){
		*ptr = *last = temp;
		return;
	}
	(*last)->link = temp;
	*last = temp;
}

//Method to save a page into the disk or into the memory
void Save(unsigned x, struct Node **ptr){
	struct Node* temp = (struct Node*)malloc(sizeof(struct Node));
	temp->data = x;
	temp->link = NULL;
	if(*ptr == NULL){
		*ptr = temp;
	}else{
		temp->link = *ptr;
		*ptr = temp;
	}
}

//Method to dequeue an element from a given queue
struct Node* Dequeue(struct Node** ptr, struct Node** last) {
	struct Node* temp = *ptr;
	if(*ptr == NULL) {
		return NULL;
	}
	if(*ptr == *last) {
		*ptr = *last = NULL;
	}else {
		*ptr = (*ptr)->link;
	}
	return temp;
}

//Method to remove a given element from the disk or the memory
_Bool Find_Remove_List(unsigned x, struct Node **ptr){
	struct Node* temp = *ptr;
	struct Node* prev = NULL;
	while(temp != NULL){
		if(temp->data == x){
			if(prev != NULL){
				prev->link = temp->link;
			}else{
				*ptr = temp->link;
			}
			free(temp);
			return 1;
		}
		prev = temp;
		temp = temp->link;
	}
	return 0;
}

//Function to determine if a given element belongs to 
//a VMS queue and update its dirty/clean property
_Bool Find_VMS(unsigned x, _Bool Dirty, struct Node **ptr){
	struct Node* temp = *ptr;
    while(temp != NULL){
        if(temp->data == x){
			if(!temp->dirty && Dirty) temp->dirty = Dirty;
			return 1;
        }
        temp = temp->link;
    }
    return 0;
}

//Method to remove a given element from the clean or the dirty list
//checking the clean list first
void Remove_CD(unsigned x){
	struct Node* temp = ptr_clean;
	struct Node* prev = NULL;
	while(temp != NULL){
		if(temp->data == x){
			if(temp == ptr_clean){
                if(temp == last_clean){
                    ptr_clean = NULL;
                    last_clean = NULL;
                }else{
                    ptr_clean = ptr_clean->link;
                }
			}else if(temp == last_clean){
			    if(prev == ptr_clean){
                    last_clean = ptr_clean;
			    }else{
			        last_clean = prev;
			    }
			}else{
			    prev->link = temp->link;
            }
			free(temp);
			if(PRINT) printf("Page %x removed from Clean List\n", x);
			return;
		}
		prev = temp;
		temp = temp->link;
	}
	temp = ptr_dirty;
	prev = NULL;
	while(temp != NULL){
		if(temp->data == x){
			if(temp == ptr_dirty){
                if(temp == last_dirty){
                    ptr_dirty = NULL;
                    last_dirty = NULL;
                }else{
                    ptr_dirty = ptr_dirty->link;
                }
			}else if(temp == last_dirty){
			    if(prev == ptr_dirty){
                    last_dirty = ptr_dirty;
			    }else{
			        last_dirty = prev;
			    }
			}else{
			    prev->link = temp->link;
            }
			free(temp);
			if(PRINT) printf("Page %x removed from Dirty List\n", x);
			return;
		}
		prev = temp;
		temp = temp->link;
	}
	if(PRINT) printf("The page wasnt found\n");
}

//Method to insert a given page to the memory requested by Process A
//with the VMS algorithm
void Insert_VMS_A(unsigned x, _Bool dirty){
	if(PRINT) printf("Requested by Process A\n");
	if(Find_VMS(x, dirty, &ptr_A)){
		if(PRINT) printf("Hit in FIFO(A)\n");
		if(PRINT) printf("Hit in Memory\n");
		hits++;
	}else{
		if(PRINT) printf("Miss in FIFO(A)\n");
		if(Find_Remove_List(x, &disk_ptr)){
			reads++;
			if(PRINT) printf("Page %x is in the disk\n", x);
		}
		if(frames_A == NUM_FRAMES/2){
			struct Node* temp = Dequeue(&ptr_A, &last_A);
			if(PRINT) printf("Evict page %x from FIFO(A)\n", temp->data);
			if(temp->dirty){
				Enqueue(temp->data, temp->dirty, &ptr_dirty, &last_dirty);
				if(PRINT) printf("Page %x saved to Dirty List\n", temp->data);
			}else{
				Enqueue(temp->data, temp->dirty, &ptr_clean, &last_clean);
				if(PRINT) printf("Page %x saved to Clean List\n", temp->data);
			}
			Enqueue(x, dirty, &ptr_A, &last_A);
			free(temp);
		}else{
			Enqueue(x, dirty, &ptr_A, &last_A);
			frames_A++;
		}
		if(Find_VMS(x, dirty, &memory_ptr)){
			if(PRINT) printf("Hit in Memory\n");
			Remove_CD(x);
			hits++;
		}else{
			misses++;
			if(PRINT) printf("Miss in Memory\n");
			if(frames == NUM_FRAMES){
				struct Node* temp = Dequeue(&ptr_clean, &last_clean);
				if(temp == NULL){
					temp = Dequeue(&ptr_dirty, &last_dirty);
					writes++;
					if(PRINT) printf("Page %x must be saved to disk\n", temp->data);
					Save(temp->data, &disk_ptr);
				}
				if(PRINT) printf("Evict page %x from Memory\n", temp->data);
				Find_Remove_List(temp->data, &memory_ptr);
				free(temp);
				Save(x, &memory_ptr);
			}else{
				Save(x, &memory_ptr);
				frames++;
			}
		}
	}
}

//Method to insert a given page to the memory requested by Process B
//with the VMS algorithm
void Insert_VMS_B(unsigned x, _Bool dirty){
	if(PRINT) printf("Requested by Process B\n");
	if(Find_VMS(x, dirty, &ptr_B)){
		if(PRINT) printf("Hit in FIFO(B)\n");
		if(PRINT) printf("Hit in Memory\n");
		hits++;
	}else{
		if(PRINT) printf("Miss in FIFO(B)\n");
		if(Find_Remove_List(x, &disk_ptr)){
			reads++;
			if(PRINT) printf("Page %x is in the disk\n", x);
		}
		if(frames_B == NUM_FRAMES/2){
			struct Node* temp = Dequeue(&ptr_B, &last_B);
			if(PRINT) printf("Evict page %x from FIFO(B)\n", temp->data);
			if(temp->dirty){
				Enqueue(temp->data, temp->dirty, &ptr_dirty, &last_dirty);
				if(PRINT) printf("Page %x saved to Dirty List\n", temp->data);
			}else{
				Enqueue(temp->data, temp->dirty, &ptr_clean, &last_clean);
				if(PRINT) printf("Page %x saved to Clean List\n", temp->data);
			}
			Enqueue(x, dirty, &ptr_B, &last_B);
			free(temp);
		}else{
			Enqueue(x, dirty, &ptr_B, &last_B);
			frames_B++;
		}
		if(Find_VMS(x, dirty, &memory_ptr)){
			if(PRINT) printf("Hit in Memory\n");
			Remove_CD(x);
			hits++;
		}else{
			misses++;
			if(PRINT) printf("Miss in Memory\n");
			if(frames == NUM_FRAMES){
				struct Node* temp = Dequeue(&ptr_clean, &last_clean);
				if(temp == NULL){
					temp = Dequeue(&ptr_dirty, &last_dirty);
					writes++;
					if(PRINT) printf("Page %x must be saved to disk\n", temp->data);
					Save(temp->data, &disk_ptr);
				}
				if(PRINT) printf("Evict page %x from Memory\n", temp->data);
				Find_Remove_List(temp->data, &memory_ptr);
				free(temp);
				Save(x, &memory_ptr);
			}else{
				Save(x, &memory_ptr);
				frames++;
			}
		}
	}
}

//Function to find a given element in the FIFO queue
//and update its clean/dirty property
_Bool Find_FIFO(unsigned x, _Bool dirty){
    struct Node* temp = ptr;
    while(temp != NULL){
        if(temp->data == x){
			if(PRINT) printf("Hit\n");
			if(!temp->dirty && dirty) temp->dirty = dirty;
			if(PRINT) printf("This page is %s\n", temp->dirty ? "dirty" : "clean");
            return 1;
        }
        temp = temp->link;
    }
    return 0;
}

//Method to insert a given page to the memory with the FIFO algorithm
void Insert_FIFO(unsigned x, _Bool dirty){
    if(Find_FIFO(x, dirty)){
		hits++;
    }else{
		if(PRINT) printf("Miss\n");
		if(PRINT) printf("This page is %s\n", dirty ? "dirty" : "clean");
		misses++;
        if(frames==NUM_FRAMES){
            struct Node* temp = Dequeue(&ptr, &last);
			if(PRINT) printf("Evict %x \n", temp->data);
			if(temp->dirty){
				if(PRINT) printf("Page %x must be saved to disk\n", temp->data);
				Save(temp->data, &disk_ptr);
				writes++;
			}
			free(temp);
			if(Find_Remove_List(x, &disk_ptr)){
				reads++;
				if(PRINT) printf("Page %x is in the disk\n", x);
			}
            Enqueue(x, dirty, &ptr, &last);
        }else{
            Enqueue(x, dirty, &ptr, &last);
			frames++;
        }
    }
}

//Function to find a given element in the LRU queue, if its found it
//should be moved to the last position of the queue. Its dirty/clean
//property is also updated
_Bool Find_LRU(unsigned x, _Bool dirty){
    struct Node* temp = ptr;
    struct Node* prev = NULL;
    while(temp != NULL){
        if(temp->data == x){
			if(temp->dirty) dirty=temp->dirty;
			if(PRINT) printf("Hit\n");
			if(PRINT) printf("This page is %s\n", dirty ? "dirty" : "clean");
			if(ptr==last) return 1;
			if(temp==ptr){
				ptr = temp->link;
				free(temp);
				Enqueue(x, dirty, &ptr, &last);
			}else if(temp!=last){
				prev->link = temp->link;
				free(temp);
				Enqueue(x, dirty, &ptr, &last);
			}
            return 1;
        }
        prev = temp;
        temp = temp->link;
    }
    return 0;
}

//Method to insert a given page to the memory with the LRU algorithm
void Insert_LRU(unsigned x, _Bool dirty){
    if(Find_LRU(x, dirty)){
		hits++;
    }else{
        if(PRINT) printf("Miss\n");
		if(PRINT) printf("This page is %s\n", dirty ? "dirty" : "clean");
		misses++;
        if(frames==NUM_FRAMES){
            struct Node* temp = Dequeue(&ptr, &last);
			if(PRINT) printf("Evict %x \n", temp->data);
			if(temp->dirty){
				if(PRINT) printf("Page %x must be saved to disk\n", temp->data);
				Save(temp->data, &disk_ptr);
				writes++;
			}
			free(temp);
			if(Find_Remove_List(x, &disk_ptr)){
				reads++;
				if(PRINT) printf("Page %x is in the disk\n", x);
			}
            Enqueue(x, dirty, &ptr, &last);
        }else{
            Enqueue(x, dirty, &ptr, &last);
			frames++;
        }
    }
}

//Main method of the FIFO algorithm, in wich reads each memory access
//and inserts the desired page in the FIFO queue, showing lastly the results
void FIFO(){
	unsigned addr;
	char rw;
	while(fscanf(file, "%x %c", &addr, &rw)>0){
		addr = addr >> 12;
		_Bool dirty = 0;
		if(rw=='W') dirty = 1;

		if(PRINT) printf("Page referenced: %x\n", addr);
		if(PRINT) printf("Operation: %c\n", rw);

		Insert_FIFO(addr, dirty);
		total_oper++;
		if(PRINT) printf("\n");
	}
	printf("total memory frames: %d\n", NUM_FRAMES);
	printf("events in trace: %d\n", total_oper);
	printf("total disk reads: %d\n", reads);
	printf("total disk writes: %d\n", writes);
	double rate = (double)hits/((double)(hits+misses));
	//printf("Hit rate: %.4f\n", rate);
}

//Main method of the LRU algorithm, in wich reads each memory access
//and inserts the desired page in the LRU queue, showing lastly the results
void LRU(){
	unsigned addr;
	char rw;
	while(fscanf(file, "%x %c", &addr, &rw)>0){
		addr = addr >> 12;
		_Bool dirty = 0;
		if(rw=='W') dirty = 1;

		if(PRINT) printf("Page referenced: %x\n", addr);
		if(PRINT) printf("Operation: %c\n", rw);

		Insert_LRU(addr, dirty);
		if(PRINT) printf("\n");
		total_oper++;
	}
	printf("total memory frames: %d\n", NUM_FRAMES);
	printf("events in trace: %d\n", total_oper);
	printf("total disk reads: %d\n", reads);
	printf("total disk writes: %d\n", writes);
	double rate = (double)hits/((double)(hits+misses));
	//printf("Hit rate: %.4f\n", rate);
}

//Main method of the VMS algorithm, in wich reads each memory access
//requested by a certain process and inserts the desired page in the memory, 
//showing lastly the results
void VMS(){
	unsigned addr;
	char rw;
	while(fscanf(file, "%x %c", &addr, &rw)>0){
		addr = addr >> 12;
		_Bool dirty = 0;
		if(rw=='W') dirty = 1;

		if(PRINT) printf("Page referenced: %x\n", addr);
		if(PRINT) printf("Operation: %c\n", rw);

		unsigned first_digit = addr >> 16;
		if(first_digit == 3){
			Insert_VMS_A(addr, dirty);
		}else{
			Insert_VMS_B(addr, dirty);
		}

		if(PRINT) printf("\n");
		total_oper++;
	}
	printf("total memory frames: %d\n", NUM_FRAMES);
	printf("events in trace: %d\n", total_oper);
	printf("total disk reads: %d\n", reads);
	printf("total disk writes: %d\n", writes);
	double rate = (double)hits/((double)(hits+misses));
	//printf("Hit rate: %.4f\n", rate);
}

//Main execution of the program
int main(int argc, char *argv[]){
	file = fopen(argv[1], "r");
	NUM_FRAMES = atoi(argv[2]);
	strcpy(ALG, argv[3]);

	if(strcmp(argv[4], quiet)==0){
		PRINT=0;
	}else{
		PRINT=1;
	}

	if(strcmp(ALG, fifo)==0){
		FIFO();
	}else if(strcmp(ALG, lru)==0){
		LRU();
	}else if(strcmp(ALG, vms)==0){
		VMS();
	}
}
