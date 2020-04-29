#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>


typedef struct process_block {
    char name[40];
    int ready_time, exec_time, remain;
    pid_t pid;
    struct process_block *next;
} PCB;

PCB *new_block()
{
    PCB *n = (PCB *)malloc(sizeof(PCB));
    assert(n != NULL);
    n->next = 0;

    return n;
}
typedef struct PCB_queue {
    PCB *head, *tail;
    int count;
} PCB_Queue;
int is_Empty(PCB_Queue *q)
{
    return q->count == 0;
}
void enqueue(PCB_Queue *q, PCB *block)
{
    block->next = NULL;
    if (q->tail == NULL) {
        q->head = q->tail = block;
    } else {
        q->tail->next = block;
        q->tail = block;
    }
    q->count += 1;
}
void insert_shortest(PCB_Queue *q, PCB *b)
{
    b->next = NULL;
    if (is_Empty(q)) {
        enqueue(q, b);
    } else {
        if (b->remain < q->head->remain) {
            b->next = q->head;
            q->head = b;
        } else {
            PCB *prev = q->head, *current = q->head->next;
            while (current != NULL && b->remain >= current->remain) {
                prev = current;
                current = current->next;
            }
            prev->next = b;
            b->next = current;
            if (current == NULL) {
                q->tail = b;
            }
        }
        q->count += 1;
    }
}
PCB *dequeue(PCB_Queue *q)
{
    if (is_Empty(q)) {
        fprintf(stderr, "empty queue\n");
        exit(-1);
    }
    PCB *temp = q->head;
    q->head = q->head->next;
    q->count -= 1;
    if (q->count == 0) {
        q->tail = NULL;
    }
    return temp;
}
int N;
PCB_Queue waiting_queue = {NULL, NULL, 0};
PCB_Queue ready_queue = {NULL, NULL, 0};

void time_unit()
{ volatile unsigned long i; for(i=0;i<1000000UL;i++); }
void set_cpu(int id) ///reference:https: //www.itread01.com/content/1547866108.html
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(id, &mask);
    if (sched_setaffinity(getpid(), sizeof(mask), &mask) == -1) {
        fprintf(stderr, "warning: could not set CPU affinity/n");
        exit(-1);
    }
}
void set_priority(pid_t pid, int policy, int priority)
{
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(policy);
    if (priority > 0) {
        param.sched_priority = priority;
    }
    if (sched_setscheduler(pid, policy, &param) == -1) {
            fprintf(stderr, "set scheduler error\n");
            exit(-1);
    }
}

void FIFO()
{
    int count = 0, time = 0;
    long long start_time;///for child process
    PCB *block = NULL;
    set_priority(getpid(), SCHED_FIFO, 0);
    while (count < N) {
        if (time == waiting_queue.head->ready_time) {
            block = dequeue(&waiting_queue);
            if ((block->pid = fork()) < 0) {
                fprintf(stderr, "fork error\n");
                exit(-1);
            } else if (block->pid == 0) { ///child
                start_time = syscall(334);
                set_cpu(1);
                for (int i = 0; i < block->exec_time; i++) {
                    time_unit();
                }
                break;
            } else {
                set_priority(block->pid, SCHED_FIFO, 0);
                count++;
            }
        } else {
            time_unit();
            time++;
        }
    }

    if (count < N) {
        printf("%s %d\n", block->name, getpid());
        syscall(335, getpid(), start_time, syscall(334));
        ///system call here
    } else {
        for (int i = 0; i < N; i++) {
            wait(NULL);
        }
    }

}
void SJF()
{
    int count = 0, time = 0, finish_time = -1;
    long long start_time;///for child
    while (count < N) {
        while (!is_Empty(&waiting_queue) && time == waiting_queue.head->ready_time) {
            PCB *process = dequeue(&waiting_queue);
            insert_shortest(&ready_queue, process);
            if ((process->pid = fork()) < 0) {
                fprintf(stderr, "fork error\n");
                exit(-1);
            } else if (process->pid == 0) { ///child
                start_time = syscall(334);
                set_cpu(1);
                for (int i = 0; i < process->exec_time; i++) {
                    time_unit();
                }
                printf("%s %d\n", process->name, getpid());
                syscall(335, getpid(), start_time, syscall(334));
        ///system call here
                return;
            } else {
                set_priority(process->pid, SCHED_FIFO, 2);
            }
        }
        if (!is_Empty(&ready_queue) && (finish_time == time || finish_time < 0)) {
            PCB *process = dequeue(&ready_queue);
            set_priority(process->pid, SCHED_FIFO, 20);
            finish_time = time + process->remain;
            count++;
        }
        time_unit();
        time++;

    }
    for (int i = 0; i < N; i++) {
        wait(NULL);
    }

}
void PSJF()
{
    int count = 0, time = 0;
    long long start_time;///for child
    PCB *current_process = NULL;
    while (count < N) {
        while (!is_Empty(&waiting_queue) && time == waiting_queue.head->ready_time) {
            PCB *process = dequeue(&waiting_queue);
            insert_shortest(&ready_queue, process);
            if ((process->pid = fork()) < 0) {
                fprintf(stderr, "fork error\n");
                exit(-1);
            } else if (process->pid == 0) { ///child
                start_time = syscall(334);
                set_cpu(1);
                for (int i = 0; i < process->exec_time; i++) {
                    time_unit();
                }
                printf("%s %d\n", process->name, getpid());
                syscall(335, getpid(), start_time, syscall(334));
        ///system call here
                return;
            } else {
                set_priority(process->pid, SCHED_FIFO, 2);
            }
        }
        if (!is_Empty(&ready_queue)) {
            if (current_process == NULL) {
                current_process = dequeue(&ready_queue);
                set_priority(current_process->pid, SCHED_FIFO, 20);
            } else if (ready_queue.head->remain < current_process->remain) {
                set_priority(current_process->pid, SCHED_FIFO, 2);
                insert_shortest(&ready_queue, current_process);
                current_process = dequeue(&ready_queue);
                set_priority(current_process->pid, SCHED_FIFO, 20);
            }
        }
        time_unit();
        time++;
        if (current_process != NULL) {
            current_process->remain -= 1;
            if (current_process->remain == 0) {
                free(current_process);
                current_process = NULL;
                count++;
            }
        }
    }
    for (int i = 0; i < N; i++) {
        wait(NULL);
    }
}
void RR()
{
    int count = 0, time = 0, time_quantumm_count = 0;
    long long start_time;///for child
    PCB *current_process = NULL;
    while (count < N) {
        while (!is_Empty(&waiting_queue) && time == waiting_queue.head->ready_time) {
            PCB *process = dequeue(&waiting_queue);
            enqueue(&ready_queue, process);
            if ((process->pid = fork()) < 0) {
                fprintf(stderr, "fork error\n");
                exit(-1);
            } else if (process->pid == 0) { ///child
                start_time = syscall(334);
                set_cpu(1);
                for (int i = 0; i < process->exec_time; i++) {
                    time_unit();
                }
                printf("%s %d\n", process->name, getpid());
                syscall(335, getpid(), start_time, syscall(334));
        ///system call here
                return;
            } else {
                set_priority(process->pid, SCHED_FIFO, 2);
            }
        }

        if (current_process != NULL) {
            current_process->remain -= 1;
            time_quantumm_count++;
            if (current_process->remain == 0) {
                free(current_process);
                current_process = NULL;
                count++;
                time_quantumm_count = 0;
            } else if (time_quantumm_count == 500) {
                time_quantumm_count = 0;
                set_priority(current_process->pid, SCHED_FIFO, 2);
                enqueue(&ready_queue, current_process);
                current_process = NULL;
            }
        }

        if (!is_Empty(&ready_queue) && current_process == NULL) {
            current_process = dequeue(&ready_queue);
            set_priority(current_process->pid, SCHED_FIFO, 20);
        }
        time_unit();
        time++;
    }
    for (int i = 0; i < N; i++) {
        wait(NULL);
    }
}
void bubblesort_readytime(PCB_Queue *q)
{
    if (q->count < 2) {
        return;
    }
    PCB *i = NULL, *j, *prev;
    for (; i != q->head;) {
        for (j = q->head; j->next != i;) {
            if (j->ready_time > j->next->ready_time) {
                if (j->next == q->tail) {
                    q->tail = j;
                }
                if (j == q->head) {
                    q->head = j->next;
                    j->next = j->next->next;
                    q->head->next = j;
                    prev = q->head;
                } else {
                    prev->next = j->next;
                    j->next = j->next->next;
                    prev->next->next = j;
                    prev = prev->next;
                }
            } else {
                prev = j;
                j = j->next;
            }
        }
        i = j;
    }
}
int main(void)
{
    set_cpu(0);
    char policy[8];
    scanf("%s", policy);
    scanf("%d", &N);
    for (int i = 0; i < N; i++) {
        PCB *b = new_block();
        scanf("%s %d %d", b->name, &(b->ready_time), &(b->exec_time));
        b->remain = b->exec_time;
        enqueue(&waiting_queue, b);
    }
    ///printf("%d %d\n", sched_get_priority_max(SCHED_FIFO), sched_get_priority_min(SCHED_FIFO));
    bubblesort_readytime(&waiting_queue);
    switch (policy[0]) {
    case 'F':
        FIFO();
        break;
    case 'R':
        RR();
        break;
    case 'S':
        SJF();
        break;
    case 'P':
        PSJF();
        break;
    }
    return 0;
}
