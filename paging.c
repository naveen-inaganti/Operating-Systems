//written Naveen Inaganti
#include <stdio.h>
#include <stdlib.h>
#include "simos.h"

// Memory definitions, including the memory itself and a page structure
// that maintains the informtion about each memory page
// config.sys input: pageSize, numFrames, OSpages
// ------------------------------------------------
// process page table definitions
// config.sys input: loadPpages, maxPpages

mType *Memory;   // The physical memory, size = pageSize*numFrames

typedef unsigned ageType;
typedef struct
{ int pid, page;   // the frame is allocated to process pid for page page
  ageType age;
  char free, dirty, pinned;   // in real systems, these are bits
  int next, prev;
} FrameStruct;

FrameStruct *memFrame;   // memFrame[numFrames]
int freeFhead, freeFtail;   // the head and tail of free frame list

// define special values for page/frame number
#define nullIndex -1   // free frame list null pointer
#define nullPage -1   // page does not exist yet
#define diskPage -2   // page is on disk swap space
#define pendingPage -3  // page is pending till it is actually swapped
   // have to ensure: #memory-frames < address-space/2, (pageSize >= 2)
   //    becuase we use negative values with the frame number
   // nullPage & diskPage are used in process page table 

// define values for fields in FrameStruct
#define zeroAge 0x00000000
#define highestAge 0x80
#define dirtyFrame 1
#define cleanFrame 0
#define freeFrame 1
#define usedFrame 0
#define pinnedFrame 1
#define nopinFrame 0
// define shifts and masks for instruction and memory address 
#define opcodeShift 24
#define operandMask 0x00ffffff

// shift address by pagenumShift bits to get the page number
unsigned pageoffsetMask;
int pagenumShift; // 2^pagenumShift = pageSize

//============================
// Our memory implementation is a mix of memory manager and physical memory.
// get_instr, put_instr, get_data, put_data are the physical memory operations
//   for instr, instr is fetched into registers: IRopcode and IRoperand
//   for data, data is fetched into registers: MBR (need to retain AC value)
// page table management is software implementation
//============================


//==========================================
// run time memory access operations, called by cpu.c
//==========================================

// define rwflag to indicate whehter the addr computation is for read or write
#define flagRead 1
#define flagWrite 2

// address calcuation are performed for the program in execution
// so, we can get address related infor from CPU registers
int calculate_memory_address (unsigned offset, int rwflag)
{ 
 //printf("came here :%d\n,addr");
 int pageno,frameoff,addr;
  pageno=offset/pageSize;
  frameoff=offset%pageSize;
  if(pageno>maxPpages)
     return(mError);
  else
    { 
       if(CPU.PTptr[pageno]==diskPage)
        {set_interrupt(pFaultException);
         return(mPFault);}
      else if(CPU.PTptr[pageno]==nullPage && rwflag==flagWrite) 
         {set_interrupt(pFaultException);
return(mPFault);}
      else if(CPU.PTptr[pageno]==nullPage && rwflag==flagRead)
         return(mError);
      else
         {//CPU.PTptr[pageno]

            //if(CPU.PTptr[pageno]==0 || CPU.PTptr[pageno]==1 || CPU.PTptr[pageno]>=numFrames)
             //return(mError);
            addr=(frameoff & pageoffsetMask) | (CPU.PTptr[pageno] << pagenumShift);
            memFrame[CPU.PTptr[pageno]].age=highestAge;
            /*if(memFrame[CPU.PTptr[pageno]].free==freeFrame)
            {   //printf("============= Frame got used after freed %\n",CPU.PTptr[pageno]);
                if(memFrame[CPU.PTptr[pageno]].prev==-1 && memFrame[CPU.PTptr[pageno]].next!=-1)
                {
                  freeFhead=memFrame[CPU.PTptr[pageno]].next;
                  memFrame[freeFhead].prev=-1;
                  memFrame[CPU.PTptr[pageno]].next=-1;
                  memFrame[CPU.PTptr[pageno]].prev=-1;
                }
                else if(memFrame[CPU.PTptr[pageno]].next==-1 && memFrame[CPU.PTptr[pageno]].prev!=-1)
                {
                  freeFtail=memFrame[CPU.PTptr[pageno]].prev;
                  memFrame[freeFtail].next=-1;
                  memFrame[CPU.PTptr[pageno]].next=-1;
                  memFrame[CPU.PTptr[pageno]].prev=-1;
                }
                else if(memFrame[CPU.PTptr[pageno]].next==-1 && memFrame[CPU.PTptr[pageno]].prev==-1)
                {
                   freeFtail=-1;
                    freeFhead=-1;
                    memFrame[CPU.PTptr[pageno]].next=-1;
                    memFrame[CPU.PTptr[pageno]].prev=-1;
                 } 
                else{
                    int temp,temp1;
                     temp=memFrame[CPU.PTptr[pageno]].next;
                     temp1=memFrame[CPU.PTptr[pageno]].prev;
                    memFrame[temp1].next=temp;
                    memFrame[temp].prev=temp1;
                     memFrame[CPU.PTptr[pageno]].next=-1;
                memFrame[CPU.PTptr[pageno]].prev=-1;
                }
                
   
            }*/
            return(addr);
         }
    }


// rwflag is used to differentiate the caller
  // different access violation decisions are made for reader/writer
  // if there is a page fault, need to set the page fault interrupt
  // also need to set the age and dirty fields accordingly
  // returns memory address or mPFault or mError
}

int get_data (int offset)

{ 
  
int maddr; 

  maddr = calculate_memory_address(offset,flagRead);
  if(maddr==mPFault)
   {//CPU.exeStatus=ePFault;
   page_fault_check=-8;
   return(mPFault);
}
   else if(maddr==mError)
   return(mError);
  else
  { 
    
    CPU.MBR = Memory[maddr].mData;
    return (mNormal);
  }


// call calculate_memory_address to get memory address
  // copy the memory content to MBR
  // return mNormal, mPFault or mError
}

int put_data (int offset)

{ 
  int maddr,pageno,frameoff; 
  pageno=offset/pageSize;
  frameoff=offset%pageSize;
  maddr = calculate_memory_address(offset,flagWrite);
  if(maddr==mPFault)
  {
   CPU.exeStatus=ePFault;
   page_fault_check=-8;
   return(mPFault);
  }
   else if(maddr==mError)
   return(mError);
  else
 { 
memFrame[CPU.PTptr[pageno]].dirty=dirtyFrame;
Memory[maddr].mData = CPU.AC;//why not cpu.ac?

    return (mNormal);
  }


// call calculate_memory_address to get memory address
  // copy MBR to memory 
  // return mNormal, mPFault or mError
}

int get_instruction (int offset)

{ 
 int maddr,instr;
   maddr=calculate_memory_address(offset,flagRead);
   if(maddr==mPFault)
{   //CPU.exeStatus=ePFault;
   page_fault_check=-7;
   return(mPFault);
}
   else if(maddr==mError)
   return(mError);
   else{
     instr = Memory[maddr].mInstr;
    CPU.IRopcode = instr >> opcodeShift; 
    CPU.IRoperand = instr & operandMask;
    return (mNormal);

   }
// call calculate_memory_address to get memory address
  // convert memory content to opcode and operand
  // return mNormal, mPFault or mError
}

// these two direct_put functions are only called for loading idle process
// no specific protection check is done
void direct_put_instruction (int findex, int offset, int instr)
{ int addr = (offset & pageoffsetMask) | (findex << pagenumShift);
  Memory[addr].mInstr = instr;
}

void direct_put_data (int findex, int offset, mdType data)
{ int addr = (offset & pageoffsetMask) | (findex << pagenumShift);
  Memory[addr].mData = data;
}

//==========================================
// Memory and memory frame management
//==========================================

void dump_one_frame (int findex)
{ 
 int i,addr;
printf("start-end:%d,%d: ",(findex*pageSize),((findex+1)*pageSize));
for(i=0;i<pageSize;i++)
{  
   addr=(i & pageoffsetMask) | (findex << pagenumShift);
   printf("%x ",Memory[addr]);
} 
printf("\n");
//dump_process_swap_page(memFrame[findex].pid,memFrame[findex].page);



// dump the content of one memory frame
}

void dump_memory ()
{ int i;

  printf ("************ Dump the entire memory\n");
  for (i=0; i<numFrames; i++) dump_one_frame (i);
}

// above: dump memory content, below: only dump frame infor

void dump_free_list ()
{ 
int i=0,temp;
   printf("******************** Memory Free Frame Dump\n");
   temp=freeFhead;
while(temp!=-1)
{
  printf("%d, ",temp);
   temp=memFrame[temp].next;
   i++;
   if(i%8==0)
   printf("\n"); 
}   


/*for(i=0;i<numFrames-OSpages;i++)
   {
   if(i%8==0)
   printf("\n");     
      if(memFrame[i+OSpages].free==freeFrame)
      printf("%d, ",i+OSpages);
}  */
printf("\n");
// dump the list of free memory frames
}

void print_one_frameinfo (int indx)
{ printf ("pid/page/age=%d,%d,%x, ",
          memFrame[indx].pid, memFrame[indx].page, memFrame[indx].age);
  printf ("dir/free/pin=%d/%d/%d, ",
          memFrame[indx].dirty, memFrame[indx].free, memFrame[indx].pinned);
  printf ("next/prev=%d,%d\n",
          memFrame[indx].next, memFrame[indx].prev);
}

void dump_memoryframe_info ()
{ int i;

  printf ("******************** Memory Frame Metadata\n");
  printf ("Memory frame head/tail: %d/%d\n", freeFhead, freeFtail);
  for (i=OSpages; i<numFrames; i++)
  { printf ("Frame %d: ", i); print_one_frameinfo (i); }
  dump_free_list ();
}

void  update_frame_info (findex, pid, page)//doubt
{
  


memFrame[findex].pid=pid;
memFrame[findex].page=page;
memFrame[findex].age=highestAge;
memFrame[findex].free=usedFrame;
memFrame[findex].dirty=cleanFrame;
//memFrame[findex].pinned=pinnedFrame;
memFrame[findex].next=-1;
memFrame[findex].prev=-1;
// update the metadata of a frame, need to consider different update scenarios
  // need this function also becuase loader also needs to update memFrame fields
  // while it is better to not to expose memFrame fields externally
}

// should write dirty frames to disk and remove them from process page table
// but we delay updates till the actual swap (page_fault_handler)
// unless frames are from the terminated process (status = nullPage)
// so, the process can continue using the page, till actual swap
void addto_free_frame (int findex, int status)//doubt
{
//if(findex==0)
//printf("i cmae from here\n");

if(status==nullPage)
{

    memFrame[findex].age = zeroAge;
    memFrame[findex].dirty = cleanFrame;
    memFrame[findex].free = freeFrame;
    memFrame[findex].pinned = nopinFrame;
    memFrame[findex].pid = idlePid;
    memFrame[findex].page=nullPage;
   if(freeFtail==freeFhead && freeFhead==-1)
   {
    memFrame[findex].prev=freeFtail;
    memFrame[findex].next=-1;
    freeFtail=findex;
    freeFhead=findex;
   }
   else
   {
    memFrame[findex].prev=freeFtail;
    memFrame[findex].next=-1;
    memFrame[freeFtail].next=findex;
    freeFtail=findex;
   }
}
else
{

    memFrame[findex].age = zeroAge;
    //memFrame[findex].dirty = cleanFrame;
    memFrame[findex].free = freeFrame;
    memFrame[findex].pinned = nopinFrame;
    //memFrame[findex].pid = idlePid;
    //memFrame[findex].page=nullPage;
    if(freeFtail==freeFhead && freeFhead==-1)
   {
    memFrame[findex].prev=freeFtail;
    memFrame[findex].next=-1;
    freeFtail=findex;
    freeFhead=findex;
   }
   else
   {
    memFrame[findex].prev=freeFtail;
    memFrame[findex].next=-1;
    memFrame[freeFtail].next=findex;
    freeFtail=findex;
   }
}
printf("Added free frame = %d\n",findex);

}
int select_agest_frame ()
{ 
  int low=memFrame[OSpages].age;
 int j;
for( j=OSpages;j<numFrames;j++)
{
  if(memFrame[j].age<low)
  {
    low=memFrame[j].age;
  }


}
int k,select=-1,count=0;
if(low==0)
{
for(k=OSpages;k<numFrames;k++)
{
if(memFrame[k].age==low)
  { if(count==0){
    select=k;
    count++;}
    else{
    count++;
    addto_free_frame(k,pendingPage);}
  }
}

}

else
{
for(k=OSpages;k<numFrames;k++)
{
if(memFrame[k].age==low)
  { 
    if(memFrame[k].dirty!=dirtyFrame)
    {if(count==0){
     select=k;
     count++;}
     else{
     addto_free_frame(k,pendingPage);
     count++;}
    }
  }
}


}
if(select==-1)
{
for(k=OSpages;k<numFrames;k++)
{
if(memFrame[k].age==low)
  { 
    select=k;
    //addto_free_frame(k,pendingPage);
    break;
  }
}



}
return(select);
// select a frame with the lowest age 
  // if there are multiple frames with the same lowest age, then choose the one
  // that is not dirty
}

int get_free_frame ()
{ 




  int i,frameno;

   freeFhead, freeFtail;
   if(freeFhead==freeFtail && freeFhead==-1)
   {
    frameno=select_agest_frame();
    printf("Selected agest frame = %d, age %x, dirty %d\n",frameno,memFrame[frameno].age,memFrame[frameno].dirty);
    
   }
   else if(freeFhead==freeFtail && freeFhead!=-1)
   {
    frameno=freeFhead;
    freeFhead=-1;
    freeFtail=-1;
    if(memFrame[frameno].age!=0)
    {printf("============= Frame got used after freed %d\n",frameno);
    printf("Selected agest frame = %d, age %x, dirty %d\n",frameno,memFrame[frameno].age,memFrame[frameno].dirty);}
   }
   else
   {
     frameno=freeFhead;
     i=memFrame[frameno].next;
     memFrame[i].prev=-1;
     freeFhead=i;
    if(memFrame[frameno].age!=0)
    {printf("============= Frame got used after freed %d\n",frameno);
    printf("Selected agest frame = %d, age %x, dirty %d\n",frameno,memFrame[frameno].age,memFrame[frameno].dirty);}
   }







   /*for(i=OSpages;i<numFrames;i++)
   {
    if(memFrame[i].free==freeFrame)
      {
      frameno=(i);         
      //return(frameno);
       break;
       }
     else
      frameno=-10;  
}
if(frameno==-10)
frameno=select_agest_frame();*/
return(frameno);
// get a free frame from the head of the free list 
// if there is no free frame, then get one frame with the lowest age
// this func always returns a frame, either from free list or get one with lowest age
} 

void initialize_memory ()
{ int i,j;
  
  // create memory + create page frame array memFrame 
  Memory = (mType *) malloc (numFrames*pageSize*sizeof(mType));
  memFrame = (FrameStruct *) malloc (numFrames*sizeof(FrameStruct));
  bzero(Memory,numFrames*pageSize*sizeof(mType));

  // compute #bits for page offset, set pagenumShift and pageoffsetMask
  // *** ADD CODE
   pagenumShift=log2(pageSize);
   pageoffsetMask=(numFrames*pageSize)-1;
  // initialize OS pages
  for (i=0; i<OSpages; i++)
  { memFrame[i].age = zeroAge;
    memFrame[i].dirty = cleanFrame;
    memFrame[i].free = usedFrame;
    memFrame[i].pinned = pinnedFrame;
    memFrame[i].pid = osPid;
  }
  // initilize the remaining pages, also put them in free list
  // *** ADD CODE
  for(i=OSpages;i<numFrames;i++)
  {

    memFrame[i].age = zeroAge;
    memFrame[i].dirty = cleanFrame;
    memFrame[i].free = freeFrame;
    memFrame[i].pinned = nopinFrame;
    memFrame[i].pid = nullPid;
    memFrame[i].page=nullPage;
    if(i==OSpages)
    {memFrame[i].next=i+1;
    memFrame[i].prev=-1;}
    else if(i==(numFrames-1))
    {
    memFrame[i].next=-1;
    memFrame[i].prev=i-1;}
    else
      {memFrame[i].next=i+1;
      memFrame[i].prev=i-1;}


}
freeFhead=OSpages;
freeFtail=numFrames-1;


}

//==========================================
// process page table manamgement
//==========================================

void init_process_pagetable (int pid)
{ int i;

  PCB[pid]->PTptr = (int *) malloc (addrSize*maxPpages);
  for (i=0; i<maxPpages; i++) PCB[pid]->PTptr[i] = nullPage;
}

// frame can be normal frame number or nullPage, diskPage
void update_process_pagetable (pid, page, frame)
int pid, page, frame;
{ 
 

PCB[pid]->PTptr[page] = frame;
printf("PT update for (%d, %d) to %d\n",pid,page,frame);
 // update the page table entry for process pid to point to the frame
  // or point to disk or null
}

int free_process_memory (int pid)
{ 
 int i; 
   printf("Free frames allocated to process %d\n",pid);
  for (i=0; i<maxPpages; i++){
   
  if(PCB[pid]->PTptr[i]!= nullPage && PCB[pid]->PTptr[i]!= diskPage && pid>1)
  {
  if(memFrame[PCB[pid]->PTptr[i]].free!=freeFrame)
  {
  addto_free_frame(PCB[pid]->PTptr[i],nullPage);
  }
  }
  PCB[pid]->PTptr[i] = nullPage; 
}
// free the memory frames for a terminated process
  // some frames may have already been freed, but still in process pagetable

return(mNormal);
}

void dump_process_pagetable (int pid)

{ 
int i;
   printf("************ Page Table Dump for Process %d\n",pid);
   for(i=0;i<maxPpages;i++)
   {
   if(i%8==0)
   printf("\n");     
   printf("%d, ",PCB[pid]->PTptr[i]);
} 
printf("\n");
// print page table entries of process pid
}

void dump_process_memory (int pid)

{ 
int i; 
printf("Process %d memory dump:\n",pid);
     
for(i=0;i<maxPpages;i++)
{
if(PCB[pid]->PTptr[i]== diskPage)
{printf("***P/F:%d,%d: ",i,PCB[pid]->PTptr[i]);
  dump_process_swap_page(pid,i);
}
else if(PCB[pid]->PTptr[i]!= nullPage && PCB[pid]->PTptr[i]!= diskPage)
{
printf("***P/F:%d,%d: ",i,PCB[pid]->PTptr[i]);
print_one_frameinfo(i);
dump_one_frame(PCB[pid]->PTptr[i]);
}
else
{
printf("***P/F:%d,%d: \n",i,nullPage);

}
}

// print out the memory content for process pid
}

//==========================================
// the major functions for paging, invoked externally
//==========================================

#define sendtoReady 1  // has to be the same as those in swap.c
#define notReady 0   
#define actRead 0   
#define actWrite 1

void page_fault_handler ()
{ 
int check_page=-1,check_pid=-1; 
int frameno,frameoff=0,addr;
frameno=get_free_frame();
printf("Got free frame = %d\n",frameno);
dump_memoryframe_info();
if(memFrame[frameno].dirty==dirtyFrame)
{ 
  //mType *buf,*buf1;
  //buf = (mType *) malloc (pageSize*sizeof(mType));
  //for(i=0;i<pageSize;i++)
//{  
   addr=(frameoff & pageoffsetMask) | (frameno << pagenumShift);
   //buf[i].mInstr=Memory[addr].mInstr;
   //buf[i].mData=Memory[addr].mData;
//}
  update_process_pagetable(memFrame[frameno].pid,memFrame[frameno].page,diskPage);
  insert_swapQ (memFrame[frameno].pid, memFrame[frameno].page, &Memory[addr], actWrite, Nothing);
  check_page=memFrame[frameno].page;
  check_pid=memFrame[frameno].pid;

}
else if(memFrame[frameno].pid != nullPid)
{
update_process_pagetable(memFrame[frameno].pid,memFrame[frameno].page,diskPage);
check_page=memFrame[frameno].page;
  check_pid=memFrame[frameno].pid;
}
//buf1 = (mType *) malloc (pageSize*sizeof(mType));
if(page_fault_check==-8)
{
update_frame_info (frameno, CPU.Pid, CPU.IRoperand/pageSize);
update_process_pagetable(CPU.Pid,CPU.IRoperand/pageSize,frameno);
addr=(frameoff & pageoffsetMask) | (frameno << pagenumShift);
  //Memory[addr]=(mType *)buf;
insert_swapQ(CPU.Pid,CPU.IRoperand/pageSize,&Memory[addr],actRead,toReady);
printf("Swap_in: in=(%d,%d,%x), out=(%d,%d,%x), m=%x\n",CPU.Pid,CPU.IRoperand/pageSize,&Memory[addr],check_pid,check_page,&Memory[addr],&Memory[0]);
printf("Page Fault Handler: pid/page=(%d,%d)\n",CPU.Pid,CPU.IRoperand/pageSize);
}
else{
update_frame_info (frameno, CPU.Pid, CPU.PC/pageSize);
update_process_pagetable(CPU.Pid,CPU.PC/pageSize,frameno);

addr=(frameoff & pageoffsetMask) | (frameno << pagenumShift);
  //Memory[addr]=(mType *)buf;
insert_swapQ(CPU.Pid,CPU.PC/pageSize,&Memory[addr],actRead,toReady);
printf("Swap_in: in=(%d,%d,%x), out=(%d,%d,%x), m=%x\n",CPU.Pid,CPU.PC/pageSize,&Memory[addr],check_pid,check_page,&Memory[addr],&Memory[0]);
printf("Page Fault Handler: pid/page=(%d,%d)\n",CPU.Pid,CPU.PC/pageSize);
}
//printf("page faults going wrong;%d\n",PCB[CPU.Pid]->numPF);
//PCB[CPU.Pid]->numPF=PCB[CPU.Pid]->numPF+1;
// handle page fault
  // obtain a free frame or get a frame with the lowest age
  // if the frame is dirty, insert a write request to swapQ 
  // insert a read request to swapQ to bring the new page to this frame
  // update the frame metadata and the page tables of the involved processes
}

// scan the memory and update the age field of each frame
void memory_agescan ()
{ 
int i;
for(i=OSpages;i<numFrames;i++)
{

memFrame[i].age=memFrame[i].age>>1;
if(memFrame[i].age==0 && memFrame[i].free!=freeFrame)
addto_free_frame(i,pendingPage);
}

}

void initialize_memory_manager ()
{ 

initialize_memory();
  // initialize memory and add page scan event request
add_timer (periodAgeScan, osPid, actAgeInterrupt, periodAgeScan);
}

int loading_memory_from_loader(int pid,int page,int act, int finishact)
{
int frameno;
int check_page=-1,check_pid=-1;
frameno=get_free_frame();
printf("Got free frame = %d\n",frameno);
dump_memoryframe_info();
if(memFrame[frameno].dirty==dirtyFrame)
{ 
 
 
   //addr=(frameoff & pageoffsetMask) | (frameno << pagenumShift);

  update_process_pagetable(memFrame[frameno].pid,memFrame[frameno].page,diskPage);
  insert_swapQ (memFrame[frameno].pid, memFrame[frameno].page, &Memory[frameno*pageSize], actWrite, Nothing);
  check_page=memFrame[frameno].page;
  check_pid=memFrame[frameno].pid;
  
}
else if(memFrame[frameno].pid != nullPid)
{
update_process_pagetable(memFrame[frameno].pid,memFrame[frameno].page,diskPage);
}

update_frame_info (frameno, pid, page);
update_process_pagetable (pid, page, frameno);
 insert_swapQ(pid,page,&Memory[frameno*pageSize],act,finishact);
printf("Swap_in: in=(%d,%d,%x), out=(%d,%d,%x), m=%x\n",pid,page,&Memory[frameno*pageSize],check_pid,check_page,&Memory[frameno*pageSize],&Memory[0]);
return(mNormal);

}
