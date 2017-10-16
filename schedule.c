#include <stdio.h>

#include "my_pthread.h"

inline void findQ(){

}

void scheduler(int signum){
    sigprocmask(SIG_BLOCK, &signalMask, NULL);

    //check all  queues for available threads
L1:
    int i = 0;
    while( empty(queue[i]) ){
        i++;
    }
    thread_Queue queue = queue[i];
    int max_count;
    switch(i){
        case 0:
            max_count = 1;
            break;
        case 1:
            max_count = 3;
            break;
        case 2:
            max_count = 7;
            break;
        case 3:
            max_count = 15;
            break;
    }

    int q_size = getQueueSize(queue);
    bool to_be_removed = 0;

    if(q_size == 1){
        tcb_ptr curr_context = getCurrentBlock(queue);
        if( curr_context->isExecuted ){
            // If current context has finished execution, dequeue
            dequeue(queue);
        }
        else{
            // Thread timer check, decrease priority if uses full quanta
            if( curr_context->t_count == max_count ){
                dequeue(queue);
                curr_context->priority++;
                enqueue(queue[curr_context->priority]);
            }
            curr_context->t_count++;
        }
    }
    else if(q_size > 1){

            tcb_ptr curr_context = getCurrentBlock(queue);

            if( curr_context != NULL ){
                if( curr_context->isExecuted ){
                    to_be_removed = 1;
                    // dequeue
                    dequeue(queue);
                }
                else{
                    // Thread timer check, decrease priority if uses full quanta
                    if( curr_context->t_count == max_count ){
                        dequeue(queue);
                        curr_context->priority++;
                        enqueue(queue[curr_context->priority]);
                    }
                    next(queue);
                }

                tcb_ptr next_context = getCurrentBlock(queue);

                while( next_context != NULL && ( next_context->isBlocked || next_context->isExecuted ) ){

                    if( next_context->isExecuted ){
                        // dequeue
                        dequeue(queue);
                    }
                    else{
                        next(queue);
                    }
                    next_context = getCurrentBlock(queue);

                }

                // If !next_context, Go to next priority queue
                if( next_context == NULL ){
                    goto L1;
                    //sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
                    //exit(0);
                }

                if( next_context != curr_context ){
                    sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
                    if( to_be_removed ){
                        // Set next thread as active, discard current thread
                        setcontext( &(next_context->thread_context) );
                        next_context->t_count++;
                    }
                    else{
                        // Swap current thread with next thread
                        swapcontext( &(curr_context->thread_context), &(next_context->thread_context) );
                        next_context->t_count++;
                    }
                }

            }

        }
sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
}
