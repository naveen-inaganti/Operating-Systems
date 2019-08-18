//wriiten by kiran including initial load
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "simos.h"

// need to be consistent with paging.c: mType and constant definitions
#define opcodeShift 24
#define operandMask 0x00ffffff
#define diskPage -2

FILE *progFd;

//==========================================
// load program into memory and build the process, called by process.c
// a specific pid is needed for loading, since registers are not for this pid
//==========================================

// may return progNormal or progError (the latter, if the program is incorrect)
int load_instruction (mType *buf, int page, int offset)
{ 
int IRopcode,IRoperand,instr;
 instr=buf[offset].mInstr;
  IRopcode=instr >> opcodeShift;
  IRopcode=IRopcode << opcodeShift;
  IRoperand=instr & operandMask;
 printf("Load instruction (%x, %d) into M(%d, %d)\n",IRopcode,IRoperand,page,offset);
 // load instruction to buffer
return (mNormal);
}

int load_data (mType *buf, int page, int offset)
{ 
    float data;
     data=buf[offset].mData;
     printf("Load data: %.2f into M(%d, %d)\n",data,page,offset);
// load data to buffer (same as load instruction, but use the mData field
return (mNormal);
}

// load program to swap space, returns the #pages loaded
int load_process_to_swap (int pid, char *fname)
{ 
   int i,ret,npages,j;
   FILE *fprog;
   int msize, numinstr, numdata,opcode,operand;
   float data;
   mType *buf;
   fprog = fopen (fname, "r");
    if (fprog == NULL)
  { printf ("Submission Error: Incorrect program name: %s!\n", fname);
    return progError;
  }
   init_process_pagetable(pid);
   ret = fscanf (fprog, "%d %d %d\n", &msize, &numinstr, &numdata);
    if (ret < 3)   // did not get all three inputs
  { printf ("Submission failure: missing %d program parameters!\n", 3-ret);
    return progError;
  }
   printf("Program info: %d %d %d\n",msize,numinstr,numdata);
   npages=ceil((float)msize/pageSize);
   //printf("values is :%d",npages);
   if(npages>maxPpages)
   return(progError);
   for(i=0;i<npages;i++)
   {
    buf = (mType *) malloc (pageSize*sizeof(mType));
    bzero(buf,sizeof(buf));
    for(j=i*pageSize;j<(i+1)*pageSize;j++)
    {
      
    if((j+1)<=numinstr)
    {
    fscanf (fprog, "%d %d\n", &opcode, &operand);
    opcode = opcode << opcodeShift;
    operand = operand & operandMask;
    
    //printf("Load instruction (%x, %d) into M(%d, %d)",opcode,operand,i,(j%pageSize));
    //printf("hellow see me :%x , %d\n",opcode | operand,j%pageSize);
    
    buf[j%pageSize].mInstr = opcode | operand;
    load_instruction(buf,  i, j%pageSize);
//printf("Load instruction (%x, %d) into M(%d, %d)",opcode,operand,i,(j%pageSize));
    }
    else if((j+1)<=msize)
    { 
      fscanf (fprog, "%f\n", &data);
      //printf("Load data: %.2f into M(%d, %d)",data,i,(j%pageSize));
      buf[j%pageSize].mData = data;
      load_data(buf, i, j%pageSize);
      //printf("Load instruction (%x, %d) into M(%d, %d)",opcode,operand,i,(j%pageSize));

     }
}
update_process_pagetable (pid, i, diskPage);

insert_swapQ (pid,i, buf, actWrite, freeBuf); 
   
}
//signal_semaphore(npages);
return (npages);

// read from program file "fname" and call load_instruction & load_data
  // to load the program into the buffer, write the program into
  // swap space by inserting it to swapQ
  // update the process page table to indicate that the page is not empty
  // and it is on disk (= diskPage)
}

int load_pages_to_memory (int pid, int numpage)
{
   dump_process_pagetable(pid);
   int npages,i,frameno,frameoff=0,addr;
   mType *buf;
   npages=loadPpages>numpage?numpage:loadPpages;
   
   //i=0;
   for(i=0;i<(npages-1);i++)
   {

     loading_memory_from_loader(pid,i,actRead,Nothing);
      
     //frameno=get_free_frame();
     //printf("Got free frame = %d\n",frameno);
     //update_frame_info (frameno, pid, i);
     //dump_memoryframe_info();
     //update_process_pagetable (pid, i, frameno);
     
     //buf = (mType *) malloc (pageSize*sizeof(mType));
     //addr=(frameoff & pageoffsetMask) | (frameno << pagenumShift);
     //insert_swapQ(pid,i,buf,actRead,Nothing);
     
     //PCB[CPU.Pid]->numPF=(PCB[CPU.Pid]->numPF)+1;
   
   }

     //frameno=get_free_frame();
     //printf("Got free frame = %d\n",frameno);
     //update_frame_info (frameno, pid, i);
     //dump_memoryframe_info();
     //update_process_pagetable (pid, i, frameno);
     
      //addr=(frameoff & pageoffsetMask) | (frameno << pagenumShift);
     //buf = (mType *) malloc (pageSize*sizeof(mType));
     loading_memory_from_loader(pid,i,actRead,toReady);
    
     //PCB[CPU.Pid]->numPF=(PCB[CPU.Pid]->numPF)+1;
     //insert_swapQ(pid,i,buf,actRead,toReady);
  return (npages);   
// call insert_swapQ to load the pages of process pid to memory
  // #pages to load = min (loadPpages, numpage = #pages loaded to swap for pid)
  // ask swap.c to place the process to ready queue only after the last load
  // do not forget to update the page table of the process
  // this function has some similarity with page fault handler
}

int load_process (int pid, char *fname)
{ int ret;

  ret = load_process_to_swap (pid, fname);   // return #pages loaded
  if (ret != progError) {ret=load_pages_to_memory (pid, ret);}
  return (ret);
}

// load idle process, idle process uses OS memory
// We give the last page of OS memory to the idle process
#define OPifgo 5   // has to be consistent with cpu.c
void load_idle_process ()
{ int page, frame;
  int instr, opcode, operand, data;

  init_process_pagetable (idlePid);
  page = 0;   frame = OSpages - 1;
  update_process_pagetable (idlePid, page, frame);
  update_frame_info (frame, idlePid, page);
  
  // load 1 ifgo instructions (2 words) and 1 data for the idle process
  opcode = OPifgo;   operand = 0;
  instr = (opcode << opcodeShift) | operand;
  direct_put_instruction (frame, 0, instr);   // 0,1,2 are offset
  direct_put_instruction (frame, 1, instr);
  direct_put_data (frame, 2, 1);
}


