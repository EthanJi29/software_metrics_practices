// Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002, 2003
// Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

#include <clocale>
#include <cstring>
#include <locale>

namespace std
{
  using namespace __gnu_cxx;

  
  locale::locale(const char* __s)
  {
    if (__s)
      {
	_S_initialize(); 
	if (std::strcmp(__s, "C") == 0 || std::strcmp(__s, "POSIX") == 0)
	  (_M_impl = _S_classic)->_M_add_reference();
	else if (std::strcmp(__s, "") != 0)
	  _M_impl = new _Impl(__s, 1);
	else
	  {
	    // Get it from the environment.
	    char* __env = std::getenv("LC_ALL");
	    // If LC_ALL is set we are done.
	    if (__env && std::strcmp(__env, "") != 0)
	      {
		if (std::strcmp(__env, "C") == 0 
		    || std::strcmp(__env, "POSIX") == 0)
		  (_M_impl = _S_classic)->_M_add_reference();
		else
		  _M_impl = new _Impl(__env, 1);
	      }
	    else
	      {
		// LANG may set a default different from "C".
		string __res;
		char* __env = std::getenv("LANG");
		if (!__env || std::strcmp(__env, "") == 0 
		    || std::strcmp(__env, "C") == 0 
		    || std::strcmp(__env, "POSIX") == 0)
		  __res = "C";
		else 
		  __res = __env;
		
		// Scan the categories looking for the first one
		// different from LANG.
		size_t __i = 0;
		if (__res == "C")
		  for (; __i < _S_categories_size; ++__i)
		    {
		      __env = std::getenv(_S_categories[__i]);
		      if (__env && std::strcmp(__env, "") != 0 
			  && std::strcmp(__env, "C") != 0 
			  && std::strcmp(__env, "POSIX") != 0)
			break;
		    }
		else
		  for (; __i < _S_categories_size; ++__i)
		    {
		      __env = std::getenv(_S_categories[__i]);
		      if (__env && std::strcmp(__env, "") != 0
			  && __res != __env)
			break;
		    }
	
		// If one is found, build the complete string of
		// the form LC_CTYPE=xxx;LC_NUMERIC=yyy; and so on...
		if (__i < _S_categories_size)
		  {
		    string __str;
		    for (size_t __j = 0; __j < __i; ++__j)
		      {
			__str += _S_categories[__j];
			__str += '=';
			__str += __res;
			__str += ';';
		      }
		    __str += _S_categories[__i];
		    __str += '=';
		    __str += __env;
		    __str += ';';
		    __i++;
		    for (; __i < _S_categories_size; ++__i)
		      {
			__env = std::getenv(_S_categories[__i]);
			if (!__env || std::strcmp(__env, "") == 0)
			  {
			    __str += _S_categories[__i];
			    __str += '=';
			    __str += __res;
			    __str += ';';
			  }
			else if (std::strcmp(__env, "C") == 0
				 || std::strcmp(__env, "POSIX") == 0)
			  {
			    __str += _S_categories[__i];
			    __str += "=C;";
			  }
			else
			  {
			    __str += _S_categories[__i];
			    __str += '=';
			    __str += __env;
			    __str += ';';
			  }
		      }
		    __str.erase(__str.end() - 1);
		    _M_impl = new _Impl(__str.c_str(), 1);
		  }
		// ... otherwise either an additional instance of
		// the "C" locale or LANG.
		else if (__res == "C")
		  (_M_impl = _S_classic)->_M_add_reference();
		else
		  _M_impl = new _Impl(__res.c_str(), 1);
	      }
	  }
      }
    else
      __throw_runtime_error(__N("locale::locale NULL not valid"));
  }

  locale::locale(const locale& __base, const char* __s, category __cat)
  { 
    // NB: There are complicated, yet more efficient ways to do
    // this. Building up locales on a per-category way is tedious, so
    // let's do it this way until people complain.
    locale __add(__s);
    _M_coalesce(__base, __add, __cat);
  }

  locale::locale(const locale& __base, const locale& __add, category __cat)
  { _M_coalesce(__base, __add, __cat); }

  void
  locale::_M_coalesce(const locale& __base, const locale& __add, 
		      category __cat)
  {
    __cat = _S_normalize_category(__cat);  
    _M_impl = new _Impl(*__base._M_impl, 1);  

    try 
      { _M_impl->_M_replace_categories(__add._M_impl, __cat); }
    catch (...) 
      { 
	_M_impl->_M_remove_reference(); 
	__throw_exception_again;
      }
  }

  // Construct named _Impl.
  locale::_Impl::
  _Impl(const char* __s, size_t __refs) 
  : _M_refcount(__refs), _M_facets_size(_GLIBCXX_NUM_FACETS)
  {
    // Initialize the underlying locale model, which also checks to
    // see if the given name is valid.
    __c_locale __cloc;
    locale::facet::_S_create_c_locale(__cloc, __s);

    _M_facets = _M_caches = 0;
    _M_names = 0;
    try
      {
	_M_facets = new const facet*[_M_facets_size];
	for (size_t __i = 0; __i < _M_facets_size; ++__i)
	  _M_facets[__i] = 0;
	_M_caches = new const facet*[_M_facets_size];
	for (size_t __i = 0; __i < _M_facets_size; ++__i)
	  _M_caches[__i] = 0;
	_M_names = new char*[_S_categories_size];
	for (size_t __i = 0; __i < _S_categories_size; ++__i)
	  _M_names[__i] = 0;

	// Name all the categories.
	const size_t __len = std::strlen(__s);
	if (!std::strchr(__s, ';'))
	  {
	    for (size_t __i = 0; __i < _S_categories_size; ++__i)
	      {
		_M_names[__i] = new char[__len + 1];
		std::strcpy(_M_names[__i], __s);
	      }
	  }
	else
	  {
	    const char* __beg = __s;
	    for (size_t __i = 0; __i < _S_categories_size; ++__i)
	      {
		__beg = std::strchr(__beg, '=') + 1;
		const char* __end = std::strchr(__beg, ';');
		if (!__end)
		  __end = __s + __len;
		char* __new = new char[__end - __beg + 1];
		std::memcpy(__new, __beg, __end - __beg);
		__new[__end - __beg] = '\0';
		_M_names[__i] = __new;
	      }
	  }
 
	// Construct all standard facets and add them to _M_facets.
	_M_init_facet(new std::ctype<char>(__cloc, 0, false));
	_M_init_facet(new codecvt<char, char, mbstate_t>(__cloc));
	_M_init_facet(new numpunct<char>(__cloc));
	_M_init_facet(new num_get<char>);
	_M_init_facet(new num_put<char>);
	_M_init_facet(new std::collate<char>(__cloc));
	_M_init_facet(new moneypunct<char, false>(__cloc, __s));
	_M_init_facet(new moneypunct<char, true>(__cloc, __s));
	_M_init_facet(new money_get<char>);
	_M_init_facet(new money_put<char>);
	_M_init_facet(new __timepunct<char>(__cloc, __s));
	_M_init_facet(new time_get<char>);
	_M_init_facet(new time_put<char>);
	_M_init_facet(new std::messages<char>(__cloc, __s));
	
#ifdef  _GLIBCXX_USE_WCHAR_T
	_M_init_facet(new std::ctype<wchar_t>(__cloc));
	_M_init_facet(new codecvt<wchar_t, char, mbstate_t>(__cloc));
	_M_init_facet(new numpunct<wchar_t>(__cloc));
	_M_init_facet(new num_get<wchar_t>);
	_M_init_facet(new num_put<wchar_t>);
	_M_init_facet(new std::collate<wchar_t>(__cloc));
	_M_init_facet(new moneypunct<wchar_t, false>(__cloc, __s));
	_M_init_facet(new moneypunct<wchar_t, true>(__cloc, __s));
	_M_init_facet(new money_get<wchar_t>);
	_M_init_facet(new money_put<wchar_t>);
	_M_init_facet(new __timepunct<wchar_t>(__cloc, __s));
	_M_init_facet(new time_get<wchar_t>);
	_M_init_facet(new time_put<wchar_t>);
	_M_init_facet(new std::messages<wchar_t>(__cloc, __s));
#endif	  
	locale::facet::_S_destroy_c_locale(__cloc);
      }
    catch(...)
      {
	locale::facet::_S_destroy_c_locale(__cloc);
	this->~_Impl();
	__throw_exception_again;
      }	
  }

  void
  locale::_Impl::
  _M_replace_categories(const _Impl* __imp, category __cat)
  {
    for (size_t __ix = 0; __ix < _S_categories_size; ++__ix)
      {
	const category __mask = 1 << __ix;
	if (__mask & __cat)
	  {
	    // Need to replace entry in _M_facets with other locale's info.
	    _M_replace_category(__imp, _S_facet_categories[__ix]);
	    // If both have names, go ahead and mangle.
	    if (std::strcmp(_M_names[__ix], "*") != 0 
		&& std::strcmp(__imp->_M_names[__ix], "*") != 0)
	      {
		char* __new = new char[std::strlen(__imp->_M_names[__ix]) + 1];
		std::strcpy(__new, __imp->_M_names[__ix]);
		delete [] _M_names[__ix];
		_M_names[__ix] = __new;
	      }
	  }
      }
  }
} // namespace std
