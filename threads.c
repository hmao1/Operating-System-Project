#include <unistd.h>
#include<string.h>
#include<sys/wait.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/types.h>
#include<ctype.h>
#include<stdio.h>
#include<stdlib.h>

//provided function header
#include<pthread.h>
#include<setjmp.h>
#include "ec440threads.h"
#include<signal.h>


//define jmp_buf
#define JB_RBX  0
#define JB_RBP  1
#define JB_R12  2
#define JB_R13  3
#define JB_R14  4
#define JB_R15  5
#define JB_RSP  6
#define JB_PC   7
//define thread state
#define EXITED  0
#define READY   1
#define RUNNING 2
#define UNDEFINE 3

//for the alaram and sigaction, using referrence of https://www.lemoda.net/c/ualarm-setjmp/
//gloabl variables
static int c_s_t = 0; //current schedule thread
static int s_c = 0; //schedule count, howmany time scheduler been called
static int thread_count=0, thread_created = 0;
static int tcb_index = 0;
static int initial=0;
static int pthread_id=0;
//static pthread_t curr_thread;
//type define thread_info
typedef struct{
        pthread_t thread_id;  //id
        jmp_buf buffer;       //array of jum_buf, contain register and rsp
                              //and  rip
        int status;  //state of thread(0exited,1ready,2running)
}thread_info;

static thread_info TCB[128];

void scheduler();

//create thread
int pthread_create(
        pthread_t *thread,
        const pthread_attr_t *attr,
        void* (*start_routine) (void*),
        void* arg)


{

//initialize the thread subsystem for the first time
//set TCB(include the main thread), sigaction, handler, ualarm
        if(initial==0)
        {
        int i;
        memset(TCB,0,128*sizeof(thread_info));
        for(i =0;i<128;i++)
        TCB[i].status = UNDEFINE;
        //TCB[0].thread_id = (pthread_t)0;
        //TCB[tcb_index].buffer; //??????initialize jmp_buff in the setjmp and loogjmp?
        //TCB[0].status = READY;


        //??????is this the right place to declare the sigaction and handler????????
        //define the signal handler
        struct sigaction action;
        action.sa_flags = SA_NODEFER;
        action.sa_handler = scheduler;
        sigaction(SIGALRM,&action,NULL);

 //????????????write main thread state in TCB jmp_buf??????????????
        //???how to modify the PC here so that it jump to pthread_create in main for loop
        setjmp(TCB[0].buffer);
        //*thread=(pthread_t)0;
        TCB[0].thread_id = (pthread_t)0;
        TCB[0].status = READY;




        //!!!!!!create second new thread
        tcb_index++;
        pthread_id++;
        thread_count++;
        thread_created++;


        *thread=(pthread_t)pthread_id;
        TCB[tcb_index].thread_id = (pthread_t)pthread_id;
        TCB[tcb_index].status = READY;

        void* stack_ptr=malloc(32767);
        stack_ptr += 32766;

        //push pthread_exit to the top of stack
        stack_ptr -= 8 ;
        *(long unsigned int*)stack_ptr = (long unsigned int)pthread_exit;


        //update and encrypt, modify jmp_buf in the TCB array(pc,rsp,r13,r12)
        TCB[tcb_index].buffer[0].__jmpbuf[JB_R12] =(long unsigned int)start_routine;
        TCB[tcb_index].buffer[0].__jmpbuf[JB_R13] = (long unsigned int)arg;


        TCB[tcb_index].buffer[0].__jmpbuf[JB_RSP] = ptr_mangle((long unsigned int)stack_ptr);
        TCB[tcb_index].buffer[0].__jmpbuf[JB_PC] = ptr_mangle((long unsigned int)start_thunk);


        initial+=1;

        }

//schedule for new thread
else{


//check if thread_count>127
if(tcb_index>127)
{
perror("Error: more than 128 threads exist cocurrently!\n");
exit(-1);
}

printf("call new %d thread created\n", thread_created);



*thread=(pthread_t)pthread_id;

//initialize the TCB
TCB[tcb_index].thread_id = (pthread_t)pthread_id;
//TCB[tcb_index].buffer; //??????initialize jmp_buff in the setjmp and loogjmp?
TCB[tcb_index].status = READY;


//for the created thread!!!!!!
//allocate stack for thread
void* stack_ptr=malloc(32767);
stack_ptr += 32766;

//push pthread_exit to the top of stack
 stack_ptr -= 8 ;
 *(long unsigned int*)stack_ptr = (long unsigned int)pthread_exit;


//update and encrypt, modify jmp_buf in the TCB array(pc,rsp,r13,r12)

TCB[tcb_index].buffer[0].__jmpbuf[JB_R12] =(long unsigned int)start_routine;
TCB[tcb_index].buffer[0].__jmpbuf[JB_R13] = (long unsigned int)arg;


TCB[tcb_index].buffer[0].__jmpbuf[JB_RSP] = ptr_mangle((long unsigned int)stack_ptr);
TCB[tcb_index].buffer[0].__jmpbuf[JB_PC] = ptr_mangle((long unsigned int)start_thunk);

}

//increment the thread_id for next new thread
//tcb_index is the new create thread index here,
//pthread_id is the thread id will be assigned to TCB
//thread-created count the num of thread created
//thread-count is the num of cocurrent thread in the TCB
tcb_index++;
pthread_id++;
thread_created++;
thread_count++;



//when the alarm is called, only one thread created
if(s_c==0)
ualarm(50000,50000);


return 0;
}





//scheduler is the signal_handler

void scheduler(){
//increment scheduler counter once it is called
s_c++;
//circle the array queue(c_s is current scheduled thread)
//c_s_t = c_s_t % thread_created;

printf("this is %d time call scheduler\n",s_c);
//run the main thread!!
                if(setjmp(TCB[c_s_t].buffer)==0)
                {
                while(1){
                c_s_t++;
                c_s_t = c_s_t % thread_created;
                if(TCB[c_s_t].status==READY)
                        break;
                }

                longjmp(TCB[c_s_t].buffer,1);
                }
}




//pthread exit
//need to clear memory
void pthread_exit(void* value_ptr){
printf("pthread exit!!!!!\n");

TCB[c_s_t].status = EXITED;
thread_count--;

//need more signal handle and clean the stack
if(thread_count==0)
{
        exit(0);

}
//else if(thread_count==1 && TCB[0].status == READY)

else
        //scheduler();
        while(1){
                c_s_t++;
                c_s_t = c_s_t % thread_created;
                if(TCB[c_s_t].status==READY)
                        break;
                }

                longjmp(TCB[c_s_t].buffer,1);

        __builtin_unreachable();
}


//return thread_id
pthread_t pthread_self(void){

return TCB[c_s_t].thread_id;
}

         

