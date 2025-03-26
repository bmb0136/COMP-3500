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
 * Max amount of time each animal is playing or eating
 */

#define MAXTIME 2

/*
 * Amount of seconds the simulation runs for
 */
#define RUNTIME 15

/*
 * Names of each animal
 */
static volatile const char *catNames[NCATS] = { "Charlie", "Cooper", "Chad", "Carson", "Caleb", "Claire" };
static volatile const char *mouseNames[NMICE] = { "Mickey", "Minnie" };

/*
 *
 * Shared Variables
 *
 */

// Animal-specific Data
static volatile struct semaphore *catsQueue = NULL, *miceQueue = NULL;
static volatile unsigned long catsWaiting = 0, miceWaiting = 0;

// Animal-agnostic Data
static volatile struct semaphore *mutex = NULL;
static volatile unsigned long dishesUsed = 0;
static volatile /*bool*/ char kitchenFree = 1;

// Test Driver Data
static volatile struct semaphore *doneSem = NULL;
static volatile /*bool*/ char run = 0;


/*
 * 
 * Function Definitions
 * 
 */

/*
 * Prototypes for animalsem()
 */
static void takeKitchen(const char *animalName, /*bool*/ char isCat,
                        struct semaphore *queue,
                        unsigned long *waiting);
static void enterKitchen(const char *animalName, /*bool*/ char isCat, int animalCount,
                         struct semaphore *queue,
                         unsigned long *waiting);
static void leaveKitchen(const char *animalName, /*bool*/ char isCat, struct semaphore *queue, struct semaphore *otherQueue, unsigned long *waiting, unsigned long *otherWaiting);

/*
 *
 * animalsem(): Animal-agnostic Cat & Mouse implementation
 *
 * Arguments:
 *    (bool) char isCat: Is the animal calling this function a cat or a mouse? True (non-zero) if cat, false (zero) if mouse
 *    unsigned long animalNumber: the specific animal that is calling this function
 *    unsigned long animalCount: the total number of animals of the caller's type
 *    struct semaphore *queue: the queue for animals of the caller's type
 *    struct semaphore *otherQueue: the queue for animals NOT of the caller's type
 *    unsigned long *waiting: the number of animals of the caller's type
 *    unsigned long *otherWaiting: the number of animals NOT of the caller's type
 *
 */
static
void
animalsem(/*bool*/ char isCat, unsigned long animalNumber, int animalCount,
          struct semaphore *queue, struct semaphore *otherQueue,
          unsigned long *waiting, unsigned long *otherWaiting)
{

  while (!run) {
    thread_yield();
  }

  const char *animalName = isCat ? catNames[animalNumber] : mouseNames[animalNumber];
  kprintf("!!! %s Started\n", animalName);

  while (run) {
    takeKitchen(animalName, isCat, queue, waiting);
    P(queue);
    enterKitchen(animalName, isCat, animalCount, queue, waiting);
    clocksleep(random() % MAXTIME);
    leaveKitchen(animalName, isCat, queue, otherQueue, waiting, otherWaiting);
    clocksleep(random() % MAXTIME);
  }
  kprintf("!!! %s Exited\n", animalName);
  V(doneSem);
}

/*
 * takeKitchen(): Let somebody in if the kitchen is free. Handles the case when no animals are in the kitchen (which is the case initially).
 *
 * Arguments:
 *    const char *animalName: The name of the animal that called this function
 *    (bool) char isCat: Whether the caller is a cat or not
 *    struct semaphore *queue: The queue to signal animals of the caller's type
 *    unsigned long *waiting: The number of animals of the caller's type that are waiting to enter the kitchen
 */
static
void
takeKitchen(const char *animalName, /*bool*/ char isCat,
            struct semaphore *queue,
            unsigned long *waiting)
{
  P(mutex);
  if (kitchenFree) {
    kitchenFree = 0;
    kprintf("+++ %s Letting another %s in (kitchen free)\n", animalName, isCat ? "Cat" : "Mouse");
    V(queue);
  }
  (*waiting)++;
  kprintf("*** %s %ld Hungry\n", animalName);
  V(mutex);
}

/*
 * enterKitchen(): Enter the kitchen. Lets another animal in if possible.
 *
 * Arguments:
 *    const char *animalName: The name of the animal that called this function
 *    (bool) char isCat: Whether the caller is a cat or not
 *    int animalCount: The total number of animals of the caller's type
 *    struct semaphore *queue: The queue to signal animals of the caller's type
 *    unsigned long *waiting: The number of animals of the caller's type that are waiting to enter the kitchen
 */
static
void
enterKitchen(const char *animalName, /*bool*/ char isCat, int animalCount,
                         struct semaphore *queue,
                         unsigned long *waiting)
{
    P(mutex);
    kprintf(">>> %s Entered the kitchen\n", animalName);
    dishesUsed++;
    (*waiting)--;
    if ((*waiting) && dishesUsed < (animalCount < NFOODBOWLS ? animalCount : NFOODBOWLS)) {
      kprintf("+++ %s Letting another %s in (dishes available)\n", animalName, isCat ? "Cat" : "Mouse");
      V(queue);
    }
    kprintf("... %s Eating\n", animalName);
    V(mutex);
}

/*
 * leaveKitchen(): Leaves the kitchen. Will let the opposite animal type in if possible, then try its own type, and finally will free the kitchen otherwise.
 *
 * Arguments:
 *    const char *animalName: The name of the animal that called this function
 *    (bool) char isCat: Whether the caller is a cat or not
 *    struct semaphore *queue: The queue to signal animals of the caller's type
 *    struct semaphore *otherQueue: The queue to signal animals NOT of the caller's type
 *    unsigned long *waiting: The number of animals of the caller's type that are waiting to enter the kitchen
 *    unsigned long *otherWaiting: The number of animals NOT of the caller's type that are waiting to enter the kitchen
 */
static
void
leaveKitchen(const char *animalName, /*bool*/ char isCat,
             struct semaphore *queue, struct semaphore *otherQueue,
             unsigned long *waiting, unsigned long *otherWaiting)
{
  P(mutex);
  kprintf("<<< %s Left the kitchen\n", animalName);
  dishesUsed--;
  if (dishesUsed == 0) {
    if (*otherWaiting > 0) {
      kprintf("+++ %s Letting a %s in\n", animalName, isCat ? "Mouse" : "Cat");
      V(otherQueue);
    } else if (*waiting > 0) {
      kprintf("+++ %s Letting another %s in (after leaving)\n", animalName, isCat ? "Cat" : "Mouse");
      V(queue);
    } else {
      kprintf("--- %s Freeing kitchen\n", animalName);
      kitchenFree = 1;
    }
  }
  kprintf("... %s Playing\n", animalName);
  V(mutex);
}

/*stuff
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
 *      Write and comment this function using semaphores.
 *
 */

static
void
catsem(void * unusedpointer, 
       unsigned long catnumber)
{
        (void) unusedpointer;
        animalsem(1, catnumber, NCATS,
                  catsQueue, miceQueue,
                  &catsWaiting, &miceWaiting);
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
 *      Write and comment this function using semaphores.
 *
 */

static
void
mousesem(void * unusedpointer, 
         unsigned long mousenumber)
{
        (void) unusedpointer;
        animalsem(0, mousenumber, NMICE,
                  miceQueue, catsQueue,
                  &miceWaiting, &catsWaiting);
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

        /*
         * Init shared variables
         */
        catsQueue = sem_create("Cats Queue", 0);
        if (!catsQueue) {
          panic("catmousesem: Failed to create cats queue\n");
        }
        miceQueue = sem_create("Mice Queue", 0);
        if (!miceQueue) {
          panic("catmousesem: Failed to create mice queue\n");
        }
        mutex = sem_create("Cats and Mice Mutex", 1);
        if (!mutex) {
          panic("catmousesem: Failed to create mutex\n");
        }
        doneSem = sem_create("Thread Exit Waiter", 0);
        if (!doneSem) {
          panic("catmousesem: Failed to done semaphore\n");
        }
   
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

  
        /*
         * Run simulation
         */
        kprintf("!!! Driver: Starting simulation\n");
        run = 1;
        clocksleep(RUNTIME);
        kprintf("!!! Driver: Ending simulation\n");
        run = 0;

        /*
         * Wait for all animals to finish
         */
        kprintf("!!! Driver: Waiting for threads to exit\n");
        run = 0;
        for (index = 0; index < (NCATS + NMICE); index++) {
                P(doneSem);
        }
        kprintf("!!! Driver: done\n");

        return 0;
}


/*
 * End of catsem.c
 */
