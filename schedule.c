#include <stdio.h>

#include "my_pthread.h"

void scheduler(int signum){

    int q_size = getQueueSize(queue);
    bool to_be_removed = 0;

    if(q_size == 1){
        if( getCurrentBlock(queue)->isExecuted ){
            // If current context has finished execution, dequeue
            dequeue(queue);
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

                if( next_context == NULL )
                    return;

                if( next_context != curr_context ){
                    if( to_be_removed ){
                        // Set next thread as active, discard current thread
                        setcontext( &(next_context->thread_context) );
                    }
                    else{
                        // Swap current thread with next thread
                        swapcontext(&(curr_context->thread_context), &(next_context->thread_context) );
                    }
                }

            }

        }

}
