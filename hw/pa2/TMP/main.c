#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lock.h>

#define DEFAULT_LOCK_PRIO 20

#define assert(x,error) if(!(x)){ \
            kprintf(error);\
            return;\
            }
int mystrncmp(char* des,char* target,int n){
    int i;
    for (i=0;i<n;i++){
        if (target[i] == '.') continue;
        if (des[i] != target[i]) return 1;
    }
    return 0;
}

/*--------------------------------Test 1--------------------------------*/
 
void reader1 (char *msg, int lck)
{
	int lock_rt = lock (lck, READ, DEFAULT_LOCK_PRIO);
        if (lock_rt == SYSERR) {
                kprintf("Failed to acquired lock\n");
        }
	kprintf ("  %s: acquired lock, sleep 2s\n", msg);
	sleep (3);
	kprintf ("  %s: to release lock\n", msg);
	releaseall (1, lck);
}

void test1 ()
{
	int	lck;
	int	pid1;
	int	pid2;

	kprintf("\nTest 1: readers can share the rwlock\n");
	lck  = lcreate ();
	assert (lck != SYSERR, "Test 1 failed");

        pid1 = create(reader1, 2000, 20, "reader a", 2, "reader a", lck);

        pid2 = create(reader1, 2000, 20, "reader b", 2, "reader b", lck);
        
        if (pid1 == SYSERR || pid2 == SYSERR) {
            kprintf("ERROR: Process creation failed\n");
	    ldelete (lck);
            return;
        }        

	resume(pid1);
	resume(pid2);
	
	sleep (5);
	ldelete (lck);
	kprintf ("Test 1 ok\n");
}

/*----------------------------------Test 2---------------------------*/
char output2[10];
int count2;
void reader2 (char msg, int lck, int lprio)
{
        int     ret;

        kprintf ("  %c: to acquire lock\n", msg);
        lock (lck, READ, lprio);
        output2[count2++]=msg;
        kprintf ("  %c: acquired lock, sleep 3s\n", msg);
        sleep (3);
        output2[count2++]=msg;
        kprintf ("  %c: to release lock\n", msg);
	releaseall (1, lck);
}

void writer2 (char msg, int lck, int lprio)
{
	kprintf ("  %c: to acquire lock\n", msg);
        lock (lck, WRITE, lprio);
        output2[count2++]=msg;
        kprintf ("  %c: acquired lock, sleep 3s\n", msg);
        sleep (3);
        output2[count2++]=msg;
        kprintf ("  %c: to release lock\n", msg);
        releaseall (1, lck);
}

void test2 ()
{
        count2 = 0;
        int     lck;
        int     rd1, rd2, rd3, rd4;
        int     wr1;

        kprintf("\nTest 2: wait on locks with priority. Expected order of"
		" lock acquisition is: reader A, reader B, reader D, writer C & reader E\n");
        lck  = lcreate ();
        assert (lck != SYSERR, "Test 2 failed");

        rd1 = create(reader2, 2000, 20, "reader2", 3, 'A', lck, 20);
        rd2 = create(reader2, 2000, 20, "reader2", 3, 'B', lck, 30);
        rd3 = create(reader2, 2000, 20, "reader2", 3, 'D', lck, 25);
        rd4 = create(reader2, 2000, 20, "reader2", 3, 'E', lck, 20);
        wr1 = create(writer2, 2000, 20, "writer2", 3, 'C', lck, 28);
	
        kprintf("-start reader A, then sleep 1s. lock granted to reader A\n");
        resume(rd1);
        sleep (1);

        kprintf("-start writer C, then sleep 1s. writer waits for the lock\n");
        resume(wr1);
        sleep10 (1);


        kprintf("-start reader B, D, E. reader B is granted lock.\n");
        resume (rd2);
        resume (rd3);
        resume (rd4);

        sleep (15);
        kprintf("output=%s\n", output2);
        assert(mystrncmp(output2,"ABABCCDEED",10)==0,"Test 2 FAILED\n");
        kprintf ("Test 2 OK\n");
}

/*----------------------------------Test 3---------------------------*/
void reader3 (char *msg, int lck)
{
        int     ret;

        kprintf ("  %s: to acquire lock\n", msg);
        lock (lck, READ, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock\n", msg);
        kprintf ("  %s: to release lock\n", msg);
        releaseall (1, lck);
}

void writer3 (char *msg, int lck)
{
        kprintf ("  %s: to acquire lock\n", msg);
        lock (lck, WRITE, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock, sleep 10s\n", msg);
        sleep (10);
        kprintf ("  %s: to release lock\n", msg);
        releaseall (1, lck);
}

void test3 ()
{
        int     lck;
        int     rd1, rd2;
        int     wr1;

        kprintf("\nTest 3: test the basic priority inheritence\n");
        lck  = lcreate ();
        assert (lck != SYSERR, "Test 3 failed (1)");

        rd1 = create(reader3, 2000, 25, "reader3", 2, "reader A", lck);
        rd2 = create(reader3, 2000, 30, "reader3", 2, "reader B", lck);
        wr1 = create(writer3, 2000, 20, "writer3", 2, "writer", lck);

        kprintf("-start writer, then sleep 1s. lock granted to write (prio 20)\n");
        resume(wr1);
        sleep (1);

        kprintf("-start reader A, then sleep 1s. reader A(prio 25) blocked on the lock\n");
        resume(rd1);
        sleep (1);
        int test_prio = getprio(wr1);
        kprintf("Writer 1 priority = %d\n", test_prio);
        assert (test_prio == 25, "Test 3 failed (2)");

        kprintf("-start reader B, then sleep 1s. reader B(prio 30) blocked on the lock\n");
        resume (rd2);
        sleep (1);
        assert (getprio(wr1) == 30, "Test 3 failed (3)");
        
        kprintf("-kill reader B, then sleep 1s\n");
        kill (rd2);
        sleep (1);
        assert (getprio(wr1) == 25, "Test 3 failed (4)");

        kprintf("-kill reader A, then sleep 1s\n");
        kill (rd1);
        sleep(1);
        assert(getprio(wr1) == 20, "Test 3 failed (5)");

        sleep (8);
        kprintf ("Test 3 OK\n");
}

void writer4 (char *msg, int lck)
{
    kprintf ("  %s: trying to acquire write lock\n", msg);
    lock (lck, WRITE, DEFAULT_LOCK_PRIO);
    kprintf ("  %s: acquired write lock, sleep 3s\n", msg);
    sleep (3);
    kprintf ("  %s: releasing write lock\n", msg);
    releaseall (1, lck);
}

void test4 ()
{
    int lck;
    int pid1, pid2;

    kprintf("\nTest 4: writer and writer - ensure mutual exclusion\n");
    lck  = lcreate ();
    assert (lck != SYSERR, "Test 4 failed");

    pid1 = create(writer4, 2000, 20, "writer A", 2, "writer A", lck);
    pid2 = create(writer4, 2000, 20, "writer B", 2, "writer B", lck);

    resume(pid1);
    sleep(1);

    resume(pid2);
    sleep(5);

    ldelete(lck);
    kprintf ("Test 4 OK\n");
}


void reader5 (char *msg, int lck)
{
    kprintf ("  %s: trying to acquire read lock\n", msg);
    lock (lck, READ, DEFAULT_LOCK_PRIO);
    kprintf ("  %s: acquired read lock, sleep 3s\n", msg);
    sleep (3);
    kprintf ("  %s: releasing read lock\n", msg);
    releaseall (1, lck);
}

void writer5 (char *msg, int lck)
{
    kprintf ("  %s: trying to acquire write lock\n", msg);
    lock (lck, WRITE, DEFAULT_LOCK_PRIO);
    kprintf ("  %s: acquired write lock, sleep 3s\n", msg);
    sleep (3);
    kprintf ("  %s: releasing write lock\n", msg);
    releaseall (1, lck);
}

void test5 ()
{
    int lck;
    int pid1, pid2;

    kprintf("\nTest 5: writer and reader - writer should block reader\n");
    lck  = lcreate ();
    assert (lck != SYSERR, "Test 5 failed");

    pid1 = create(writer5, 2000, 20, "writer", 2, "writer", lck);
    pid2 = create(reader5, 2000, 20, "reader", 2, "reader", lck);

    resume(pid1);
    sleep(1);

    resume(pid2);
    sleep(5);

    ldelete(lck);
    kprintf ("Test 5 OK\n");
}


void reader6 (char *msg, int lck)
{
    kprintf ("  %s: trying to acquire read lock\n", msg);
    lock (lck, READ, DEFAULT_LOCK_PRIO);
    kprintf ("  %s: acquired read lock, sleep 3s\n", msg);
    sleep (3);
    kprintf ("  %s: releasing read lock\n", msg);
    releaseall (1, lck);
}

void writer6 (char *msg, int lck)
{
    kprintf ("  %s: trying to acquire write lock\n", msg);
    lock (lck, WRITE, DEFAULT_LOCK_PRIO);
    kprintf ("  %s: acquired write lock, sleep 3s\n", msg);
    sleep (3);
    kprintf ("  %s: releasing write lock\n", msg);
    releaseall (1, lck);
}

void test6 ()
{
    int lck;
    int pid1, pid2;

    kprintf("\nTest 6: reader first, writer waits until reader releases\n");
    lck  = lcreate ();
    assert (lck != SYSERR, "Test 6 failed");

    pid1 = create(reader6, 2000, 20, "reader", 2, "reader", lck);
    pid2 = create(writer6, 2000, 20, "writer", 2, "writer", lck);

    resume(pid1);
    sleep(1);

    resume(pid2);
    sleep(5);

    ldelete(lck);
    kprintf ("Test 6 OK\n");
}


void reader7 (char *msg, int lck)
{
    kprintf ("  %s: trying to acquire read lock\n", msg);
    lock (lck, READ, DEFAULT_LOCK_PRIO);
    kprintf ("  %s: acquired read lock, sleep 3s\n", msg);
    sleep (3);
    kprintf ("  %s: releasing read lock\n", msg);
    releaseall (1, lck);
}

void writer7 (char *msg, int lck)
{
    kprintf ("  %s: trying to acquire write lock\n", msg);
    lock (lck, WRITE, DEFAULT_LOCK_PRIO);
    kprintf ("  %s: acquired write lock, sleep 3s\n", msg);
    sleep (3);
    kprintf ("  %s: releasing write lock\n", msg);
    releaseall (1, lck);
}

void test7 ()
{
    int lck;
    int pid1, pid2, pid3, pid4;

    kprintf("\nTest 7: Multiple readers, then writer waits until all readers release\n");
    lck  = lcreate ();
    assert (lck != SYSERR, "Test 7 failed");

    pid1 = create(reader7, 2000, 20, "reader A", 2, "reader A", lck);
    pid2 = create(reader7, 2000, 20, "reader B", 2, "reader B", lck);
    pid3 = create(reader7, 2000, 20, "reader C", 2, "reader C", lck);
    pid4 = create(writer7, 2000, 20, "writer", 2, "writer", lck);

    resume(pid1);
    resume(pid2);
    resume(pid3);
    sleep(1);

    resume(pid4);
    sleep(6);

    ldelete(lck);
    kprintf ("Test 7 OK\n");
}

void writer8 (char *msg, int lck, int prio)
{
    kprintf ("  %s: trying to acquire write lock with priority %d\n", msg, prio);
    int ret = lock(lck, WRITE, prio);
    
    if (ret == SYSERR) {
        kprintf("  %s: failed to acquire lock, it no longer exists\n", msg);
    } else {
        kprintf ("  %s: acquired write lock, sleep 3s\n", msg);
        sleep (3);
        kprintf ("  %s: releasing write lock\n", msg);
        releaseall (1, lck);
    }
}

void test8 ()
{
    int lck;
    int pid1, pid2, pid3;

    kprintf("\nTest 8: Multiple writers competing with different priorities\n");
    lck  = lcreate ();
    assert (lck != SYSERR, "Test 8 failed");

    pid1 = create(writer8, 2000, 20, "writer LOW", 3, "writer LOW", lck, 20);
    resume(pid1);
    sleep(1);

    pid2 = create(writer8, 2000, 20, "writer HIGH", 3, "writer HIGH", lck, 40);
    resume(pid2);

    int pid4 = create(writer8, 2000, 20, "writer MAX", 3, "writer MAX", lck, 50);
    resume(pid4);
    
    pid3 = create(writer8, 2000, 20, "writer MID", 3, "writer MID", lck, 30);
    resume(pid3);

    sleep(15);

    ldelete(lck);
    kprintf ("Test 8 OK\n");
}


void reader9 (char *msg, int lck, int prio)
{
    kprintf ("  %s: trying to acquire read lock (prio %d)\n", msg, prio);
    lock (lck, READ, prio);
    kprintf ("  %s: acquired read lock, sleep 3s\n", msg);
    sleep (3);
    kprintf ("  %s: releasing read lock\n", msg);
    releaseall (1, lck);
}

void writer9 (char *msg, int lck, int prio)
{
    kprintf ("  %s: trying to acquire write lock (prio %d)\n", msg, prio);
    lock (lck, WRITE, prio);
    kprintf ("  %s: acquired write lock, sleep 3s\n", msg);
    sleep (3);
    kprintf ("  %s: releasing write lock\n", msg);
    releaseall (1, lck);
}

void test9 ()
{
    int lck;

    kprintf("\nTest 9: Different priority readers and writers\n");
    lck  = lcreate ();
    assert (lck != SYSERR, "Test 9 failed");

    int pid1 = create(reader9, 2000, 20, "reader A", 3, "reader A", lck, 20);
    int pid2 = create(writer9, 2000, 20, "writer B", 3, "writer B", lck, 30);
    int pid3 = create(reader9, 2000, 20, "reader C", 3, "reader C", lck, 25);
    int pid4 = create(writer9, 2000, 20, "writer D", 3, "writer D", lck, 35);
    int pid5 = create(reader9, 2000, 20, "reader E", 3, "reader E", lck, 33);
    int pid6 = create(reader9, 2000, 20, "reader F", 3, "reader F", lck, 27);

    // A -> D -> E 

    resume(pid1);
    sleep(1);

    resume(pid2);
    resume(pid3);
    resume(pid4);
    resume(pid5);
    resume(pid6);
    sleep(25);

    ldelete(lck);
    kprintf ("Test 9 OK\n");
}

void reader10 (char *msg, int lck)
{
    kprintf ("  %s: trying to acquire read lock\n", msg);
    int ret = lock(lck, READ, DEFAULT_LOCK_PRIO);
    
    if (ret == DELETED) {
        kprintf("  %s: lock was deleted while waiting\n", msg);
    } else if (ret == SYSERR) {
        kprintf("  %s: failed to acquire lock, it no longer exists\n", msg);
    } else {
        kprintf ("  %s: acquired read lock, sleep 5s\n", msg);
        sleep (5);
        kprintf ("  %s: releasing read lock\n", msg);
        releaseall(1, lck);
    }
}


void writer10 (char *msg, int lck)
{
    kprintf ("  %s: trying to acquire write lock (prio %d)\n", msg, DEFAULT_LOCK_PRIO);
    int ret = lock (lck, WRITE, DEFAULT_LOCK_PRIO);

    if (ret == SYSERR) {
        kprintf("  %s: failed to acquire lock\n", msg);
    } else if (ret == DELETED) {
        kprintf("  %s: lock is DELETED while waiting\n", msg);

    } else {
        kprintf ("  %s: acquired write lock, sleep 3s\n", msg);
        sleep (3);
        kprintf ("  %s: releasing write lock\n", msg);
        releaseall (1, lck);
    }
}


void test10 ()
{
    int lck;
    int pid1, pid2, pid3;

    kprintf("\nTest: Lock deletion while writers are waiting\n");
    lck  = lcreate ();
    assert (lck != SYSERR, "Test failed: Unable to create lock\n");

    pid1 = create(writer10, 2000, 20, "writer A", 2, "writer A", lck);
    pid2 = create(writer10, 2000, 20, "writer B", 2, "writer B", lck);
    pid3 = create(writer10, 2000, 20, "writer C", 2, "writer C", lck);
    resume(pid1);
    sleep(1);

    resume(pid2);
    resume(pid3);
    sleep(2);

    kprintf("  writer A: deleting the lock while writers are waiting\n");
    ldelete(lck);

    sleep(5);
    kprintf ("Test OK\n");
}

void reader11 (char *msg, int lck1, int lck2)
{
    kprintf ("  %s: trying to acquire read lock on %d and %d\n", msg, lck1, lck2);
    lock(lck1, READ, DEFAULT_LOCK_PRIO);
    lock(lck2, READ, DEFAULT_LOCK_PRIO);
    kprintf ("  %s: acquired both locks, sleep 2s\n", msg);
    sleep (3);
    kprintf ("  %s: releasing both locks\n", msg);
    releaseall(2, lck1, lck2);
}

void reader_waiting11 (char *msg, int lck)
{
    kprintf ("  %s: trying to acquire read lock on %d\n", msg, lck);
    lock(lck, READ, DEFAULT_LOCK_PRIO);
    kprintf ("  %s: acquired read lock, sleep 2s\n", msg);
    sleep (3);
    kprintf ("  %s: releasing read lock\n", msg);
    releaseall(1, lck);
}

void test11 ()
{
    int lck1, lck2;
    int pid1, pid2, pid3;

    kprintf("\nTest 11: releaseall() should correctly release multiple locks\n");
    lck1 = lcreate();
    lck2 = lcreate();
    assert (lck1 != SYSERR && lck2 != SYSERR, "Test 11 failed: Unable to create locks\n");

    pid1 = create(reader11, 2000, 20, "reader A", 3, "reader A", lck1, lck2);
    pid2 = create(reader_waiting11, 2000, 20, "reader B", 2, "reader B", lck1);
    pid3 = create(reader_waiting11, 2000, 20, "reader C", 2, "reader C", lck2);

    resume(pid1);
    sleep(1);
    resume(pid2);
    resume(pid3);
    
    sleep(5);
    ldelete(lck1);
    ldelete(lck2);
    kprintf ("Test 11 OK\n");
}

void reader12 (char *msg, int lck)
{
    kprintf ("  %s: trying to acquire read lock %d\n", msg, lck);
    int ret = lock(lck, READ, DEFAULT_LOCK_PRIO);
    
    if (ret == SYSERR) {
        kprintf("  %s: failed to acquire lock, it no longer exists\n", msg);
    } else {
        kprintf ("  %s: acquired read lock, sleep 2s\n", msg);
        sleep (3);
        kprintf ("  %s: releasing read lock\n", msg);
        releaseall(1, lck);
    }
}

void test12 ()
{
    int lck1, lck2;
    int pid1, pid2;

    kprintf("\nTest 12: Reuse of lock ID should not confuse old processes\n");
    lck1 = lcreate();
    assert (lck1 != SYSERR, "Test 12 failed: Unable to create lock\n");

    pid1 = create(reader12, 2000, 20, "reader A", 2, "reader A", lck1);

    resume(pid1);
    sleep(2);

    kprintf("  Main process: Deleting lock %d\n", lck1);
    ldelete(lck1);

    lck2 = lcreate();
    kprintf("  Main process: Created new lock %d\n", lck2);

    pid2 = create(reader12, 2000, 20, "reader B", 2, "reader B", lck1);
    resume(pid2);
    
    sleep(3);
    ldelete(lck2);
    kprintf ("Test 12 OK\n");
}

void reader13 (char *msg, int lck, int prio)
{
    kprintf ("  %s: trying to acquire read lock (prio %d)\n", msg, prio);
    int ret = lock (lck, READ, prio);

    if (ret == SYSERR) {
        kprintf("  %s: failed to acquire lock\n", msg);
    } else {
        kprintf ("  %s: acquired read lock, sleep 3s\n", msg);
        sleep (3);
        kprintf ("  %s: releasing read lock\n", msg);
        releaseall (1, lck);
    }
}

void writer13 (char *msg, int lck, int prio)
{
    kprintf ("  %s: trying to acquire write lock (prio %d)\n", msg, prio);
    int ret = lock (lck, WRITE, prio);

    if (ret == SYSERR) {
        kprintf("  %s: failed to acquire lock\n", msg);
    } else {
        kprintf ("  %s: acquired write lock, sleep 3s\n", msg);
        sleep (3);
        kprintf ("  %s: releasing write lock\n", msg);
        releaseall (1, lck);
    }
}

void test13 ()
{
    int lck;
    int pid1, pid2, pid3, pid4, pid5, pid6, pid7;

    kprintf("\nTest 13: If the next candidate is a reader, all readers with priority > highest waiting writer should be woken up\n");

    lck  = lcreate ();
    assert (lck != SYSERR, "Test 13 failed: Unable to create lock\n");

    pid1 = create(reader13, 2000, 20, "reader A", 3, "reader A", lck, 20);
    resume(pid1);
    sleep(1);

    pid2 = create(writer13, 2000, 20, "writer B", 3, "writer B", lck, 30);
    resume(pid2);
    sleep(1);

    pid3 = create(reader13, 2000, 20, "reader C", 3, "reader C", lck, 25);
    pid4 = create(reader13, 2000, 20, "reader D", 3, "reader D", lck, 40);
    pid5 = create(reader13, 2000, 20, "reader E", 3, "reader E", lck, 35);
    pid6 = create(reader13, 2000, 20, "reader F", 3, "reader F", lck, 33);
    pid7 = create(reader13, 2000, 20, "reader G", 3, "reader G", lck, 27);

    resume(pid3);
    resume(pid4);
    resume(pid5);
    resume(pid6);
    resume(pid7);

    sleep(5);

    ldelete(lck);
    kprintf("Test 13 OK\n");
}



int main()
{
    kprintf("main() start\n");

    /* These test cases are only used for test purposes.
     * The provided results do not guarantee your correctness.
     * You need to read the PA2 instruction carefully.
    */
	test1(); // readers can share the rwlock
        
	//test2();
        
	//test3();
        
	//test4();  // writer and writer - ensure mutual exclusion

	//test5(); // writer first then reader

	//test6(); // reader first, writer waits until reader releases

	//test7();  // Multiple readers, then writer waits until all readers release

	//test8();  // Multiple writers competing with different priorities

	//test9();  // Different priority readers and writers

	//test10();  // Process waiting on a lock should receive DELETED if the lock is deleted

	//test11();  // Releasing multiple lock at once to test variable parameter

	//test12();  // Reuse of lock ID should not confuse old processes

	//test13();  // Wake up reader if there is writer waiting


    //sleep(2);
	//test2();
	//test3();

    /* The hook to shutdown QEMU for process-like execution of XINU.
    * This API call exists the QEMU process.
    */
    shutdown();
}




