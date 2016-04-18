/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>
////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *namearg, int initial_count)
{
	struct semaphore *sem;

	assert(initial_count >= 0);

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
	splx(spl);

	/*
	 * Note: while someone could theoretically start sleeping on
	 * the semaphore after the above test but before we free it,
	 * if they're going to do that, they can just as easily wait
	 * a bit and start sleeping on the semaphore after it's been
	 * freed. Consequently, there's not a whole lot of point in 
	 * including the kfrees in the splhigh block, so we don't.
	 */

	kfree(sem->name);
	kfree(sem);
}

void 
P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void
V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name);
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}
	
	// add stuff here as needed
	// init the locker holder 
	// (NULL since no one hold it when first created)
	lock->holder = NULL;
	
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	int spl;
	assert(lock != NULL);

	// add stuff here as needed
	// disable interuptions
	spl = splhigh();
	
	// make sure no one holds this lock
	// and no one is waiting for this lock
	assert(lock->holder == NULL);
	assert(thread_hassleepers(lock) == 0);
	
	//enable interuptions again
	splx(spl);	
	
	kfree(lock->name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	int spl;
	assert(lock != NULL);
	
	// disable interuptions
	spl = splhigh();
	
	// make sure that the lock is not holding by the current thread
	assert(lock_do_i_hold(lock) == 0);
	//assert(lock->holder != curthread);

	// if the lock currently occupied, go sleep	
	while(lock->holder != NULL){
		thread_sleep(lock);
	}
	
	// change the thread holder after get the access
	lock->holder = curthread;

	// enable interuption again
	splx(spl);
}

void
lock_release(struct lock *lock)
{
	int spl;
	assert(lock != NULL);
	
	// disable interuptions
	spl = splhigh();
	
	// make sure that the lock is holding by the current thread
	assert(lock_do_i_hold(lock) == 1);
	//assert(lock->holder == curthread);

	// unlock and wake up other threads
	lock->holder = NULL;
	thread_wakeup(lock);
	
	// enable interuptions again
	splx(spl);
}

int
lock_do_i_hold(struct lock *lock)
{
	int spl;
	int holded;
	assert(lock != NULL);
	
	// disable interuptions
	spl = splhigh();

	//check the current holder
	if(lock->holder == curthread)
		holded = 1;
	else
		holded = 0;

	// enable interuptions
	splx(spl);
	return holded;
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}	
	
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);
	
	kfree(cv->name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	int spl;
	
	//check whether cv or lock is empty
	assert(cv != NULL);
	assert(lock != NULL);
	
	//disable interrupts
	spl = splhigh();
	
	//lock must be held by the thread when cv_wait function is called
	assert(lock_do_i_hold(lock));
	
	//release the lock, go to sleep and require the lock again
	lock_release(lock);
	thread_sleep(cv);
	lock_acquire(lock);
	
	//enable interrupts
	splx(spl);
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	int spl;
	
	//check whether cv or lock is empty
	assert(cv != NULL);
	assert(lock != NULL);
	
	//disable interrupts
	spl = splhigh();
	
	//lock must be held by the thread when cv_wait function is called
	assert(lock_do_i_hold(lock));
	
	//wake up the first thread on the queue which is blocked by cv
	thread_wakeupfirst(cv);
	
	//enable interrupts
	splx(spl);
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	int spl;
	
	//check whether cv or lock is empty
	assert(cv != NULL);
	assert(lock != NULL);
	
	//disable interrupts
	spl = splhigh();
	
	//lock must be held by the thread when cv_wait function is called
	assert(lock_do_i_hold(lock));
	
	//wake up all the threads sleeping on the cv
	thread_wakeup(cv);
	
	//enable interrupts
	splx(spl);
}

//==================================================================spinlock
struct spinlock *
spinlock_create(const char *name)
{
	struct spinlock *spinlock;

	spinlock = kmalloc(sizeof(struct spinlock));
	if (spinlock == NULL) {
		return NULL;
	}

	spinlock->name = kstrdup(name);
	if (spinlock->name == NULL) {
		kfree(spinlock);
		return NULL;
	}
	
	// add stuff here as needed
	// init the spinlocker holder 
	// (NULL since no one hold it when first created)
	spinlock->holder = NULL;
	
	return spinlock;
}

void
spinlock_destroy(struct spinlock *spinlock)
{
	int spl;
	assert(spinlock != NULL);

	// add stuff here as needed
	// disable interuptions
	spl = splhigh();
	
	// make sure no one holds this spinlock
	// and no one is waiting for this spinlock
	assert(spinlock->holder == NULL);
	assert(thread_hassleepers(spinlock) == 0);
	
	//enable interuptions again
	splx(spl);	
	
	kfree(spinlock->name);
	kfree(spinlock);
}

void
spinlock_acquire(struct spinlock *spinlock)
{
	int spl;
	assert(spinlock != NULL);
	
	// disable interuptions
	spl = splhigh();
	
	// make sure that the spinlock is not holding by the current thread
	assert(spinlock_do_i_hold(spinlock) == 0);
	//assert(spinlock->holder != curthread);

	// if the spinlock currently occupied, go sleep	
	while(spinlock->holder != NULL){
		
	}
	
	// change the thread holder after get the access
	spinlock->holder = curthread;

	// enable interuption again
	splx(spl);
}

void
spinlock_release(struct spinlock *spinlock)
{
	int spl;
	assert(spinlock != NULL);
	
	// disable interuptions
	spl = splhigh();
	
	// make sure that the spinlock is holding by the current thread
	assert(spinlock_do_i_hold(spinlock) == 1);
	//assert(spinlock->holder == curthread);

	// unspinlock and wake up other threads
	spinlock->holder = NULL;
	thread_wakeup(spinlock);
	
	// enable interuptions again
	splx(spl);
}

int
spinlock_do_i_hold(struct spinlock *spinlock)
{
	int spl;
	int holded;
	assert(spinlock != NULL);
	
	// disable interuptions
	spl = splhigh();

	//check the current holder
	if(spinlock->holder == curthread)
		holded = 1;
	else
		holded = 0;

	// enable interuptions
	splx(spl);
	return holded;
}
