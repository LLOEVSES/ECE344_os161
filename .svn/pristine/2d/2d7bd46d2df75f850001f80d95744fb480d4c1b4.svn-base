/*
 * catsem.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use SEMAPHORES to solve the cat syncronization problem in 
 * this file.
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

/*
 * 
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2

/*
 * Number of times to eat per animal.
 */

#define NMEALS 4

//number of total meals = NCATS*NMICE*4iterations
#define NTOTALMEAL 32

/*
 * 
 * Globals
 * 
 */

// The bowls which cats are allowed to each from
struct semaphore *c_bowls;

// The bowls which mice are allowed to each from
struct semaphore *m_bowls;

//currently eating animals
struct semaphore *current_eating;

//animals forced to be blocked from their bowls
struct semaphore *blocked;

//bowl_state, 0 means occupied, 1 means available
int c_bowl1=1;
int m_bowl1=1;

//counts meals have been eaten
int meal_count = 0;

//lock when changes the bowl_state
struct semaphore *bowl_state_lock;

/*
 * 
 * Function Definitions
 * 
 */

/* who should be "cat" or "mouse" */
static void
sem_eat(const char *who, int num, int bowl, int iteration)
{
        kprintf("%s: %d starts eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
        clocksleep(1);
        kprintf("%s: %d ends eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
}


//if the game is not finished, return 1
int not_finished(){
	int game_status;
	P(bowl_state_lock);
		if(meal_count<NTOTALMEAL)
			game_status = 1;
		else
			game_status = 0;
	V(bowl_state_lock);
	return game_status;
}


//only allows one kind of animal can touch their bowls, the other is blocked, and
//constantly switch animals
void switch_bowl() {
   
    struct semaphore* temp;
    
    while (not_finished()) {
	//drop two bowls for the kind of animals who are currently eating
	P(current_eating);
	P(current_eating);
	
	//give two bowls for the kind of animals who are currently blocked
	V(blocked);
	V(blocked);
	
	//switch animal
	temp = current_eating;
	current_eating = blocked;
	blocked = temp;
    }
}



/*
 * catsem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Implements the cat thread.
 *
 */

static
void
catsem(void * unusedpointer, 
       unsigned long catnumber)
{
    int i;

    /*
     * Avoid unused variable warnings.
     */
    (void) unusedpointer;


    for (i = 0; i < 4; i++) {
        P(c_bowls);
		int bowl;
		//find which bowl is available
		P(bowl_state_lock);
			if(c_bowl1){
				bowl = 1;
				c_bowl1 = 0;
			}
			else{
				bowl = 2;
			}
		V(bowl_state_lock);
        	sem_eat("cat", catnumber, bowl, i);
		P(bowl_state_lock);
			if(bowl==1)
				c_bowl1 = 1;
		V(bowl_state_lock);
        V(c_bowls);
    }
}
        

/*
 * mousesem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Implements the mouse thread.
 *
 */

static
void
mousesem(void * unusedpointer, 
         unsigned long mousenumber)
{
    int i;

    /*
     * Avoid unused variable warnings.
     */
    (void) unusedpointer;
    

    for (i = 0; i < 4; i++) {
        P(m_bowls);
		int bowl = 0;
		//find which bowl is available
		P(bowl_state_lock);
			if(m_bowl1){
				bowl = 1;
				m_bowl1 = 0;
			}
			else{
				bowl = 2;
			}
		V(bowl_state_lock);
        	sem_eat("mouse", mousenumber, bowl, i);
		P(bowl_state_lock);
			if(bowl==1)
				m_bowl1 = 1;
		V(bowl_state_lock);

        V(m_bowls);
    }
}



/*
 * catmousesem()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catsem() and mousesem() threads.  Change this 
 *      code as necessary for your solution.
 */

int
catmousesem(int nargs,
            char ** args)
{
         int index, error;
        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;

	//randomize the initial eating animal
        //int initial_eating = rand()%2;
        
        /*if(initial_eating){
        	c_bowls.sem_create("c_bowls", 2);
        	m_bowls.sem_create("m_bowls", 0);
        	current_eating = c_bowls;
        	blocked = m_bowls;
		}
		else{
			c_bowls.sem_create("c_bowls", 0);
        	m_bowls.sem_create("m_bowls", 2);
        	current_eating = m_bowls;
        	blocked = c_bowls;
		}*/
        c_bowls = sem_create("c_bowls", NFOODBOWLS);
        m_bowls = sem_create("m_bowls", 0);
        bowl_state_lock = sem_create("bowl_state_lock", 1);

    	blocked = m_bowls;
    
    	current_eating = c_bowls; 

        /*
         * Start NCATS catsem() threads.
         */

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catsem Thread", 
                                    NULL, 
                                    index, 
                                    catsem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catsem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
        
        /*
         * Start NMICE mousesem() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mousesem Thread", 
                                    NULL, 
                                    index, 
                                    mousesem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mousesem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        switch_bowl();

        sem_destroy(c_bowls);
        sem_destroy(m_bowls);
        sem_destroy(bowl_state_lock);
        
        return 0;
}


/*
 * End of catsem.c
 */
