#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<time.h>
#include<stdbool.h>


void worker_fn(void *args);
void scheduler(void *type);
void sched(int);
int min();
int srtf();
int pbs();
void init_variables();

int size;

int ind = 0, thread_count = 0, scheduler_start_flag = 0, global_time = 0;
int values[100][3] = {0};

pthread_t th[100];
pthread_t ts;
pthread_mutex_t mutex[100] = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ts = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t event[100] = PTHREAD_COND_INITIALIZER;
pthread_cond_t event_s = PTHREAD_COND_INITIALIZER;
pthread_cond_t init_event = PTHREAD_COND_INITIALIZER;

typedef struct queue
{
	int id;
	int stime;
	struct queue *next;
}queue;

struct queue *head[5] = {NULL};
struct queue *curr[5] = {NULL};

struct queue* create_list(int id, int stime, int n)
{
    //printf("creating list %d with headnode as [%d]\n", n, id);
    struct queue *ptr = (struct queue*)malloc(sizeof(struct queue));
    if(NULL == ptr)
    {
        printf("\n Node creation failed \n");
        return NULL;
    }
    ptr->id = id;
    ptr->stime = stime;
    ptr->next = head[n];

    head[n] = curr[n] = ptr;
    return ptr;
}

struct queue* add_to_list(int id, int stime, int n)
{
    if(NULL == head[n])
    {
        return (create_list(id, stime, n));
    }

    struct queue *ptr = (struct queue*)malloc(sizeof(struct queue));
    if(NULL == ptr)
    {
        printf("\n Node creation failed \n");
        return NULL;
    }
    ptr->id = id;
    ptr->stime = stime;
    ptr->next = head[n];

    curr[n]->next = ptr;
    curr[n] = ptr;

    return ptr;
}

int delete_from_list(int thid, int n)
{
    struct queue *del = NULL;
    //printf("Deleting value [%d] from list\n",thid);
    if (head[n] == NULL) // when list is empty
		return 0;
	else
	{
		if (head[n] == curr[n])
			head[n] = curr[n] = 0;
		else
		{
			struct queue *present = head[n];
			struct queue *temp = head[n];
			while (present != NULL && present->id != thid)
			{
				temp = present;
				present = present->next; // checks if the number entered is there in the list, if yes, then which position
			}
			if (present == NULL)
			{
				//free(del);
				del = NULL;
			}
			else
			{
				del = present;
				present = present->next;
				temp->next = present; // links the list when an element gets deleted
				if (del == head[n])
				{
					head[n] = head[n]->next;
					temp = NULL;
				}
				//free(del);
				del = NULL;
			}
		}
	}
    return 0;
}


void print_list(int n)
{
    struct queue *ptr = head[n];
    if (ptr == NULL)
	return;

    printf("\n -------Printing list %d Start------- \n", n);
    if(curr[n] == head[n])
    {
	printf("\n [%d] \n",ptr->id);
	return;
    }
    while(ptr->next != head[n])
    {
        printf("\n [%d] \n",ptr->id);
        ptr = ptr->next;
    }
    printf("\n -------Printing list End------- \n");

    return;
}

struct thread_t
{
	int th_id;
	int start_time;
	int req_time;
	int state;
	int time_rem;
	int tprio;
	int flag;
	int cnt;
}arg[100];

void read_values ()
{
	FILE* file = fopen ("data.txt", "r");
	int i = 0;
	fscanf(file, "%d", &size);
	while (!feof (file))
	{
		fscanf (file, "%d %d %d\n", &values[i][0], &values[i][1], &values[i][2]);
		//printf("%d %d %d\n", values[i][0], values[i][1], values[i][2]);
		i++;
	}
	fclose (file);
}

int main()
{
	srand(time(0));
	int i, j, m, type;
	printf("1. FCFS \n");
	printf("2. SRTF \n");
	printf("3. PBS \n");
	printf("4. MLFQ \n");
	printf("Select the type of scheduler : ");
	scanf("%d", &type);
    	init_variables();
	read_values();
	pthread_create(&ts, NULL, (void *)scheduler, (void *) &type);
	for(i = 0; i < size; i++)
	{
		arg[i].th_id = i;
		arg[i].start_time = values[i][0];
		arg[i].req_time = values[i][1];
		arg[i].state = 0;
		arg[i].time_rem = arg[i].req_time;
		arg[i].tprio = values[i][2];
		arg[i].flag = 0;
		arg[i].cnt = 0;
		//printf("arguments set as %d and %d tprio %d\n", arg[i].start_time, arg[i].req_time, arg[i].tprio);
		pthread_create(&th[i], NULL, (void *)worker_fn, (void *)&arg[i]);
	}
   	pthread_join(ts,NULL);
    	printf("Simulation program is exiting \n");
	return 1;
}


void init_variables(){
    int i;
    for(i=0;i<size;i++){
        pthread_mutex_init(&mutex[i],NULL);
        pthread_cond_init(&event[i],NULL);
    }
    pthread_mutex_init(&mutex_ts,NULL);
    pthread_cond_init(&event_s,NULL);
    pthread_cond_init(&init_event,NULL);
}

void worker_fn(void *args)
{
	struct thread_t *t_arg;
	t_arg = (struct thread_t*)args;
    	pthread_mutex_lock(&mutex[t_arg->th_id]);
	sched(t_arg->th_id);
	while (t_arg->time_rem > 0 && t_arg->state == 1)
	{
		//printf("thid = %d \t start time = %d \t remaining time = %d \t priority = %d\n", t_arg->th_id, t_arg->start_time, t_arg->time_rem, t_arg->tprio);
		printf("%d - TH%d\n", global_time, t_arg->th_id);
		t_arg->time_rem--;
		global_time++;
		t_arg->cnt++;
		sched(t_arg->th_id);
	}
	sched(t_arg->th_id);
	return;
}

void sched(int id)
{
	int x;
	if (arg[id].time_rem == 0)
	{
        	printf("TH%d was eliminated \n",id);
		arg[id].state = 2;
		delete_from_list(arg[id].th_id, arg[id].flag);
		arg[id].flag = -1;
		pthread_cond_signal(&event_s);
        	pthread_mutex_unlock(&mutex[id]);
        	pthread_exit(NULL);
	}
	else
	{
		arg[id].state = 0;
        	if(scheduler_start_flag == 0)
		{
            		thread_count++;
            		//printf("thread count is %d and scheduler_start_flag is %d \n",thread_count,scheduler_start_flag);
            		if(thread_count == size)
			{
                		scheduler_start_flag = 1;
                		//printf("signalling scheduler value is %d \n",scheduler_start_flag);
                		pthread_cond_signal(&init_event);
            		}
        	}
        	//printf("thread %d about to wait \n",id);
        	pthread_cond_signal(&event_s);
		pthread_cond_wait(&event[id], &mutex[id]);
		arg[ind].state = 1;
	}
	return;
}

void scheduler(void *type)
{
	int l;
	int *t;
	t = (int *)type;
    pthread_mutex_lock(&mutex_ts);
    //printf("scheduler waiting \n");
    pthread_cond_wait(&init_event, &mutex_ts);
    pthread_mutex_unlock(&mutex_ts);
    //printf("scheduler awakened \n");
    goto L1;
	while(1)
	{
		int eliminate_count;
		pthread_mutex_lock(&mutex_ts);
		pthread_cond_wait(&event_s, &mutex_ts);
		//printf("scheduler signalled\n");
		pthread_mutex_unlock(&mutex_ts);
	    L1: eliminate_count = 0;
    	        for(l = 0; l < size; l++)
		{
			if(arg[l].state == 2)
				eliminate_count++;
		}
		if(eliminate_count < size)
		{
			switch(*t)
			{
			case 1 :
				ind = min();
                		pthread_mutex_lock(&mutex[ind]);
                		//printf("thread %d signalled ! \n",ind);
				pthread_cond_signal(&event[ind]);
                		pthread_mutex_unlock(&mutex[ind]);
				break;

			case 2 :
				ind = srtf();
				pthread_mutex_lock(&mutex[ind]);
				//printf("thread %d signalled ! \n",ind);
				pthread_cond_signal(&event[ind]);
				pthread_mutex_unlock(&mutex[ind]);
				break;
			case 3 :
				ind = pbs();
				pthread_mutex_lock(&mutex[ind]);
				//printf("thread %d signalled ! \n",ind);
				pthread_cond_signal(&event[ind]);
				pthread_mutex_unlock(&mutex[ind]);
				break;
			case 4 :
				ind = mlfq();
				pthread_mutex_lock(&mutex[ind]);
				//printf("thread %d signalled ! \n",ind);
				pthread_cond_signal(&event[ind]);
				pthread_mutex_unlock(&mutex[ind]);
				break;
			default :
				break;
			}
		}
		else
			break;
	}
}

int min()
{
	int min_index, minimum, k;
   L5 : min_index = size + 1;
	minimum = 1000;
	for(k = 0; k < size; k++)
	{
		if(global_time >= arg[k].start_time)
		{
			if(arg[k].start_time < minimum && arg[k].state != 2)
			{
				minimum = arg[k].start_time;
				min_index = k;
			}
		}
	}
	if(min_index == size + 1)
	{
		printf("%d\n", global_time);
		global_time++;
		goto L5;
	}
	else
	{
		//printf("min_index %d\n", min_index);
		return min_index;
	}
}

int srtf()
{
	int srtf_index, srt, k;
   L2 : srtf_index = size + 1;
	srt = 1000;
	for(k = 0; k < size; k++)
	{
		if(global_time >= arg[k].start_time)
		{
			if(arg[k].time_rem < srt && arg[k].state != 2)
			{
				srt = arg[k].time_rem;
				srtf_index = k;
			}
		}
	}
	for(k = 0; k < size; k++)
	{
		if(global_time >= arg[k].start_time)
		{
			if(arg[k].time_rem == srt && arg[k].th_id != srtf_index && arg[k].state != 2)
			{
				if(arg[k].start_time < arg[srtf_index].start_time)
					srtf_index = k;
			}
		}
	}
	if(srtf_index == size + 1)
	{
		printf("%d\n", global_time);
		global_time++;
		goto L2;
	}
	else
	{
		//printf("srtf_index %d\n", srtf_index);
		return srtf_index;
	}
}

int pbs()
{
	int pbs_index, pbs, k, rep;
   L3 : pbs_index = size + 1;
	pbs = 1000;
	for(k = 0; k < size; k++)
	{
		if(global_time >= arg[k].start_time)
		{
			if(arg[k].tprio < pbs && arg[k].state != 2)
			{
				pbs = arg[k].tprio;
				pbs_index = k;
			}
		}
	}
	//printf("pbs %d, ind %d\n", pbs, pbs_index);
	for(k = 0; k < size; k++)
	{
		if(global_time >= arg[k].start_time)
		{
			if(arg[k].tprio == pbs && arg[k].th_id != pbs_index && arg[k].state != 2)
			{
				if(arg[k].start_time < arg[pbs_index].start_time)
					pbs_index = k;
			}
		}
	}

	if(pbs_index == size + 1)
	{
		printf("%d\n", global_time);
		global_time++;
		goto L3;
	}
	else
	{
		//printf("pbs %d, ind %d\n", pbs, pbs_index);
		return pbs_index;
	}
}

int mlfq()
{
	int k, p, r, l = 0, min_index, q[5] = {0};
	struct queue *tmp;
   L4 : min_index = size + 1;
	for (r=0;r<size; r++)
	{
		if(arg[r].start_time == global_time)
			add_to_list(arg[r].th_id, arg[r].start_time, 0);
	}
	for (r = 0; r < size; r++)
	{
		if (arg[r].state != 2)
		{
			if(arg[r].cnt == 5 && arg[r].flag == 0)
			{
				add_to_list(arg[r].th_id, arg[r].start_time, 1);
				delete_from_list(arg[r].th_id, 0);
				arg[r].flag = 1;
			}
			else if(arg[r].cnt == 15 && arg[r].flag == 1)
			{
				add_to_list(arg[r].th_id, arg[r].start_time, 2);
				delete_from_list(arg[r].th_id, 1);
				arg[r].flag = 2;
			}
			else if(arg[r].cnt == 30 && arg[r].flag == 2)
			{
				add_to_list(arg[r].th_id, arg[r].start_time, 3);
				delete_from_list(arg[r].th_id, 2);
				arg[r].flag = 3;
			}
			else if(arg[r].cnt == 50 && arg[r].flag == 3)
			{
				add_to_list(arg[r].th_id, arg[r].start_time, 4);
				delete_from_list(arg[r].th_id, 3);
				arg[r].flag = 4;
			}
		}
	}
	//for(p = 0; p<5; p++)
	//	print_list(4);
	for (k = 0; k < size; k++)
	{
		if(global_time >= arg[k].start_time)
		{
			if(arg[k].state != 2)
			{
				if(head[0] != NULL)
				{
					//printf("queue 0\n");
					min_index = head[0]->id;
				}
				else if(head[0] == NULL && head[1] != NULL)
				{
					//printf("queue 1\n");
					min_index = head[1]->id;
				}
				else if(head[0] == NULL && head[1] == NULL && head[2] != NULL)
				{
					//printf("queue 2\n");
					min_index = head[2]->id;
				}
				else if(head[0] == NULL && head[1] == NULL && head[2] == NULL && head[3] != NULL)
				{
					//printf("queue 3\n");
					min_index = head[3]->id;
				}
				else if(head[0] == NULL && head[1] == NULL && head[2] == NULL && head[3] == NULL && head[4] != NULL)
				{
					//printf("queue 4\n");
					min_index = head[4]->id;
					head[4] = head[4]->next;
				}
			}
		}
	}
	if(min_index == size + 1)
	{
		printf("%d\n", global_time);
		global_time++;
		goto L4;
	}
	else
	{
		//printf(" ind %d\n", min_index);
		return min_index;
	}
}
