#ifndef MLOCK_WIN32
#define MLOCK_WIN32

#        define mlock(p, s) !VirtualLock(p, s)
#        define munlock(p, s) !VirtualUnlock(p, s)

#endif
