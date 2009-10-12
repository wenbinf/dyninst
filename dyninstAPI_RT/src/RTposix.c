/*
 * Copyright (c) 1996-2004 Barton P. Miller
 * 
 * We provide the Paradyn Parallel Performance Tools (below
 * described as "Paradyn") on an AS IS basis, and do not warrant its
 * validity or performance.  We reserve the right to update, modify,
 * or discontinue this software at any time.  We shall have no
 * obligation to supply such updates or modifications or any other
 * form of support to you.
 * 
 * This license is for research uses.  For such uses, there is no
 * charge. We define "research use" to mean you may freely use it
 * inside your organization for whatever purposes you see fit. But you
 * may not re-distribute Paradyn or parts of Paradyn, in any form
 * source or binary (including derivatives), electronic or otherwise,
 * to any other organization or entity without our permission.
 * 
 * (for other uses, please contact us at paradyn@cs.wisc.edu)
 * 
 * All warranties, including without limitation, any warranty of
 * merchantability or fitness for a particular purpose, are hereby
 * excluded.
 * 
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 * 
 * Even if advised of the possibility of such damages, under no
 * circumstances shall we (or any other person or entity with
 * proprietary rights in the software licensed hereunder) be liable
 * to you or any third party for direct, indirect, or consequential
 * damages of any character regardless of type of action, including,
 * without limitation, loss of profits, loss of use, loss of good
 * will, or computer failure or malfunction.  You agree to indemnify
 * us (and any other person or entity with proprietary rights in the
 * software licensed hereunder) for any and all liability it may
 * incur to third parties resulting from your use of Paradyn.
 */

/************************************************************************
 * $Id: RTposix.c,v 1.37 2008/04/11 23:30:45 legendre Exp $
 * RTposix.c: runtime instrumentation functions for generic posix.
 ************************************************************************/

#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <pwd.h>
#include "dyninstAPI_RT/h/dyninstAPI_RT.h"
#include "dyninstAPI_RT/src/RTcommon.h"
#include "dyninstAPI_RT/src/RTheap.h"

#if defined (os_osf)
#define SOCKLEN_T size_t
#else
#define SOCKLEN_T socklen_t
#endif

#if !defined(cap_save_the_world) && !(defined(arch_power) && defined(os_linux))
void RTmutatedBinary_init() 
{
    return;
}
#endif

#if defined(__GNUC) || defined(__GNUC__)
void libdyninstAPI_RT_init(void) __attribute__ ((constructor));
#endif

#if defined (cap_async_events)
struct passwd *passwd_info = NULL;
#endif

void libdyninstAPI_RT_init() 
{
   static int initCalledOnce = 0;

   rtdebug_printf("%s[%d]:  DYNINSTinit:  welcome to libdyninstAPI_RT_init()\n", __FILE__, __LINE__);

   if (initCalledOnce) return;
   initCalledOnce++;

#if defined(arch_x86) || defined(arch_x86_64) || defined(arch_ia64)
   /* Modern x86-32/x86-64/ia64 cpus have non-executable data */
   mark_heaps_exec();
#endif

   /* RTmutatedBinary_init(); */
   
   if (libdyninstAPI_RT_init_localCause != -1 && 
       libdyninstAPI_RT_init_localPid != -1 &&
       libdyninstAPI_RT_init_maxthreads != -1)
   {
      DYNINSTinit(libdyninstAPI_RT_init_localCause, libdyninstAPI_RT_init_localPid,
                  libdyninstAPI_RT_init_maxthreads, libdyninstAPI_RT_init_debug_flag);
   }

   rtdebug_printf("%s[%d]:  did DYNINSTinit\n", __FILE__, __LINE__);
}


/************************************************************************
 * void DYNINSTasyncConnect(int pid)
 *
 * Connect to mutator's async handler thread. <pid> is pid of mutator
************************************************************************/

static int async_socket = -1;
static int needToDisconnect = 0;
static char socket_path[255];

int DYNINSTasyncConnect(int pid)
{
   if (DYNINSTstaticMode)
      return 0;
#if defined (cap_async_events)

  int sock_fd;
  struct sockaddr_un sadr;
   int res;
   int mutatee_pid;
   uid_t euid;

   rtdebug_printf("%s[%d]:  DYNINSTasyncConnnect:  entry\n", __FILE__, __LINE__);
   rtdebug_printf("%s[%d]:  DYNINSTinit:  before geteuid\n", __FILE__, __LINE__);

   euid = geteuid();
   passwd_info = getpwuid(euid);
   assert(passwd_info);

  if (async_socket != -1)
  {
	  fprintf(stderr, "%s[%d]: - DYNINSTasyncConnect already initialized\n",
			  __FILE__, __LINE__);

     rtdebug_printf("%s[%d]:  DYNINSTasyncConnnect:  already connected\n", 
			 __FILE__, __LINE__);
     return 0;
  }

  rtdebug_printf("%s[%d]:  DYNINSTasyncConnnect:  before socket 2\n", __FILE__, __LINE__);
  mutatee_pid = getpid();

  snprintf(socket_path, (size_t) 255, "%s/dyninstAsync.%s.%d.%d", 
		  P_tmpdir, passwd_info->pw_name, pid, mutatee_pid);

  rtdebug_printf("%s[%d]:  DYNINSTasyncConnnect:  before socket: %s\n", __FILE__, __LINE__, socket_path);

  errno = 0;

  sock_fd = socket(PF_UNIX, SOCK_STREAM, 0);

  if (sock_fd < 0) 
  {
    fprintf(stderr, "%s[%d]: DYNINSTasyncConnect() socket(%s): %s\n", 
			__FILE__, __LINE__, socket_path, strerror(errno));
    abort();
  }

  rtdebug_printf("%s[%d]:  DYNINSTasyncConnnect:  after socket\n", __FILE__, __LINE__);

  sadr.sun_family = PF_UNIX;
  strcpy(sadr.sun_path, socket_path);

  rtdebug_printf("%s[%d]:  DYNINSTasyncConnnect:  before connect\n", __FILE__, __LINE__);
  res = 0;
  errno = 0;

  res = connect(sock_fd, (struct sockaddr *) &sadr, sizeof(sadr)); 

  if (res < 0)
  {
    perror("DYNINSTasyncConnect() connect()");
  }

  rtdebug_printf("%s[%d]:  DYNINSTasyncConnnect:  after connect to %s, res = %d, -- %s\n", 
		  __FILE__, __LINE__, socket_path, res, strerror(errno));

  /* maybe need to do fcntl to set nonblocking writes on this fd */

  if (async_socket == -1)
  {
	  rtdebug_printf("%s[%d]:  WARN:  async socket has not been reset!!\n", __FILE__, __LINE__);
  }

  async_socket = sock_fd;

  needToDisconnect = 1;

 /* atexit(exit_func); */
  rtdebug_printf("%s[%d]:  leaving DYNINSTasyncConnect\n", __FILE__, __LINE__);
  return 1; 
#else
  fprintf(stderr, "%s[%d]:  called DYNINSTasyncConect when async_events disabled\n",
		  __FILE__, __LINE__);
  return 0;
#endif
}

int DYNINSTasyncDisconnect()
{
   if (DYNINSTstaticMode)
      return 0;
   rtdebug_printf("%s[%d]:  welcome to DYNINSTasyncDisconnect\n", __FILE__, __LINE__);
   if (needToDisconnect) {
      close (async_socket);
      needToDisconnect = 0;
   }
   async_socket = -1;
   return 0;
}

int DYNINSTwriteEvent(void *ev, size_t sz)
{
  int res;

  if (DYNINSTstaticMode)
     return 0;
  
    rtdebug_printf("%s[%d]:  welcome to DYNINSTwriteEvent: %d bytes\n", __FILE__, __LINE__, sz);
  if (-1 == async_socket)
  {
	  fprintf(stderr, "%s[%d]:  failed to DYNINSTwriteEvent, no socket\n", __FILE__, __LINE__);
	  return -1;
  }

try_again:
  res = write(async_socket, ev, sz); 
  if (-1 == res) {
    if (errno == EINTR || errno == EAGAIN) 
       goto try_again;
    else {
       perror("write");
       return -1;
    }
  }
  if (res != sz) {
    /*  maybe we need logic to handle partial writes? */
    fprintf(stderr, "%s[%d]:  partial ? write error, %d bytes, should be %d\n",
            __FILE__, __LINE__, res, (int) sz);
    return -1;
  }
  return 0;
}

void *map_region(void *addr, int len, int fd) {
     void *result;
     result = mmap(addr, len, PROT_READ|PROT_WRITE|PROT_EXEC, 
                   DYNINSTheap_mmapFlags, fd, 0);
     if (result == MAP_FAILED)
         return NULL;
     return result;
}

int unmap_region(void *addr, int len) {
    int result;
    result = munmap(addr, len);
    if (result == -1)
        return 0;
    return 1;
}

#if defined(cap_mutatee_traps)
extern void dyninstTrapHandler(int sig, siginfo_t *info, void *context);

int DYNINSTinitializeTrapHandler()
{
   int result;
   struct sigaction new_handler;

   new_handler.sa_sigaction = dyninstTrapHandler;
   new_handler.sa_restorer = NULL;
   sigemptyset(&new_handler.sa_mask);
   new_handler.sa_flags = SA_SIGINFO | SA_NOMASK;
   
   result = sigaction(SIGTRAP, &new_handler, NULL);
   return (result == 0) ? 1 /*Success*/ : 0 /*Fail*/ ;
}
#endif
