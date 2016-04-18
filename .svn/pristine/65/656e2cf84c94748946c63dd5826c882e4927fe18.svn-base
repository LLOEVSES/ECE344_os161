/*
 * stoplight.c
 *
 * 31-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: You can use any synchronization primitives available to solve
 * the stoplight problem in this file.
 *
 */


/*
 *
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>
#include <curthread.h>

// every theard waiting at the intersection will be considered as a waiting thread
// containing the thread pointer point to the this thread
// the cv to awaken the thread: first
// the link to the next thread
struct waiting_thread
{
    struct thread *thread;
    struct cv *first;
    struct waiting_thread *next;
};

// the link list for waiting thread
// each direction will have a wait list for thread
struct wait_list
{
    struct waiting_thread *head;
    struct waiting_thread *tail;
    struct lock *list_lock;
};

/*
 *
 * Constants & Globals
 *
 */

/*
 * Number of cars created.
 */

#define NCARS 20

// array of locks for 4 areas in the path, the index also satand for directions
struct lock *area_locks[4];
struct wait_list *direction_wait_lists[4];

/*
 *
 * Function Definitions
 *
 */

static const char *directions[] = { "N", "E", "S", "W" };

static const char *msgs[] =
{
    "approaching:",
    "region1:    ",
    "region2:    ",
    "region3:    ",
    "leaving:    "
};

/* use these constants for the first parameter of message */
enum { APPROACHING, REGION1, REGION2, REGION3, LEAVING };

static void
message(int msg_nr, int carnumber, int cardirection, int destdirection)
{
    kprintf("%s car = %2d, direction = %s, destination = %s\n",
            msgs[msg_nr], carnumber,
            directions[cardirection], directions[destdirection]);
}

// This function is used to add a thread to the corresponding wait_list
void add_thread(struct wait_list *wl)
{
    struct waiting_thread *new_thread;

    lock_acquire(wl->list_lock);

    // alloc memory for the new 
    new_thread = kmalloc(sizeof(struct waiting_thread));
    new_thread->thread = curthread;
    new_thread->next = NULL;
    new_thread->first = cv_create("waiting_thread_first");

    // check if this is the first thread in the waiting list
    if (wl->head == NULL)
    {
        wl->head = new_thread;
        wl->tail = new_thread;
    }
    else
    {
        wl->tail->next = new_thread;
        wl->tail = new_thread;
    }
}

// Remove the current thread from the waitlist.
void remove_thread(struct wait_list *wl)
{
    struct waiting_thread *first_thread;

    lock_acquire(wl->list_lock);

    // get the first thread and remove it from the wait list
    first_thread = wl->head;
    wl->head = wl->head->next;
    if (wl->head == NULL)
    {
        wl->tail = NULL;
    }
    else
    {
        // Notify the next thread that it is now at the head.
        cv_signal(wl->head->first, wl->list_lock);
    }

    // destruct the first thread
    cv_destroy(first_thread->first);
    kfree(first_thread);

    lock_release(wl->list_lock);
}

// make the thread waiting (sleep) until it become the first one in the wait list
void waiting_for_first(struct wait_list *wl)
{
    // Wait until we're at the front
    if (wl->head->thread != curthread)
    {
        cv_wait(wl->tail->first, wl->list_lock);
    }
    lock_release(wl->list_lock);
}

// sorting the path arry
void
bubble_sort(int *p)
{
    int i, j;
    for(j = 2; j > 0; j--)
        for(i = 0; i < j; i++)
        {
            if(p[i] > p[i + 1])
            {
                int temp = p[i];
                p[i] = p[i + 1];
                p[i + 1] = temp;
            }
        }
}

int *sort_path(int *p)
{
    if(p[1] == -1)
        return p;
    if(p[2] == -1)
    {
        if(p[0] < p[1])
            return p;
        if(p[0] > p[1])
        {
            int temp = p[0];
            p[0] = p[1];
            p[1] = temp;
            return p;
        }
    }
    bubble_sort(p);
    return p;
}


/*
 * check_lock()
 *
 * Arguments:
 *      int * path: the pointer to path array
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function take the path contains areas the car will travel
 *  it will acquire the lock in the given order until every lock is got
 */
void
check_lock(int *path)
{
    // acquire area_locks in the path one by one
    // after calling this function, all areas in the path will be locked by the car
    // if the area is null if the car do not pass that area(not acquire)
    assert(path[0] != -1);

    lock_acquire(area_locks[path[0]]);

    if (path[1] != -1)
    {
        lock_acquire(area_locks[path[1]]);
    }

    if (path[2] != -1)
    {
        lock_acquire(area_locks[path[2]]);
    }
}

/*
 * go()
 *
 * Arguments:
 *      int * path: the pointer to path array
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function will print message when travelling through the path and release the locks
 */
void
go(int *path, int carnumber, int cardirection, int destination)
{
    assert(path[0] != -1);

    message(REGION1, carnumber, cardirection, destination);
    lock_release(area_locks[path[0]]);

    if(path[1] != -1)
    {
        message(REGION2, carnumber, cardirection, destination);
        lock_release(area_locks[path[1]]);
    }

    if(path[2] != -1)
    {
        message(REGION3, carnumber, cardirection, destination);
        lock_release(area_locks[path[2]]);
    }

    message(LEAVING, carnumber, cardirection, destination);
}

/*
 * approachintersection()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 */

static
void
approachintersection(void *unusedpointer,
                     unsigned long carnumber)
{
    // This two random number stand for cardirection(star point) and the car action
    int cardirection;
    int move;
    int destination;
    int i;
    int path[3];
    struct wait_list *direction_wait_list;

    /*
      * Avoid unused variable and function warnings.
      */

    (void) unusedpointer;

    /*
     * cardirection is set randomly.
     * also, a car move(straight, left, right) is set randomly.
    * since the random() geberate an int for us, we assign:
     * 0 = N; 1 = E; 2 = S; 3 = W;
     * 0 = turn right; 1 = go straight; 2 = turn left
     */

    cardirection = random() % 4;
    move = random() % 3;

    // determine path and destination by cardirection and move
    switch(move)
    {
    case 0 :
        path[0] = cardirection;
        path[1] = -1;
        path[2] = -1;
        destination = (cardirection + 3) % 4;
        break;
    case 1 :
        path[0] = cardirection;
        path[1] = (cardirection + 3) % 4;
        path[2] = -1;
        destination = (cardirection + 2) % 4;
        break;
    case 2 :
        path[0] = cardirection;
        path[1] = (cardirection + 3) % 4;
        path[2] = (cardirection + 2) % 4;
        destination = (cardirection + 1) % 4;
        break;
    default :
        assert(0);
        break;
    }

    // we are tring to acquire every lock in the path
    // however directly acquiring in sequence may cause a deadlock
    // solve the problem by sort the path and acquire in order
    int sorted_path[3];
    for (i = 0; i < 4; i++)
    {
        /* make a copy of path since go() function is order sensitive */
        sorted_path[i] = path[i];
    }
    sort_path(sorted_path);

    // Enter the queue to approach the intersection
    direction_wait_list = direction_wait_lists[cardirection];
    add_thread(direction_wait_list);
    message(APPROACHING, carnumber, cardirection, destination);

    // Wait until we're at the front of the queue
    waiting_for_first(direction_wait_list);

    // we now at the the top of the queue
    // call check_locks function to check and wait until the path is available
    check_lock(sorted_path);
    remove_thread(direction_wait_list);

    // print the message and release locks hold by the car
    go(path, carnumber, cardirection, destination);
}


/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.
 */

int
createcars(int nargs,
           char **args)
{
    int i, index, error;

    /*
     * Avoid unused variable warnings.
     */

    (void) nargs;
    (void) args;

    /*
     * Initialize synchronization constructs.
     */

    for (i = 0; i < 4; i++)
    {
        area_locks[i] = lock_create("area_locks");
        direction_wait_lists[i] = kmalloc(sizeof(struct wait_list));
        direction_wait_lists[i]->head = NULL;
        direction_wait_lists[i]->tail = NULL;
        direction_wait_lists[i]->list_lock = lock_create("wait_list_lock");
    }

    /*
     * Start NCARS approachintersection() threads.
     */

    for (index = 0; index < NCARS; index++)
    {

        error = thread_fork("approachintersection thread",
                            NULL,
                            index,
                            approachintersection,
                            NULL
                           );

        /*
         * panic() on error.
         */

        if (error)
        {

            panic("approachintersection: thread_fork failed: %s\n",
                  strerror(error)
                 );
        }
    }
    return 0;
}

