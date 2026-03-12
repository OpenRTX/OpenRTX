#ifndef _LIBS_UTILS_THREADS_H
#define _LIBS_UTILS_THREADS_H

// ------------------------------------------------------------------
// C API

enum {
    /*
     * This maps directly to the "nice" priorites we use in Android.
     * A thread priority should be chosen inverse-proportinally to
     * the amount of work the thread is expected to do. The more work
     * a thread will do, the less favorable priority it should get so that 
     * it doesn't starve the system. Threads not behaving properly might
     * be "punished" by the kernel.
     * Use the levels below when appropriate. Intermediate values are
     * acceptable, preferably use the {MORE|LESS}_FAVORABLE constants below.
     */
    DROID_PRIORITY_LOWEST         =  1,

    /* use for background tasks */
    DROID_PRIORITY_BACKGROUND     =  9,
    
    /* most threads run at normal priority */
    DROID_PRIORITY_NORMAL         =   10,
    
    /* threads currently running a UI that the user is interacting with */
    DROID_PRIORITY_FOREGROUND     =  11,

    /* the main UI thread has a slightly more favorable priority */
    DROID_PRIORITY_DISPLAY        =  11,
    
    /* ui service treads might want to run at a urgent display (uncommon) */
    DROID_PRIORITY_URGENT_DISPLAY =  11,
    
    /* all normal audio threads */
    DROID_PRIORITY_AUDIO          = 12,
    
    /* service audio threads (uncommon) */
    DROID_PRIORITY_URGENT_AUDIO   = 13,

    /* should never be used in practice. regular process might not 
     * be allowed to use this level */
    DROID_PRIORITY_HIGHEST        = 31,

    DROID_PRIORITY_DEFAULT        = DROID_PRIORITY_NORMAL,
    DROID_PRIORITY_MORE_FAVORABLE = -1,
    DROID_PRIORITY_LESS_FAVORABLE = +1,
};

#endif // _LIBS_UTILS_THREADS_H
