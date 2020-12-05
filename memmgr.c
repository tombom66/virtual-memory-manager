#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256
#define PAGE_SIZE 256
#define TLB_SIZE 16
#define PHYSICAL_MEMORY_SIZE PAGE_SIZE * FRAME_SIZE

int address_count = 0; // keeps track of how many addresses have been read
int Hit = 0; 
int tlbSize = 0; //size of the tlb
int tlbHitCount = 0; //used to count how many tlb hits there are
int Frame = 0;
int pageFaultCount = 0; // counts the amounts of page faults
float pageFaultRate;
float tlbHitRate;

//-------------------------------------------------------------------
unsigned getpage(unsigned x) { return (0xff00 & x) >> 8; }

unsigned getoffset(unsigned x) { return (0xff & x); }

void getpage_offset(unsigned x) {
  unsigned  page   = getpage(x);
  unsigned  offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, paddress: %u\n", x, page, offset, (page << 8) | getoffset(x), page * 256 + offset);
}

struct tlbTable{
  unsigned int pageNum;
  unsigned int frameNum;
};

int main(int argc, const char* argv[]) {
  FILE* fadd = fopen("addresses.txt", "r");    // open file addresses.txt  (contains the logical addresses)
  if (fadd == NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n");  exit(FILE_ERROR);  }

  FILE* fcorr = fopen("correct.txt", "r");     // contains the logical and physical address, and its value
  if (fcorr == NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n");  exit(FILE_ERROR);  }

  FILE* fbin = fopen("backing_store.bin", "rb"); // Will read the binary storage file
  if (fbin == NULL) { fprintf(stderr, "Could not open file : 'backing_store.bin'\n"); exit(FILE_ERROR); }

  char buf[BUFLEN];
  int physicalMemory[PHYSICAL_MEMORY_SIZE];
  unsigned   page, offset, physical_add, frame = 0;
  unsigned   logic_add;                  // read from file address.txt
  unsigned   virt_add, phys_add, value;  // read from file correct.txt

  // Declare and initialize page table to -1
  int pageTable[PAGE_SIZE];
  memset(pageTable, -1, 256*sizeof(int));

  //Declare and initialize tlb[] structure to -1
  struct tlbTable tlb[TLB_SIZE];
  memset(pageTable, -1, 16 * sizeof(char));

  while (fscanf(fadd, "%d", &logic_add) == 1) { // will read every line from address.txt
    address_count++;

    page = getpage(logic_add);
    offset = getoffset(logic_add);
    Hit = -1;

    //This will check to see if the page number is already in tlb
    //If it is in tlb, then tlb hit will increase
    for(int i = 0; i < tlbSize; i++)
      if (tlb[i].pageNum == page){
        Hit = tlb[i].frameNum;
        physical_add = Hit * FRAME_SIZE + offset;
      }
    if(!(Hit == -1))
      tlbHitCount++;

    //Checking for tlb miss
    //Will get physical page number from table
    else if (pageTable[page] == -1){
      fseek(fbin, page*256, SEEK_SET);
      fread(buf, sizeof(char), BUFLEN, fbin);
      pageTable[page] = Frame;

      for(int i = 0; i < 256; i++)
        physicalMemory[Frame*256 + i] = buf[i];

      pageFaultCount++;
      Frame++;

      if(tlbSize == 16)
        tlbSize--;

      for(int i = tlbSize; i > 0; i--){
        tlb[i].pageNum = tlb[i - 1].pageNum;
        tlb[i].frameNum = tlb[i - 1].frameNum;
      }

      if(tlbSize <= 15)
        tlbSize++;

      tlb[0].pageNum = page;
      tlb[0].frameNum = pageTable[page];
      physical_add = pageTable[page]*256 + offset;
    }
    else
      physical_add = pageTable[page]*256 + offset;

    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add, buf, buf, &phys_add, buf, &value);  // read from file correct.txt
    
    assert(physical_add == phys_add);

    // Reads backing_store.bin and confirms that it is the same value in correct.txt 
    fseek(fbin, logic_add, SEEK_SET);
    char c;
    fread(&c, sizeof(char), 1, fbin);
    int val = (int)(c);
    assert(val == value);
    //printf("Value from backing_store.txt is %d whereas value from correct.txt is %d \n", val, value);

    printf("logical: %5u (page: %3u, offset: %3u) ---> physical: %5u -- passed\n", logic_add, page, offset, physical_add);
    if (frame % 5 == 0) { printf("\n"); }

    //The statistics of the program
    pageFaultRate = pageFaultCount*1.0f / address_count;
    tlbHitRate = tlbHitCount*1.0f /address_count;

  }
  fclose(fbin);
  fclose(fcorr);
  fclose(fadd);

  printf("Now to print some statistics of the program \n");
  printf("Number of Addresses %d \n", address_count);
  printf("Number of Page Faults %d \n", pageFaultCount);
  printf("The Page Fault Rate is %f \n", pageFaultRate);
  printf("The amount of TLB Hits is %d \n", tlbHitCount);
  printf("The TLB Hit Rate is %f \n", tlbHitRate);
  return 0;
}
