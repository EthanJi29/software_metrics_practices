// 2003-05-03  Loren J. Rittle <rittle@labs.mot.com> <ljrittle@acm.org>
//
// Copyright (C) 2003, 2004 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// { dg-do run { target *-*-freebsd* *-*-netbsd* *-*-linux* *-*-solaris* *-*-cygwin *-*-darwin* alpha*-*-osf* } }
// { dg-options "-pthread" { target *-*-freebsd* *-*-netbsd* *-*-linux* alpha*-*-osf* } }
// { dg-options "-pthreads" { target *-*-solaris* } }

#include <ext/rope>
#include <cstring>
#include <testsuite_hooks.h>

// Do not include <pthread.h> explicitly; if threads are properly
// configured for the port, then it is picked up free from STL headers.

#if __GTHREADS

const int max_thread_count = 4;
const int max_loop_count = 10000;

__gnu_cxx::crope foo4;

void* thread_main(void *) 
{
  // To see a problem with gcc 3.3 and before, set a break point here.
  // Single step through c_str implementation, call sched_yield after
  // capture of NULL __old_c_string in any thread.  Single step
  // through another thread past that same point.  Now, one thread
  // will receive a bad pointer return.  Adding dummy sched_yield
  // should never change program semantics under POSIX threads.
  const char* data4 = foo4.c_str();

  // Please note that the memory leak in the rope implementation with
  // this test case, existed before and after fixing this bug...
  bool test __attribute__((unused)) = true;
  VERIFY( !std::strcmp (data4, "barbazbonglehellohellohello") );
  return 0;
}

#if !__GXX_WEAK__ && _MT_ALLOCATOR_H
// Explicitly instantiate for systems with no COMDAT or weak support.
template class __gnu_cxx::__mt_alloc<__gnu_cxx::_Rope_RopeLeaf<char, std::allocator<char> > >;
template class __gnu_cxx::__mt_alloc<__gnu_cxx::_Rope_RopeFunction<char, std::allocator<char> > >;
template class __gnu_cxx::__mt_alloc<__gnu_cxx::_Rope_RopeSubstring<char, std::allocator<char> > >;
template class __gnu_cxx::__mt_alloc<__gnu_cxx::_Rope_RopeConcatenation<char, std::allocator<char> > >;
#endif                                                                

int
main()
{
  bool test __attribute__((unused)) = true;

  pthread_t tid[max_thread_count];

#if defined(__sun) && defined(__svr4__)
  pthread_setconcurrency (max_thread_count);
#endif

  __gnu_cxx::crope foo;
  foo += "bar";
  foo += "baz";
  foo += "bongle";
  const char* data = foo.c_str();
  VERIFY( !std::strcmp (data, "barbazbongle") );

  const char* data2;
  {
    __gnu_cxx::crope foo2;
    foo2 += "bar2";
    foo2 += "baz2";
    foo2 += "bongle2";
    data2 = foo2.c_str();
    VERIFY( !std::strcmp (data2, "bar2baz2bongle2") );
  }

  __gnu_cxx::crope foo3 ("hello");
  const char* data3 = foo3.c_str();
  VERIFY( !std::strcmp (data3, "hello") );

  for (int j = 0; j < max_loop_count; j++)
    {
      foo4 = foo;
      foo4 += foo3;
      foo4 += foo3;
      foo4 += foo3;

      for (int i = 0; i < max_thread_count; i++)
	pthread_create (&tid[i], NULL, thread_main, 0);

      for (int i = 0; i < max_thread_count; i++)
	pthread_join (tid[i], NULL);
    }

  // Nothing says the data will be trashed at this point...
  VERIFY( std::strcmp (data2, "bar2baz2bongle2") );

  return 0;
}
#else
int main (void) {}
#endif
