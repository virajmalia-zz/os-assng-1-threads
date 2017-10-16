#include "my_pthread.c"

void scheduler(int signum){
    sigprocmask(SIG_BLOCK, &signalMask, NULL);

    int q_size = getQSize(queue);
    int to_be_removed = 0;

    if(q_size == 1){
        tcb_ptr curr_context = getCurrentBlock(queue);
        if( curr_context->isExecuted ){
            // If current context has finished execution, dequeue
            heap_pop(queue);
        }
        else{
            // Thread timer check, decrease priority if uses full quanta
            if( curr_context->t_count == max_count ){
                heap_pop(queue);
                curr_context->priority--;
                heap_push(queue);
            }
            curr_context->t_count++;
        }
    }   // end if (q_size == 1)
    else if(q_size > 1){

            tcb_ptr curr_context = getCurrentBlock(queue);

            if( curr_context != NULL ){
                // Current context check
                if( curr_context->isExecuted ){
                    to_be_removed = 1;
                    // dequeue
                    heap_pop(queue);
                }
                else{
                    // Thread timer check, decrease priority if uses full quanta
                    if( curr_context->t_count == max_count ){
                        heap_pop(queue);
                        curr_context->priority--;
                        heap_push(queue);
                    }
                    tcb_ptr temp_head = heap_pop(queue);    // For next context
                    heap_push(queue, temp_head);
                }

                tcb_ptr next_context = getCurrentBlock(queue);

                while( next_context != NULL && next_context != curr_context && ( next_context->isBlocked || next_context->isExecuted ) ){

                    if( next_context->isExecuted ){
                        // dequeue
                        heap_pop(queue);
                    }
                    else{
                        tcb_ptr temp_head = heap_pop(queue);    // For next context
                        heap_push(queue, temp_head);
                    }
                    next_context = getCurrentBlock(queue);

                }

                // If !next_context, Go to next priority queue
                if( next_context == NULL ){
                    sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
                    exit(0);
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

            }   // end curr_context == NULL

        }
        sigprocmask(SIG_UNBLOCK, &signalMask, NULL);
}
