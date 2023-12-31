// Locale support -*- C++ -*-

// Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004
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

// Warning: this file is not meant for user inclusion. Use <locale>.

#ifndef _LOCALE_FACETS_TCC
#define _LOCALE_FACETS_TCC 1

#pragma GCC system_header

#include <limits>		// For numeric_limits
#include <typeinfo>		// For bad_cast.
#include <bits/streambuf_iterator.h>

namespace std
{
  template<typename _Facet>
    locale
    locale::combine(const locale& __other) const
    {
      _Impl* __tmp = new _Impl(*_M_impl, 1);
      try
	{
	  __tmp->_M_replace_facet(__other._M_impl, &_Facet::id);
	}
      catch(...)
	{
	  __tmp->_M_remove_reference();
	  __throw_exception_again;
	}
      return locale(__tmp);
    }

  template<typename _CharT, typename _Traits, typename _Alloc>
    bool
    locale::operator()(const basic_string<_CharT, _Traits, _Alloc>& __s1,
                       const basic_string<_CharT, _Traits, _Alloc>& __s2) const
    {
      typedef std::collate<_CharT> __collate_type;
      const __collate_type& __collate = use_facet<__collate_type>(*this);
      return (__collate.compare(__s1.data(), __s1.data() + __s1.length(),
				__s2.data(), __s2.data() + __s2.length()) < 0);
    }

  /**
   *  @brief  Test for the presence of a facet.
   *
   *  has_facet tests the locale argument for the presence of the facet type
   *  provided as the template parameter.  Facets derived from the facet
   *  parameter will also return true.
   *
   *  @param  Facet  The facet type to test the presence of.
   *  @param  locale  The locale to test.
   *  @return  true if locale contains a facet of type Facet, else false.
  */
  template<typename _Facet>
    inline bool
    has_facet(const locale& __loc) throw()
    {
      const size_t __i = _Facet::id._M_id();
      const locale::facet** __facets = __loc._M_impl->_M_facets;
      return (__i < __loc._M_impl->_M_facets_size && __facets[__i]);
    }

  /**
   *  @brief  Return a facet.
   *
   *  use_facet looks for and returns a reference to a facet of type Facet
   *  where Facet is the template parameter.  If has_facet(locale) is true,
   *  there is a suitable facet to return.  It throws std::bad_cast if the
   *  locale doesn't contain a facet of type Facet.
   *
   *  @param  Facet  The facet type to access.
   *  @param  locale  The locale to use.
   *  @return  Reference to facet of type Facet.
   *  @throw  std::bad_cast if locale doesn't contain a facet of type Facet.
  */
  template<typename _Facet>
    inline const _Facet&
    use_facet(const locale& __loc)
    {
      const size_t __i = _Facet::id._M_id();
      const locale::facet** __facets = __loc._M_impl->_M_facets;
      if (!(__i < __loc._M_impl->_M_facets_size && __facets[__i]))
        __throw_bad_cast();
      return static_cast<const _Facet&>(*__facets[__i]);
    }

  // Routine to access a cache for the facet.  If the cache didn't
  // exist before, it gets constructed on the fly.
  template<typename _Facet>
    struct __use_cache
    {
      const _Facet*
      operator() (const locale& __loc) const;
    };

  // Specializations.
  template<typename _CharT>
    struct __use_cache<__numpunct_cache<_CharT> >
    {
      const __numpunct_cache<_CharT>*
      operator() (const locale& __loc) const
      {
	const size_t __i = numpunct<_CharT>::id._M_id();
	const locale::facet** __caches = __loc._M_impl->_M_caches;
	if (!__caches[__i])
	  {
	    __numpunct_cache<_CharT>* __tmp = NULL;
	    try
	      {
		__tmp = new __numpunct_cache<_CharT>;
		__tmp->_M_cache(__loc);
	      }
	    catch(...)
	      {
		delete __tmp;
		__throw_exception_again;
	      }
	    __loc._M_impl->_M_install_cache(__tmp, __i);
	  }
	return static_cast<const __numpunct_cache<_CharT>*>(__caches[__i]);
      }
    };

  template<typename _CharT, bool _Intl>
    struct __use_cache<__moneypunct_cache<_CharT, _Intl> >
    {
      const __moneypunct_cache<_CharT, _Intl>*
      operator() (const locale& __loc) const
      {
	const size_t __i = moneypunct<_CharT, _Intl>::id._M_id();
	const locale::facet** __caches = __loc._M_impl->_M_caches;
	if (!__caches[__i])
	  {
	    __moneypunct_cache<_CharT, _Intl>* __tmp = NULL;
	    try
	      {
		__tmp = new __moneypunct_cache<_CharT, _Intl>;
		__tmp->_M_cache(__loc);
	      }
	    catch(...)
	      {
		delete __tmp;
		__throw_exception_again;
	      }
	    __loc._M_impl->_M_install_cache(__tmp, __i);
	  }
	return static_cast<
	  const __moneypunct_cache<_CharT, _Intl>*>(__caches[__i]);
      }
    };

  template<typename _CharT>
    void
    __numpunct_cache<_CharT>::_M_cache(const locale& __loc)
    {
      _M_allocated = true;

      const numpunct<_CharT>& __np = use_facet<numpunct<_CharT> >(__loc);

      _M_grouping_size = __np.grouping().size();
      char* __grouping = new char[_M_grouping_size];
      __np.grouping().copy(__grouping, _M_grouping_size);
      _M_grouping = __grouping;
      _M_use_grouping = _M_grouping_size && __np.grouping()[0] != 0;

      _M_truename_size = __np.truename().size();
      _CharT* __truename = new _CharT[_M_truename_size];
      __np.truename().copy(__truename, _M_truename_size);
      _M_truename = __truename;

      _M_falsename_size = __np.falsename().size();
      _CharT* __falsename = new _CharT[_M_falsename_size];
      __np.falsename().copy(__falsename, _M_falsename_size);
      _M_falsename = __falsename;

      _M_decimal_point = __np.decimal_point();
      _M_thousands_sep = __np.thousands_sep();

      const ctype<_CharT>& __ct = use_facet<ctype<_CharT> >(__loc);
      __ct.widen(__num_base::_S_atoms_out,
		 __num_base::_S_atoms_out + __num_base::_S_oend, _M_atoms_out);
      __ct.widen(__num_base::_S_atoms_in,
		 __num_base::_S_atoms_in + __num_base::_S_iend, _M_atoms_in);
    }

  template<typename _CharT, bool _Intl>
    void
    __moneypunct_cache<_CharT, _Intl>::_M_cache(const locale& __loc)
    {
      _M_allocated = true;

      const moneypunct<_CharT, _Intl>& __mp =
	use_facet<moneypunct<_CharT, _Intl> >(__loc);

      _M_grouping_size = __mp.grouping().size();
      char* __grouping = new char[_M_grouping_size];
      __mp.grouping().copy(__grouping, _M_grouping_size);
      _M_grouping = __grouping;
      _M_use_grouping = _M_grouping_size && __mp.grouping()[0] != 0;
      
      _M_decimal_point = __mp.decimal_point();
      _M_thousands_sep = __mp.thousands_sep();
      _M_frac_digits = __mp.frac_digits();
      
      _M_curr_symbol_size = __mp.curr_symbol().size();
      _CharT* __curr_symbol = new _CharT[_M_curr_symbol_size];
      __mp.curr_symbol().copy(__curr_symbol, _M_curr_symbol_size);
      _M_curr_symbol = __curr_symbol;
      
      _M_positive_sign_size = __mp.positive_sign().size();
      _CharT* __positive_sign = new _CharT[_M_positive_sign_size];
      __mp.positive_sign().copy(__positive_sign, _M_positive_sign_size);
      _M_positive_sign = __positive_sign;

      _M_negative_sign_size = __mp.negative_sign().size();
      _CharT* __negative_sign = new _CharT[_M_negative_sign_size];
      __mp.negative_sign().copy(__negative_sign, _M_negative_sign_size);
      _M_negative_sign = __negative_sign;
      
      _M_pos_format = __mp.pos_format();
      _M_neg_format = __mp.neg_format();

      const ctype<_CharT>& __ct = use_facet<ctype<_CharT> >(__loc);
      __ct.widen(money_base::_S_atoms,
		 money_base::_S_atoms + money_base::_S_end, _M_atoms);
    }


  // Used by both numeric and monetary facets.
  // Check to make sure that the __grouping_tmp string constructed in
  // money_get or num_get matches the canonical grouping for a given
  // locale.
  // __grouping_tmp is parsed L to R
  // 1,222,444 == __grouping_tmp of "\1\3\3"
  // __grouping is parsed R to L
  // 1,222,444 == __grouping of "\3" == "\3\3\3"
  static bool
  __verify_grouping(const char* __grouping, size_t __grouping_size,
		    const string& __grouping_tmp);

  template<typename _CharT, typename _InIter>
    _InIter
    num_get<_CharT, _InIter>::
    _M_extract_float(_InIter __beg, _InIter __end, ios_base& __io,
		     ios_base::iostate& __err, string& __xtrc) const
    {
      typedef char_traits<_CharT>			__traits_type;
      typedef typename numpunct<_CharT>::__cache_type	__cache_type;
      __use_cache<__cache_type> __uc;
      const locale& __loc = __io._M_getloc();
      const __cache_type* __lc = __uc(__loc);
      const _CharT* __lit = __lc->_M_atoms_in;

      // True if a mantissa is found.
      bool __found_mantissa = false;

      // First check for sign.
      if (__beg != __end)
	{
	  const char_type __c = *__beg;
	  const bool __plus = __c == __lit[__num_base::_S_iplus];
	  if ((__plus || __c == __lit[__num_base::_S_iminus])
	      && !(__lc->_M_use_grouping && __c == __lc->_M_thousands_sep)
	      && !(__c == __lc->_M_decimal_point))
	    {
	      __xtrc += __plus ? '+' : '-';
	      ++__beg;
	    }
	}

      // Next, look for leading zeros.
      while (__beg != __end)
	{
	  const char_type __c = *__beg;
	  if (__lc->_M_use_grouping && __c == __lc->_M_thousands_sep
	      || __c == __lc->_M_decimal_point)
	    break;
	  else if (__c == __lit[__num_base::_S_izero])
	    {
	      if (!__found_mantissa)
		{
		  __xtrc += '0';
		  __found_mantissa = true;
		}
	      ++__beg;
	    }
	  else
	    break;
	}

      // Only need acceptable digits for floating point numbers.
      bool __found_dec = false;
      bool __found_sci = false;
      string __found_grouping;
      if (__lc->_M_use_grouping)
	__found_grouping.reserve(32);
      int __sep_pos = 0;
      const char_type* __lit_zero = __lit + __num_base::_S_izero;
      const char_type* __q;
      while (__beg != __end)
        {
	  // According to 22.2.2.1.2, p8-9, first look for thousands_sep
	  // and decimal_point.
	  const char_type __c = *__beg;
          if (__lc->_M_use_grouping && __c == __lc->_M_thousands_sep)
	    {
	      if (!__found_dec && !__found_sci)
		{
		  // NB: Thousands separator at the beginning of a string
		  // is a no-no, as is two consecutive thousands separators.
		  if (__sep_pos)
		    {
		      __found_grouping += static_cast<char>(__sep_pos);
		      __sep_pos = 0;
		      ++__beg;
		    }
		  else
		    {
		      __err |= ios_base::failbit;
		      break;
		    }
		}
	      else
		break;
            }
	  else if (__c == __lc->_M_decimal_point)
	    {
	      if (!__found_dec && !__found_sci)
		{
		  // If no grouping chars are seen, no grouping check
		  // is applied. Therefore __found_grouping is adjusted
		  // only if decimal_point comes after some thousands_sep.
		  if (__found_grouping.size())
		    __found_grouping += static_cast<char>(__sep_pos);
		  __xtrc += '.';
		  __found_dec = true;
		  ++__beg;
		}
	      else
		break;
	    }
          else if (__q = __traits_type::find(__lit_zero, 10, __c))
	    {
	      __xtrc += __num_base::_S_atoms_in[__q - __lit];
	      __found_mantissa = true;
	      ++__sep_pos;
	      ++__beg;
	    }
	  else if ((__c == __lit[__num_base::_S_ie] 
		    || __c == __lit[__num_base::_S_iE])
		   && __found_mantissa && !__found_sci)
	    {
	      // Scientific notation.
	      if (__found_grouping.size() && !__found_dec)
		__found_grouping += static_cast<char>(__sep_pos);
	      __xtrc += 'e';
	      __found_sci = true;

	      // Remove optional plus or minus sign, if they exist.
	      if (++__beg != __end)
		{
		  const bool __plus = *__beg == __lit[__num_base::_S_iplus];
		  if ((__plus || *__beg == __lit[__num_base::_S_iminus])
		      && !(__lc->_M_use_grouping
			   && *__beg == __lc->_M_thousands_sep)
		      && !(*__beg == __lc->_M_decimal_point))
		    {
		      __xtrc += __plus ? '+' : '-';
		      ++__beg;
		    }
		}
	    }
	  else
	    // Not a valid input item.
	    break;
        }

      // Digit grouping is checked. If grouping and found_grouping don't
      // match, then get very very upset, and set failbit.
      if (__lc->_M_use_grouping && __found_grouping.size())
        {
          // Add the ending grouping if a decimal or 'e'/'E' wasn't found.
	  if (!__found_dec && !__found_sci)
	    __found_grouping += static_cast<char>(__sep_pos);

          if (!std::__verify_grouping(__lc->_M_grouping, 
				      __lc->_M_grouping_size,
				      __found_grouping))
	    __err |= ios_base::failbit;
        }

      // Finish up.
      if (__beg == __end)
        __err |= ios_base::eofbit;
      return __beg;
    }

  template<typename _CharT, typename _InIter>
    template<typename _ValueT>
      _InIter
      num_get<_CharT, _InIter>::
      _M_extract_int(_InIter __beg, _InIter __end, ios_base& __io,
		     ios_base::iostate& __err, _ValueT& __v) const
      {
        typedef char_traits<_CharT>			__traits_type;
	typedef typename numpunct<_CharT>::__cache_type __cache_type;
	__use_cache<__cache_type> __uc;
	const locale& __loc = __io._M_getloc();
	const __cache_type* __lc = __uc(__loc);
	const _CharT* __lit = __lc->_M_atoms_in;

	// NB: Iff __basefield == 0, __base can change based on contents.
	const ios_base::fmtflags __basefield = __io.flags()
	                                       & ios_base::basefield;
	const bool __oct = __basefield == ios_base::oct;
	int __base = __oct ? 8 : (__basefield == ios_base::hex ? 16 : 10);

	// True if numeric digits are found.
	bool __found_num = false;

	// First check for sign.
	bool __negative = false;
	if (__beg != __end)
	  {
	    const char_type __c = *__beg;
	    if (numeric_limits<_ValueT>::is_signed)
	      __negative = __c == __lit[__num_base::_S_iminus];
	    if ((__negative || __c == __lit[__num_base::_S_iplus])
		&& !(__lc->_M_use_grouping && __c == __lc->_M_thousands_sep)
		&& !(__c == __lc->_M_decimal_point))
	      ++__beg;
	  }

	// Next, look for leading zeros and check required digits
	// for base formats.
	while (__beg != __end)
	  {
	    const char_type __c = *__beg;
	    if (__lc->_M_use_grouping && __c == __lc->_M_thousands_sep
		|| __c == __lc->_M_decimal_point)
	      break;
	    else if (__c == __lit[__num_base::_S_izero] 
		     && (!__found_num || __base == 10))
	      {
		__found_num = true;
		++__beg;
	      }
	    else if (__found_num)
	      {
		if (__c == __lit[__num_base::_S_ix] 
		    || __c == __lit[__num_base::_S_iX])
		  {
		    if (__basefield == 0)
		      __base = 16;
		    if (__base == 16)
		      {
			__found_num = false;
			++__beg;
		      }
		  }
		else if (__basefield == 0)
		  __base = 8;
		break;
	      }
	    else
	      break;
	  }

	// At this point, base is determined. If not hex, only allow
	// base digits as valid input.
	const size_t __len = __base == 16 ? __num_base::_S_iend - __num_base::_S_izero : __base;

	// Extract.
	string __found_grouping;
	if (__lc->_M_use_grouping)
	  __found_grouping.reserve(32);
	int __sep_pos = 0;
	bool __overflow = false;
	_ValueT __result = 0;
	const char_type* __lit_zero = __lit + __num_base::_S_izero;
	const char_type* __q;
	if (__negative)
	  {
	    const _ValueT __min = numeric_limits<_ValueT>::min() / __base;
	    for (; __beg != __end; ++__beg)
	      {
		// According to 22.2.2.1.2, p8-9, first look for thousands_sep
		// and decimal_point.
		const char_type __c = *__beg;
		if (__lc->_M_use_grouping && __c == __lc->_M_thousands_sep)
		  {
		    // NB: Thousands separator at the beginning of a string
		    // is a no-no, as is two consecutive thousands separators.
		    if (__sep_pos)
		      {
			__found_grouping += static_cast<char>(__sep_pos);
			__sep_pos = 0;
		      }
		    else
		      {
			__err |= ios_base::failbit;
			break;
		      }
		  }
		else if (__c == __lc->_M_decimal_point)
		  break;
		else if (__q = __traits_type::find(__lit_zero, __len, __c))
		  {
		    int __digit = __q - __lit_zero;
		    if (__digit > 15)
		      __digit -= 6;
		    if (__result < __min)
		      __overflow = true;
		    else
		      {
			const _ValueT __new_result = __result * __base
			                             - __digit;
			__overflow |= __new_result > __result;
			__result = __new_result;
			++__sep_pos;
			__found_num = true;
		      }
		  }
		else
		  // Not a valid input item.
		  break;
	      }
	  }
	else
	  {
	    const _ValueT __max = numeric_limits<_ValueT>::max() / __base;
	    for (; __beg != __end; ++__beg)
	      {
		const char_type __c = *__beg;
		if (__lc->_M_use_grouping && __c == __lc->_M_thousands_sep)
		  {
		    if (__sep_pos)
		      {
			__found_grouping += static_cast<char>(__sep_pos);
			__sep_pos = 0;
		      }
		    else
		      {
			__err |= ios_base::failbit;
			break;
		      }
		  }
		else if (__c == __lc->_M_decimal_point)
		  break;
		else if (__q = __traits_type::find(__lit_zero, __len, __c))
		  {
		    int __digit = __q - __lit_zero;
		    if (__digit > 15)
		      __digit -= 6;
		    if (__result > __max)
		      __overflow = true;
		    else
		      {
			const _ValueT __new_result = __result * __base
			                             + __digit;
			__overflow |= __new_result < __result;
			__result = __new_result;
			++__sep_pos;
			__found_num = true;
		      }
		  }
		else
		  break;
	      }
	  }

	// Digit grouping is checked. If grouping and found_grouping don't
	// match, then get very very upset, and set failbit.
	if (__lc->_M_use_grouping && __found_grouping.size())
	  {
	    // Add the ending grouping.
	    __found_grouping += static_cast<char>(__sep_pos);

	    if (!std::__verify_grouping(__lc->_M_grouping,
					__lc->_M_grouping_size,
					__found_grouping))
	      __err |= ios_base::failbit;
	  }

	if (!(__err & ios_base::failbit) && !__overflow
	    && __found_num)
	  __v = __result;
	else
	  __err |= ios_base::failbit;

	if (__beg == __end)
	  __err |= ios_base::eofbit;
	return __beg;
      }

  // _GLIBCXX_RESOLVE_LIB_DEFECTS
  // 17.  Bad bool parsing
  template<typename _CharT, typename _InIter>
    _InIter
    num_get<_CharT, _InIter>::
    do_get(iter_type __beg, iter_type __end, ios_base& __io,
           ios_base::iostate& __err, bool& __v) const
    {
      if (!(__io.flags() & ios_base::boolalpha))
        {
	  // Parse bool values as long.
          // NB: We can't just call do_get(long) here, as it might
          // refer to a derived class.
	  long __l = -1;
          __beg = _M_extract_int(__beg, __end, __io, __err, __l);
	  if (__l == 0 || __l == 1)
	    __v = __l;
	  else
            __err |= ios_base::failbit;
        }
      else
        {
	  // Parse bool values as alphanumeric.
	  typedef char_traits<_CharT>                     __traits_type;
	  typedef typename numpunct<_CharT>::__cache_type __cache_type;
	  __use_cache<__cache_type> __uc;
	  const locale& __loc = __io._M_getloc();
	  const __cache_type* __lc = __uc(__loc);

	  bool __testf = true;
	  bool __testt = true;
	  size_t __n;
          for (__n = 0; __beg != __end; ++__n, ++__beg)
            {
	      if (__testf)
		if (__n < __lc->_M_falsename_size)
		  __testf = *__beg == __lc->_M_falsename[__n];
		else
		  break;

	      if (__testt)
		if (__n < __lc->_M_truename_size)
		  __testt = *__beg == __lc->_M_truename[__n];
		else
		  break;

	      if (!__testf && !__testt)
		break;
            }
	  if (__testf && __n == __lc->_M_falsename_size)
	    __v = 0;
	  else if (__testt && __n == __lc->_M_truename_size)
	    __v = 1;
	  else
	    __err |= ios_base::failbit;

          if (__beg == __end)
            __err |= ios_base::eofbit;
        }
      return __beg;
    }

  template<typename _CharT, typename _InIter>
    _InIter
    num_get<_CharT, _InIter>::
    do_get(iter_type __beg, iter_type __end, ios_base& __io,
           ios_base::iostate& __err, long& __v) const
    { return _M_extract_int(__beg, __end, __io, __err, __v); }

  template<typename _CharT, typename _InIter>
    _InIter
    num_get<_CharT, _InIter>::
    do_get(iter_type __beg, iter_type __end, ios_base& __io,
           ios_base::iostate& __err, unsigned short& __v) const
    { return _M_extract_int(__beg, __end, __io, __err, __v); }

  template<typename _CharT, typename _InIter>
    _InIter
    num_get<_CharT, _InIter>::
    do_get(iter_type __beg, iter_type __end, ios_base& __io,
           ios_base::iostate& __err, unsigned int& __v) const
    { return _M_extract_int(__beg, __end, __io, __err, __v); }

  template<typename _CharT, typename _InIter>
    _InIter
    num_get<_CharT, _InIter>::
    do_get(iter_type __beg, iter_type __end, ios_base& __io,
           ios_base::iostate& __err, unsigned long& __v) const
    { return _M_extract_int(__beg, __end, __io, __err, __v); }

#ifdef _GLIBCXX_USE_LONG_LONG
  template<typename _CharT, typename _InIter>
    _InIter
    num_get<_CharT, _InIter>::
    do_get(iter_type __beg, iter_type __end, ios_base& __io,
           ios_base::iostate& __err, long long& __v) const
    { return _M_extract_int(__beg, __end, __io, __err, __v); }

  template<typename _CharT, typename _InIter>
    _InIter
    num_get<_CharT, _InIter>::
    do_get(iter_type __beg, iter_type __end, ios_base& __io,
           ios_base::iostate& __err, unsigned long long& __v) const
    { return _M_extract_int(__beg, __end, __io, __err, __v); }
#endif

  template<typename _CharT, typename _InIter>
    _InIter
    num_get<_CharT, _InIter>::
    do_get(iter_type __beg, iter_type __end, ios_base& __io,
	   ios_base::iostate& __err, float& __v) const
    {
      string __xtrc;
      __xtrc.reserve(32);
      __beg = _M_extract_float(__beg, __end, __io, __err, __xtrc);
      std::__convert_to_v(__xtrc.c_str(), __v, __err, _S_get_c_locale());
      return __beg;
    }

  template<typename _CharT, typename _InIter>
    _InIter
    num_get<_CharT, _InIter>::
    do_get(iter_type __beg, iter_type __end, ios_base& __io,
           ios_base::iostate& __err, double& __v) const
    {
      string __xtrc;
      __xtrc.reserve(32);
      __beg = _M_extract_float(__beg, __end, __io, __err, __xtrc);
      std::__convert_to_v(__xtrc.c_str(), __v, __err, _S_get_c_locale());
      return __beg;
    }

  template<typename _CharT, typename _InIter>
    _InIter
    num_get<_CharT, _InIter>::
    do_get(iter_type __beg, iter_type __end, ios_base& __io,
           ios_base::iostate& __err, long double& __v) const
    {
      string __xtrc;
      __xtrc.reserve(32);
      __beg = _M_extract_float(__beg, __end, __io, __err, __xtrc);
      std::__convert_to_v(__xtrc.c_str(), __v, __err, _S_get_c_locale());
      return __beg;
    }

  template<typename _CharT, typename _InIter>
    _InIter
    num_get<_CharT, _InIter>::
    do_get(iter_type __beg, iter_type __end, ios_base& __io,
           ios_base::iostate& __err, void*& __v) const
    {
      // Prepare for hex formatted input.
      typedef ios_base::fmtflags        fmtflags;
      const fmtflags __fmt = __io.flags();
      __io.flags(__fmt & ~ios_base::basefield | ios_base::hex);

      unsigned long __ul;
      __beg = _M_extract_int(__beg, __end, __io, __err, __ul);

      // Reset from hex formatted input.
      __io.flags(__fmt);

      if (!(__err & ios_base::failbit))
	__v = reinterpret_cast<void*>(__ul);
      else
	__err |= ios_base::failbit;
      return __beg;
    }

  // For use by integer and floating-point types after they have been
  // converted into a char_type string.
  template<typename _CharT, typename _OutIter>
    void
    num_put<_CharT, _OutIter>::
    _M_pad(_CharT __fill, streamsize __w, ios_base& __io,
	   _CharT* __new, const _CharT* __cs, int& __len) const
    {
      // [22.2.2.2.2] Stage 3.
      // If necessary, pad.
      __pad<_CharT, char_traits<_CharT> >::_S_pad(__io, __fill, __new, __cs,
						  __w, __len, true);
      __len = static_cast<int>(__w);
    }

  // Forwarding functions to peel signed from unsigned integer types.
  template<typename _CharT>
    inline int
    __int_to_char(_CharT* __bufend, long __v, const _CharT* __lit,
		  ios_base::fmtflags __flags)
    {
      unsigned long __ul = static_cast<unsigned long>(__v);
      bool __neg = false;
      if (__v < 0)
	{
	  __ul = -__ul;
	  __neg = true;
	}
      return __int_to_char(__bufend, __ul, __lit, __flags, __neg);
    }

  template<typename _CharT>
    inline int
    __int_to_char(_CharT* __bufend, unsigned long __v, const _CharT* __lit,
		  ios_base::fmtflags __flags)
    { return __int_to_char(__bufend, __v, __lit, __flags, false); }

#ifdef _GLIBCXX_USE_LONG_LONG
  template<typename _CharT>
    inline int
    __int_to_char(_CharT* __bufend, long long __v, const _CharT* __lit,
		  ios_base::fmtflags __flags)
    {
      unsigned long long __ull = static_cast<unsigned long long>(__v);
      bool __neg = false;
      if (__v < 0)
	{
	  __ull = -__ull;
	  __neg = true;
	}
      return __int_to_char(__bufend, __ull, __lit, __flags, __neg);
    }

  template<typename _CharT>
    inline int
    __int_to_char(_CharT* __bufend, unsigned long long __v, 
		  const _CharT* __lit, ios_base::fmtflags __flags)
    { return __int_to_char(__bufend, __v, __lit, __flags, false); }
#endif

  template<typename _CharT, typename _ValueT>
    int
    __int_to_char(_CharT* __bufend, _ValueT __v, const _CharT* __lit,
		  ios_base::fmtflags __flags, bool __neg)
    {
      // Don't write base if already 0.
      const bool __showbase = (__flags & ios_base::showbase) && __v;
      const ios_base::fmtflags __basefield = __flags & ios_base::basefield;
      _CharT* __buf = __bufend - 1;

      if (__builtin_expect(__basefield != ios_base::oct &&
			   __basefield != ios_base::hex, true))
	{
	  // Decimal.
	  do
	    {
	      *__buf-- = __lit[(__v % 10) + __num_base::_S_odigits];
	      __v /= 10;
	    }
	  while (__v != 0);
	  if (__neg)
	    *__buf-- = __lit[__num_base::_S_ominus];
	  else if (__flags & ios_base::showpos)
	    *__buf-- = __lit[__num_base::_S_oplus];
	}
      else if (__basefield == ios_base::oct)
	{
	  // Octal.
	  do
	    {
	      *__buf-- = __lit[(__v & 0x7) + __num_base::_S_odigits];
	      __v >>= 3;
	    }
	  while (__v != 0);
	  if (__showbase)
	    *__buf-- = __lit[__num_base::_S_odigits];
	}
      else
	{
	  // Hex.
	  const bool __uppercase = __flags & ios_base::uppercase;
	  const int __case_offset = __uppercase ? __num_base::_S_oudigits
	                                        : __num_base::_S_odigits;
	  do
	    {
	      *__buf-- = __lit[(__v & 0xf) + __case_offset];
	      __v >>= 4;
	    }
	  while (__v != 0);
	  if (__showbase)
	    {
	      // 'x' or 'X'
	      *__buf-- = __lit[__num_base::_S_ox + __uppercase];
	      // '0'
	      *__buf-- = __lit[__num_base::_S_odigits];
	    }
	}
      return __bufend - __buf - 1;
    }

  template<typename _CharT, typename _OutIter>
    void
    num_put<_CharT, _OutIter>::
    _M_group_int(const char* __grouping, size_t __grouping_size, _CharT __sep,
		 ios_base& __io, _CharT* __new, _CharT* __cs, int& __len) const
    {
      // By itself __add_grouping cannot deal correctly with __cs when
      // ios::showbase is set and ios_base::oct || ios_base::hex.
      // Therefore we take care "by hand" of the initial 0, 0x or 0X.
      // However, remember that the latter do not occur if the number
      // printed is '0' (__len == 1).
      streamsize __off = 0;
      const ios_base::fmtflags __basefield = __io.flags()
	                                     & ios_base::basefield;
      if ((__io.flags() & ios_base::showbase) && __len > 1)
	if (__basefield == ios_base::oct)
	  {
	    __off = 1;
	    __new[0] = __cs[0];
	  }
	else if (__basefield == ios_base::hex)
	  {
	    __off = 2;
	    __new[0] = __cs[0];
	    __new[1] = __cs[1];
	  }
      _CharT* __p;
      __p = std::__add_grouping(__new + __off, __sep, __grouping,
				__grouping_size, __cs + __off,
				__cs + __len);
      __len = __p - __new;
    }

  template<typename _CharT, typename _OutIter>
    template<typename _ValueT>
      _OutIter
      num_put<_CharT, _OutIter>::
      _M_insert_int(_OutIter __s, ios_base& __io, _CharT __fill,
		    _ValueT __v) const
      {
	typedef typename numpunct<_CharT>::__cache_type	__cache_type;
	__use_cache<__cache_type> __uc;
	const locale& __loc = __io._M_getloc();
	const __cache_type* __lc = __uc(__loc);
	const _CharT* __lit = __lc->_M_atoms_out;

	// Long enough to hold hex, dec, and octal representations.
	const int __ilen = 4 * sizeof(_ValueT);
	_CharT* __cs = static_cast<_CharT*>(__builtin_alloca(sizeof(_CharT)
							     * __ilen));

	// [22.2.2.2.2] Stage 1, numeric conversion to character.
	// Result is returned right-justified in the buffer.
	int __len;
	__len = __int_to_char(__cs + __ilen, __v, __lit, __io.flags());
	__cs += __ilen - __len;

	// Add grouping, if necessary.
	if (__lc->_M_use_grouping)
	  {
	    // Grouping can add (almost) as many separators as the
	    // number of digits, but no more.
	    _CharT* __cs2 = static_cast<_CharT*>(__builtin_alloca(sizeof(_CharT)
								  * __len * 2));
	    _M_group_int(__lc->_M_grouping, __lc->_M_grouping_size,
			 __lc->_M_thousands_sep, __io, __cs2, __cs, __len);
	    __cs = __cs2;
	  }

	// Pad.
	const streamsize __w = __io.width();
	if (__w > static_cast<streamsize>(__len))
	  {
	    _CharT* __cs3 = static_cast<_CharT*>(__builtin_alloca(sizeof(_CharT)
								  * __w));
	    _M_pad(__fill, __w, __io, __cs3, __cs, __len);
	    __cs = __cs3;
	  }
	__io.width(0);

	// [22.2.2.2.2] Stage 4.
	// Write resulting, fully-formatted string to output iterator.
	return std::__write(__s, __cs, __len);
      }

  template<typename _CharT, typename _OutIter>
    void
    num_put<_CharT, _OutIter>::
    _M_group_float(const char* __grouping, size_t __grouping_size,
		   _CharT __sep, const _CharT* __p, _CharT* __new,
		   _CharT* __cs, int& __len) const
    {
      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 282. What types does numpunct grouping refer to?
      // Add grouping, if necessary.
      _CharT* __p2;
      const int __declen = __p ? __p - __cs : __len;
      __p2 = std::__add_grouping(__new, __sep, __grouping, __grouping_size,
				 __cs, __cs + __declen);

      // Tack on decimal part.
      int __newlen = __p2 - __new;
      if (__p)
	{
	  char_traits<_CharT>::copy(__p2, __p, __len - __declen);
	  __newlen += __len - __declen;
	}
      __len = __newlen;
    }

  // The following code uses snprintf (or sprintf(), when
  // _GLIBCXX_USE_C99 is not defined) to convert floating point values
  // for insertion into a stream.  An optimization would be to replace
  // them with code that works directly on a wide buffer and then use
  // __pad to do the padding.  It would be good to replace them anyway
  // to gain back the efficiency that C++ provides by knowing up front
  // the type of the values to insert.  Also, sprintf is dangerous
  // since may lead to accidental buffer overruns.  This
  // implementation follows the C++ standard fairly directly as
  // outlined in 22.2.2.2 [lib.locale.num.put]
  template<typename _CharT, typename _OutIter>
    template<typename _ValueT>
      _OutIter
      num_put<_CharT, _OutIter>::
      _M_insert_float(_OutIter __s, ios_base& __io, _CharT __fill, char __mod,
		       _ValueT __v) const
      {
	typedef typename numpunct<_CharT>::__cache_type	__cache_type;
	__use_cache<__cache_type> __uc;
	const locale& __loc = __io._M_getloc();
	const __cache_type* __lc = __uc(__loc);

	// Note: digits10 is rounded down: add 1 to ensure the maximum
	// available precision.  Then, in general, one more 1 needs to
	// be added since, when the %{g,G} conversion specifiers are
	// chosen inside _S_format_float, the precision field is "the
	// maximum number of significant digits", *not* the "number of
	// digits to appear after the decimal point", as happens for
	// %{e,E,f,F} (C99, 7.19.6.1,4).
	const int __max_digits = numeric_limits<_ValueT>::digits10 + 2;

	// Use default precision if out of range.
	streamsize __prec = __io.precision();
	if (__prec > static_cast<streamsize>(__max_digits))
	  __prec = static_cast<streamsize>(__max_digits);
	else if (__prec < static_cast<streamsize>(0))
	  __prec = static_cast<streamsize>(6);

	// [22.2.2.2.2] Stage 1, numeric conversion to character.
	int __len;
	// Long enough for the max format spec.
	char __fbuf[16];

#ifdef _GLIBCXX_USE_C99
	// First try a buffer perhaps big enough (for sure sufficient
	// for non-ios_base::fixed outputs)
	int __cs_size = __max_digits * 3;
	char* __cs = static_cast<char*>(__builtin_alloca(__cs_size));

	__num_base::_S_format_float(__io, __fbuf, __mod);
	__len = std::__convert_from_v(__cs, __cs_size, __fbuf, __v,
				      _S_get_c_locale(), __prec);

	// If the buffer was not large enough, try again with the correct size.
	if (__len >= __cs_size)
	  {
	    __cs_size = __len + 1;
	    __cs = static_cast<char*>(__builtin_alloca(__cs_size));
	    __len = std::__convert_from_v(__cs, __cs_size, __fbuf, __v,
					  _S_get_c_locale(), __prec);
	  }
#else
	// Consider the possibility of long ios_base::fixed outputs
	const bool __fixed = __io.flags() & ios_base::fixed;
	const int __max_exp = numeric_limits<_ValueT>::max_exponent10;

	// The size of the output string is computed as follows.
	// ios_base::fixed outputs may need up to __max_exp+1 chars
	// for the integer part + up to __max_digits chars for the
	// fractional part + 3 chars for sign, decimal point, '\0'. On
	// the other hand, for non-fixed outputs __max_digits*3 chars
	// are largely sufficient.
	const int __cs_size = __fixed ? __max_exp + __max_digits + 4
	                              : __max_digits * 3;
	char* __cs = static_cast<char*>(__builtin_alloca(__cs_size));

	__num_base::_S_format_float(__io, __fbuf, __mod);
	__len = std::__convert_from_v(__cs, 0, __fbuf, __v,
				      _S_get_c_locale(), __prec);
#endif

      // [22.2.2.2.2] Stage 2, convert to char_type, using correct
      // numpunct.decimal_point() values for '.' and adding grouping.
      const ctype<_CharT>& __ctype = use_facet<ctype<_CharT> >(__loc);

      _CharT* __ws = static_cast<_CharT*>(__builtin_alloca(sizeof(_CharT)
							   * __len));
      __ctype.widen(__cs, __cs + __len, __ws);

      // Replace decimal point.
      const _CharT __cdec = __ctype.widen('.');
      const _CharT __dec = __lc->_M_decimal_point;
      const _CharT* __p;
      if (__p = char_traits<_CharT>::find(__ws, __len, __cdec))
	__ws[__p - __ws] = __dec;

      // Add grouping, if necessary.
      if (__lc->_M_use_grouping)
	{
	  // Grouping can add (almost) as many separators as the
	  // number of digits, but no more.
	  _CharT* __ws2 = static_cast<_CharT*>(__builtin_alloca(sizeof(_CharT)
								* __len * 2));
	  _M_group_float(__lc->_M_grouping, __lc->_M_grouping_size,
			 __lc->_M_thousands_sep, __p, __ws2, __ws, __len);
	  __ws = __ws2;
	}

      // Pad.
      const streamsize __w = __io.width();
      if (__w > static_cast<streamsize>(__len))
	{
	  _CharT* __ws3 = static_cast<_CharT*>(__builtin_alloca(sizeof(_CharT)
								* __w));
	  _M_pad(__fill, __w, __io, __ws3, __ws, __len);
	  __ws = __ws3;
	}
      __io.width(0);

      // [22.2.2.2.2] Stage 4.
      // Write resulting, fully-formatted string to output iterator.
      return std::__write(__s, __ws, __len);
      }

  template<typename _CharT, typename _OutIter>
    _OutIter
    num_put<_CharT, _OutIter>::
    do_put(iter_type __s, ios_base& __io, char_type __fill, bool __v) const
    {
      const ios_base::fmtflags __flags = __io.flags();
      if ((__flags & ios_base::boolalpha) == 0)
        {
          unsigned long __uv = __v;
          __s = _M_insert_int(__s, __io, __fill, __uv);
        }
      else
        {
	  typedef typename numpunct<_CharT>::__cache_type __cache_type;
	  __use_cache<__cache_type> __uc;
	  const locale& __loc = __io._M_getloc();
	  const __cache_type* __lc = __uc(__loc);

	  const _CharT* __name = __v ? __lc->_M_truename
	                             : __lc->_M_falsename;
	  int __len = __v ? __lc->_M_truename_size
	                  : __lc->_M_falsename_size;

	  const streamsize __w = __io.width();
	  if (__w > static_cast<streamsize>(__len))
	    {
	      _CharT* __cs
		= static_cast<_CharT*>(__builtin_alloca(sizeof(_CharT)
							* __w));
	      _M_pad(__fill, __w, __io, __cs, __name, __len);
	      __name = __cs;
	    }
	  __io.width(0);
	  __s = std::__write(__s, __name, __len);
	}
      return __s;
    }

  template<typename _CharT, typename _OutIter>
    _OutIter
    num_put<_CharT, _OutIter>::
    do_put(iter_type __s, ios_base& __io, char_type __fill, long __v) const
    { return _M_insert_int(__s, __io, __fill, __v); }

  template<typename _CharT, typename _OutIter>
    _OutIter
    num_put<_CharT, _OutIter>::
    do_put(iter_type __s, ios_base& __io, char_type __fill,
           unsigned long __v) const
    { return _M_insert_int(__s, __io, __fill, __v); }

#ifdef _GLIBCXX_USE_LONG_LONG
  template<typename _CharT, typename _OutIter>
    _OutIter
    num_put<_CharT, _OutIter>::
    do_put(iter_type __s, ios_base& __b, char_type __fill, long long __v) const
    { return _M_insert_int(__s, __b, __fill, __v); }

  template<typename _CharT, typename _OutIter>
    _OutIter
    num_put<_CharT, _OutIter>::
    do_put(iter_type __s, ios_base& __io, char_type __fill,
           unsigned long long __v) const
    { return _M_insert_int(__s, __io, __fill, __v); }
#endif

  template<typename _CharT, typename _OutIter>
    _OutIter
    num_put<_CharT, _OutIter>::
    do_put(iter_type __s, ios_base& __io, char_type __fill, double __v) const
    { return _M_insert_float(__s, __io, __fill, char(), __v); }

  template<typename _CharT, typename _OutIter>
    _OutIter
    num_put<_CharT, _OutIter>::
    do_put(iter_type __s, ios_base& __io, char_type __fill,
	   long double __v) const
    { return _M_insert_float(__s, __io, __fill, 'L', __v); }

  template<typename _CharT, typename _OutIter>
    _OutIter
    num_put<_CharT, _OutIter>::
    do_put(iter_type __s, ios_base& __io, char_type __fill,
           const void* __v) const
    {
      const ios_base::fmtflags __flags = __io.flags();
      const ios_base::fmtflags __fmt = ~(ios_base::showpos
					 | ios_base::basefield
					 | ios_base::uppercase
					 | ios_base::internal);
      __io.flags(__flags & __fmt | (ios_base::hex | ios_base::showbase));

      __s = _M_insert_int(__s, __io, __fill,
			  reinterpret_cast<unsigned long>(__v));
      __io.flags(__flags);
      return __s;
    }

  template<typename _CharT, typename _InIter>
    template<bool _Intl>
      _InIter
      money_get<_CharT, _InIter>::
      _M_extract(iter_type __beg, iter_type __end, ios_base& __io,
		 ios_base::iostate& __err, string& __units) const
      {
	typedef char_traits<_CharT>			  __traits_type;
	typedef typename string_type::size_type	          size_type;	
	typedef money_base::part			  part;
	typedef moneypunct<_CharT, _Intl>		  __moneypunct_type;
	typedef typename __moneypunct_type::__cache_type  __cache_type;
	
	const locale& __loc = __io._M_getloc();
	const ctype<_CharT>& __ctype = use_facet<ctype<_CharT> >(__loc);

	__use_cache<__cache_type> __uc;
	const __cache_type* __lc = __uc(__loc);
	const char_type* __lit = __lc->_M_atoms;

	// Deduced sign.
	bool __negative = false;
	// Sign size.
	size_type __sign_size = 0;
	// True if sign is mandatory.
	const bool __mandatory_sign = (__lc->_M_positive_sign_size
				       && __lc->_M_negative_sign_size);
	// String of grouping info from thousands_sep plucked from __units.
	string __grouping_tmp;
	if (__lc->_M_use_grouping)
	  __grouping_tmp.reserve(32);
	// Last position before the decimal point.
	int __last_pos = 0;
	// Separator positions, then, possibly, fractional digits.
	int __n = 0;
	// If input iterator is in a valid state.
	bool __testvalid = true;
	// Flag marking when a decimal point is found.
	bool __testdecfound = false;

	// The tentative returned string is stored here.
	string __res;
	__res.reserve(32);

	const char_type* __lit_zero = __lit + money_base::_S_zero;
	const char_type* __q;
	const money_base::pattern __p = __lc->_M_neg_format;	
	for (int __i = 0; __i < 4 && __testvalid; ++__i)
	  {
	    const part __which = static_cast<part>(__p.field[__i]);
	    switch (__which)
	      {
	      case money_base::symbol:
		// According to 22.2.6.1.2, p2, symbol is required
		// if (__io.flags() & ios_base::showbase), otherwise
		// is optional and consumed only if other characters
		// are needed to complete the format.
		if (__io.flags() & ios_base::showbase || __sign_size > 1
		    || __i == 0
		    || (__i == 1 && (__mandatory_sign
				     || (static_cast<part>(__p.field[0])
					 == money_base::sign)
				     || (static_cast<part>(__p.field[2])
					 == money_base::space)))
		    || (__i == 2 && ((static_cast<part>(__p.field[3])
				      == money_base::value)
				     || __mandatory_sign
				     && (static_cast<part>(__p.field[3])
					 == money_base::sign))))
		  {
		    const size_type __len = __lc->_M_curr_symbol_size;
		    size_type __j = 0;
		    for (; __beg != __end && __j < __len
			   && *__beg == __lc->_M_curr_symbol[__j];
			 ++__beg, ++__j);
		    if (__j != __len
			&& (__j || __io.flags() & ios_base::showbase))
		      __testvalid = false;
		  }
		break;
	      case money_base::sign:
		// Sign might not exist, or be more than one character long.
		if (__lc->_M_positive_sign_size && __beg != __end
		    && *__beg == __lc->_M_positive_sign[0])
		  {
		    __sign_size = __lc->_M_positive_sign_size;
		    ++__beg;
		  }
		else if (__lc->_M_negative_sign_size && __beg != __end
			 && *__beg == __lc->_M_negative_sign[0])
		  {
		    __negative = true;
		    __sign_size = __lc->_M_negative_sign_size;
		    ++__beg;
		  }
		else if (__lc->_M_positive_sign_size
			 && !__lc->_M_negative_sign_size)
		  // "... if no sign is detected, the result is given the sign
		  // that corresponds to the source of the empty string"
		  __negative = true;
		else if (__mandatory_sign)
		  __testvalid = false;
		break;
	      case money_base::value:
		// Extract digits, remove and stash away the
		// grouping of found thousands separators.
		for (; __beg != __end; ++__beg)
		  if (__q = __traits_type::find(__lit_zero, 10, *__beg))
		    {
		      __res += money_base::_S_atoms[__q - __lit];
		      ++__n;
		    }
		  else if (*__beg == __lc->_M_decimal_point && !__testdecfound)
		    {
		      __last_pos = __n;
		      __n = 0;
		      __testdecfound = true;
		    }
		  else if (__lc->_M_use_grouping
			   && *__beg == __lc->_M_thousands_sep
			   && !__testdecfound)
		    {
		      if (__n)
			{
			  // Mark position for later analysis.
			  __grouping_tmp += static_cast<char>(__n);
			  __n = 0;
			}
		      else
			{
			  __testvalid = false;
			  break;
			}
		    }
		  else
		    break;
		if (__res.empty())
		  __testvalid = false;
		break;
	      case money_base::space:
		// At least one space is required.
		if (__beg != __end && __ctype.is(ctype_base::space, *__beg))
		  ++__beg;
		else
		  __testvalid = false;
	      case money_base::none:
		// Only if not at the end of the pattern.
		if (__i != 3)
		  for (; __beg != __end
			 && __ctype.is(ctype_base::space, *__beg); ++__beg);
		break;
	      }
	  }

	// Need to get the rest of the sign characters, if they exist.
	if (__sign_size > 1 && __testvalid)
	  {
	    const char_type* __sign = __negative ? __lc->_M_negative_sign
	                                         : __lc->_M_positive_sign;
	    size_type __i = 1;
	    for (; __beg != __end && __i < __sign_size
		   && *__beg == __sign[__i]; ++__beg, ++__i);
	    
	    if (__i != __sign_size)
	      __testvalid = false;
	  }

	if (__testvalid)
	  {
	    // Strip leading zeros.
	    if (__res.size() > 1)
	      {
		const size_type __first = __res.find_first_not_of('0');
		const bool __only_zeros = __first == string::npos;
		if (__first)
		  __res.erase(0, __only_zeros ? __res.size() - 1 : __first);
	      }

	    // 22.2.6.1.2, p4
	    if (__negative && __res[0] != '0')
	      __res.insert(__res.begin(), '-');
	    
	    // Test for grouping fidelity.
	    if (__grouping_tmp.size())
	      {
		// Add the ending grouping.
		__grouping_tmp += static_cast<char>(__testdecfound ? __last_pos
						                   : __n);
		if (!std::__verify_grouping(__lc->_M_grouping,
					    __lc->_M_grouping_size,
					    __grouping_tmp))
		  __testvalid = false;
	      }
	    
	    // Iff not enough digits were supplied after the decimal-point.
	    if (__testdecfound && __lc->_M_frac_digits > 0
		&& __n != __lc->_M_frac_digits)
	      __testvalid = false;
	  }
	
	// Iff no more characters are available.
	if (__beg == __end)
	  __err |= ios_base::eofbit;
	
	// Iff valid sequence is not recognized.
	if (!__testvalid)
	  __err |= ios_base::failbit;
	else
	  __units.swap(__res);
	
	return __beg;
      }

  template<typename _CharT, typename _InIter>
    _InIter
    money_get<_CharT, _InIter>::
    do_get(iter_type __beg, iter_type __end, bool __intl, ios_base& __io,
	   ios_base::iostate& __err, long double& __units) const
    {
      string __str;
      if (__intl)
	__beg = _M_extract<true>(__beg, __end, __io, __err, __str);
      else
	__beg = _M_extract<false>(__beg, __end, __io, __err, __str);
      std::__convert_to_v(__str.c_str(), __units, __err, _S_get_c_locale());
      return __beg;
    }

  template<typename _CharT, typename _InIter>
    _InIter
    money_get<_CharT, _InIter>::
    do_get(iter_type __beg, iter_type __end, bool __intl, ios_base& __io,
	   ios_base::iostate& __err, string_type& __units) const
    {
      typedef typename string::size_type                  size_type;

      const locale& __loc = __io._M_getloc();
      const ctype<_CharT>& __ctype = use_facet<ctype<_CharT> >(__loc);

      string __str;
      const iter_type __ret = __intl ? _M_extract<true>(__beg, __end, __io,
							__err, __str)
	                             : _M_extract<false>(__beg, __end, __io,
							 __err, __str);
      const size_type __len = __str.size();
      if (__len)
	{
	  _CharT* __ws = static_cast<_CharT*>(__builtin_alloca(sizeof(_CharT)
							       * __len));
	  __ctype.widen(__str.data(), __str.data() + __len, __ws);
	  __units.assign(__ws, __len);
	}

      return __ret;
    }

  template<typename _CharT, typename _OutIter>
    template<bool _Intl>
      _OutIter
      money_put<_CharT, _OutIter>::
      _M_insert(iter_type __s, ios_base& __io, char_type __fill,
		const string_type& __digits) const
      {
	typedef typename string_type::size_type	          size_type;
	typedef money_base::part                          part;
	typedef moneypunct<_CharT, _Intl>                 __moneypunct_type;
	typedef typename __moneypunct_type::__cache_type  __cache_type;
      
	const locale& __loc = __io._M_getloc();
	const ctype<_CharT>& __ctype = use_facet<ctype<_CharT> >(__loc);

	__use_cache<__cache_type> __uc;
	const __cache_type* __lc = __uc(__loc);
	const char_type* __lit = __lc->_M_atoms;

	// Determine if negative or positive formats are to be used, and
	// discard leading negative_sign if it is present.
	const char_type* __beg = __digits.data();

	money_base::pattern __p;
	const char_type* __sign;
	size_type __sign_size;
	if (*__beg != __lit[money_base::_S_minus])
	  {
	    __p = __lc->_M_pos_format;
	    __sign = __lc->_M_positive_sign;
	    __sign_size = __lc->_M_positive_sign_size;
	  }
	else
	  {
	    __p = __lc->_M_neg_format;
	    __sign = __lc->_M_negative_sign;
	    __sign_size = __lc->_M_negative_sign_size;
	    if (__digits.size())
	      ++__beg;
	  }
       
	// Look for valid numbers in the ctype facet within input digits.
	size_type __len = __ctype.scan_not(ctype_base::digit, __beg,
					   __beg + __digits.size()) - __beg;
	if (__len)
	  {
	    // Assume valid input, and attempt to format.
	    // Break down input numbers into base components, as follows:
	    //   final_value = grouped units + (decimal point) + (digits)
	    string_type __value;
	    __value.reserve(2 * __len);

	    // Add thousands separators to non-decimal digits, per
	    // grouping rules.
	    int __paddec = __len - __lc->_M_frac_digits;
	    if (__paddec > 0)
  	      {
		if (__lc->_M_frac_digits < 0)
		  __paddec = __len;
  		if (__lc->_M_grouping_size)
  		  {
		    _CharT* __ws =
  		      static_cast<_CharT*>(__builtin_alloca(sizeof(_CharT)
  							    * 2 * __len));
  		    _CharT* __ws_end =
		      std::__add_grouping(__ws, __lc->_M_thousands_sep,
					  __lc->_M_grouping,
					  __lc->_M_grouping_size,
					  __beg, __beg + __paddec);
		    __value.assign(__ws, __ws_end - __ws);
  		  }
  		else
		  __value.assign(__beg, __paddec);
	      }

	    // Deal with decimal point, decimal digits.
	    if (__lc->_M_frac_digits > 0)
	      {
		__value += __lc->_M_decimal_point;
		if (__paddec >= 0)
		  __value.append(__beg + __paddec, __lc->_M_frac_digits);
		else
		  {
		    // Have to pad zeros in the decimal position.
		    __value.append(-__paddec, __lit[money_base::_S_zero]);
		    __value.append(__beg, __len);
		  }
  	      }
  
	    // Calculate length of resulting string.
	    const ios_base::fmtflags __f = __io.flags() 
	                                   & ios_base::adjustfield;
	    __len = __value.size() + __sign_size;
	    __len += ((__io.flags() & ios_base::showbase)
		      ? __lc->_M_curr_symbol_size : 0);

	    string_type __res;
	    __res.reserve(2 * __len);
	    
	    const size_type __width = static_cast<size_type>(__io.width());  
	    const bool __testipad = (__f == ios_base::internal
				     && __len < __width);
	    // Fit formatted digits into the required pattern.
	    for (int __i = 0; __i < 4; ++__i)
	      {
		const part __which = static_cast<part>(__p.field[__i]);
		switch (__which)
		  {
		  case money_base::symbol:
		    if (__io.flags() & ios_base::showbase)
		      __res.append(__lc->_M_curr_symbol,
				   __lc->_M_curr_symbol_size);
		    break;
		  case money_base::sign:
		    // Sign might not exist, or be more than one
		    // charater long. In that case, add in the rest
		    // below.
		    if (__sign_size)
		      __res += __sign[0];
		    break;
		  case money_base::value:
		    __res += __value;
		    break;
		  case money_base::space:
		    // At least one space is required, but if internal
		    // formatting is required, an arbitrary number of
		    // fill spaces will be necessary.
		    if (__testipad)
		      __res.append(__width - __len, __fill);
		    else
		      __res += __fill;
		    break;
		  case money_base::none:
		    if (__testipad)
		      __res.append(__width - __len, __fill);
		    break;
		  }
	      }
	    
	    // Special case of multi-part sign parts.
	    if (__sign_size > 1)
	      __res.append(__sign + 1, __sign_size - 1);
	    
	    // Pad, if still necessary.
	    __len = __res.size();
	    if (__width > __len)
	      {
		if (__f == ios_base::left)
		  // After.
		  __res.append(__width - __len, __fill);
		else
		  // Before.
		  __res.insert(0, __width - __len, __fill);
		__len = __width;
	      }
	    
	    // Write resulting, fully-formatted string to output iterator.
	    __s = std::__write(__s, __res.data(), __len);
	  }
	__io.width(0);
	return __s;    
      }
  
  template<typename _CharT, typename _OutIter>
    _OutIter
    money_put<_CharT, _OutIter>::
    do_put(iter_type __s, bool __intl, ios_base& __io, char_type __fill,
	   long double __units) const
    {
      const locale __loc = __io.getloc();
      const ctype<_CharT>& __ctype = use_facet<ctype<_CharT> >(__loc);
#ifdef _GLIBCXX_USE_C99
      // First try a buffer perhaps big enough.
      int __cs_size = 64;
      char* __cs = static_cast<char*>(__builtin_alloca(__cs_size));
      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 328. Bad sprintf format modifier in money_put<>::do_put()
      int __len = std::__convert_from_v(__cs, __cs_size, "%.0Lf", __units,
					_S_get_c_locale());
      // If the buffer was not large enough, try again with the correct size.
      if (__len >= __cs_size)
	{
	  __cs_size = __len + 1;
	  __cs = static_cast<char*>(__builtin_alloca(__cs_size));
	  __len = std::__convert_from_v(__cs, __cs_size, "%.0Lf", __units,
					_S_get_c_locale());
	}
#else
      // max_exponent10 + 1 for the integer part, + 2 for sign and '\0'.
      const int __cs_size = numeric_limits<long double>::max_exponent10 + 3;
      char* __cs = static_cast<char*>(__builtin_alloca(__cs_size));
      int __len = std::__convert_from_v(__cs, 0, "%.0Lf", __units,
					_S_get_c_locale());
#endif
      _CharT* __ws = static_cast<_CharT*>(__builtin_alloca(sizeof(_CharT)
							   * __cs_size));
      __ctype.widen(__cs, __cs + __len, __ws);
      const string_type __digits(__ws, __len);
      return __intl ? _M_insert<true>(__s, __io, __fill, __digits)
	            : _M_insert<false>(__s, __io, __fill, __digits);
    }

  template<typename _CharT, typename _OutIter>
    _OutIter
    money_put<_CharT, _OutIter>::
    do_put(iter_type __s, bool __intl, ios_base& __io, char_type __fill,
	   const string_type& __digits) const
    { return __intl ? _M_insert<true>(__s, __io, __fill, __digits)
	            : _M_insert<false>(__s, __io, __fill, __digits); }


  // NB: Not especially useful. Without an ios_base object or some
  // kind of locale reference, we are left clawing at the air where
  // the side of the mountain used to be...
  template<typename _CharT, typename _InIter>
    time_base::dateorder
    time_get<_CharT, _InIter>::do_date_order() const
    { return time_base::no_order; }

  // Recursively expand a strftime format string and parse it.  Starts w/ %x
  // and %X from do_get_time() and do_get_date(), which translate to a more
  // specific string, which may contain yet more strings.  I.e. %x => %r =>
  // %H:%M:%S => extracted characters.
  template<typename _CharT, typename _InIter>
    _InIter
    time_get<_CharT, _InIter>::
    _M_extract_via_format(iter_type __beg, iter_type __end, ios_base& __io,
			  ios_base::iostate& __err, tm* __tm,
			  const _CharT* __format) const
    {
      const locale& __loc = __io._M_getloc();
      const __timepunct<_CharT>& __tp = use_facet<__timepunct<_CharT> >(__loc);
      const ctype<_CharT>& __ctype = use_facet<ctype<_CharT> >(__loc);
      const size_t __len = char_traits<_CharT>::length(__format);

      for (size_t __i = 0; __beg != __end && __i < __len && !__err; ++__i)
	{
	  if (__ctype.narrow(__format[__i], 0) == '%')
	    {
	      // Verify valid formatting code, attempt to extract.
	      char __c = __ctype.narrow(__format[++__i], 0);
	      int __mem = 0;
	      if (__c == 'E' || __c == 'O')
		__c = __ctype.narrow(__format[++__i], 0);
	      switch (__c)
		{
		  const char* __cs;
		  _CharT __wcs[10];
		case 'a':
		  // Abbreviated weekday name [tm_wday]
		  const char_type*  __days1[7];
		  __tp._M_days_abbreviated(__days1);
		  __beg = _M_extract_name(__beg, __end, __tm->tm_wday, __days1,
					  7, __io, __err);
		  break;
		case 'A':
		  // Weekday name [tm_wday].
		  const char_type*  __days2[7];
		  __tp._M_days(__days2);
		  __beg = _M_extract_name(__beg, __end, __tm->tm_wday, __days2,
					  7, __io, __err);
		  break;
		case 'h':
		case 'b':
		  // Abbreviated month name [tm_mon]
		  const char_type*  __months1[12];
		  __tp._M_months_abbreviated(__months1);
		  __beg = _M_extract_name(__beg, __end, __tm->tm_mon, 
					  __months1, 12, __io, __err);
		  break;
		case 'B':
		  // Month name [tm_mon].
		  const char_type*  __months2[12];
		  __tp._M_months(__months2);
		  __beg = _M_extract_name(__beg, __end, __tm->tm_mon, 
					  __months2, 12, __io, __err);
		  break;
		case 'c':
		  // Default time and date representation.
		  const char_type*  __dt[2];
		  __tp._M_date_time_formats(__dt);
		  __beg = _M_extract_via_format(__beg, __end, __io, __err, 
						__tm, __dt[0]);
		  break;
		case 'd':
		  // Day [01, 31]. [tm_mday]
		  __beg = _M_extract_num(__beg, __end, __tm->tm_mday, 1, 31, 2,
					 __io, __err);
		  break;
		case 'e':
		  // Day [1, 31], with single digits preceded by
		  // space. [tm_mday]
		  if (__ctype.is(ctype_base::space, *__beg))
		    __beg = _M_extract_num(++__beg, __end, __tm->tm_mday, 1, 9,
					   1, __io, __err);
		  else
		    __beg = _M_extract_num(__beg, __end, __tm->tm_mday, 10, 31,
					   2, __io, __err);
		  break;
		case 'D':
		  // Equivalent to %m/%d/%y.[tm_mon, tm_mday, tm_year]
		  __cs = "%m/%d/%y";
		  __ctype.widen(__cs, __cs + 9, __wcs);
		  __beg = _M_extract_via_format(__beg, __end, __io, __err, 
						__tm, __wcs);
		  break;
		case 'H':
		  // Hour [00, 23]. [tm_hour]
		  __beg = _M_extract_num(__beg, __end, __tm->tm_hour, 0, 23, 2,
					 __io, __err);
		  break;
		case 'I':
		  // Hour [01, 12]. [tm_hour]
		  __beg = _M_extract_num(__beg, __end, __tm->tm_hour, 1, 12, 2,
					 __io, __err);
		  break;
		case 'm':
		  // Month [01, 12]. [tm_mon]
		  __beg = _M_extract_num(__beg, __end, __mem, 1, 12, 2, 
					 __io, __err);
		  if (!__err)
		    __tm->tm_mon = __mem - 1;
		  break;
		case 'M':
		  // Minute [00, 59]. [tm_min]
		  __beg = _M_extract_num(__beg, __end, __tm->tm_min, 0, 59, 2,
					 __io, __err);
		  break;
		case 'n':
		  if (__ctype.narrow(*__beg, 0) == '\n')
		    ++__beg;
		  else
		    __err |= ios_base::failbit;
		  break;
		case 'R':
		  // Equivalent to (%H:%M).
		  __cs = "%H:%M";
		  __ctype.widen(__cs, __cs + 6, __wcs);
		  __beg = _M_extract_via_format(__beg, __end, __io, __err, 
						__tm, __wcs);
		  break;
		case 'S':
		  // Seconds.
		  __beg = _M_extract_num(__beg, __end, __tm->tm_sec, 0, 59, 2,
					 __io, __err);
		  break;
		case 't':
		  if (__ctype.narrow(*__beg, 0) == '\t')
		    ++__beg;
		  else
		    __err |= ios_base::failbit;
		  break;
		case 'T':
		  // Equivalent to (%H:%M:%S).
		  __cs = "%H:%M:%S";
		  __ctype.widen(__cs, __cs + 9, __wcs);
		  __beg = _M_extract_via_format(__beg, __end, __io, __err, 
						__tm, __wcs);
		  break;
		case 'x':
		  // Locale's date.
		  const char_type*  __dates[2];
		  __tp._M_date_formats(__dates);
		  __beg = _M_extract_via_format(__beg, __end, __io, __err, 
						__tm, __dates[0]);
		  break;
		case 'X':
		  // Locale's time.
		  const char_type*  __times[2];
		  __tp._M_time_formats(__times);
		  __beg = _M_extract_via_format(__beg, __end, __io, __err, 
						__tm, __times[0]);
		  break;
		case 'y':
		case 'C': // C99
		  // Two digit year. [tm_year]
		  __beg = _M_extract_num(__beg, __end, __tm->tm_year, 0, 99, 2,
					 __io, __err);
		  break;
		case 'Y':
		  // Year [1900). [tm_year]
		  __beg = _M_extract_num(__beg, __end, __mem, 0, 9999, 4,
					 __io, __err);
		  if (!__err)
		    __tm->tm_year = __mem - 1900;
		  break;
		case 'Z':
		  // Timezone info.
		  if (__ctype.is(ctype_base::upper, *__beg))
		    {
		      int __tmp;
		      __beg = _M_extract_name(__beg, __end, __tmp,
				       __timepunct_cache<_CharT>::_S_timezones,
					      14, __io, __err);

		      // GMT requires special effort.
		      if (__beg != __end && !__err && __tmp == 0
			  && (*__beg == __ctype.widen('-')
			      || *__beg == __ctype.widen('+')))
			{
			  __beg = _M_extract_num(__beg, __end, __tmp, 0, 23, 2,
						 __io, __err);
			  __beg = _M_extract_num(__beg, __end, __tmp, 0, 59, 2,
						 __io, __err);
			}
		    }
		  else
		    __err |= ios_base::failbit;
		  break;
		default:
		  // Not recognized.
		  __err |= ios_base::failbit;
		}
	    }
	  else
	    {
	      // Verify format and input match, extract and discard.
	      if (__format[__i] == *__beg)
		++__beg;
	      else
		__err |= ios_base::failbit;
	    }
	}
      return __beg;
    }

  template<typename _CharT, typename _InIter>
    _InIter
    time_get<_CharT, _InIter>::
    _M_extract_num(iter_type __beg, iter_type __end, int& __member,
		   int __min, int __max, size_t __len,
		   ios_base& __io, ios_base::iostate& __err) const
    {
      const locale& __loc = __io._M_getloc();
      const ctype<_CharT>& __ctype = use_facet<ctype<_CharT> >(__loc);

      // As-is works for __len = 1, 2, 4, the values actually used.
      int __mult = __len == 2 ? 10 : (__len == 4 ? 1000 : 1);

      ++__min;
      size_t __i = 0;
      int __value = 0;
      for (; __beg != __end && __i < __len; ++__beg, ++__i)
	{
	  const char __c = __ctype.narrow(*__beg, '*');
	  if (__c >= '0' && __c <= '9')
	    {
	      __value = __value * 10 + (__c - '0');
	      const int __valuec = __value * __mult;
	      if (__valuec > __max || __valuec + __mult < __min)
		break;
	      __mult /= 10;
	    }
	  else
	    break;
	}
      if (__i == __len)
	__member = __value;
      else
	__err |= ios_base::failbit;
      return __beg;
    }

  // Assumptions:
  // All elements in __names are unique.
  template<typename _CharT, typename _InIter>
    _InIter
    time_get<_CharT, _InIter>::
    _M_extract_name(iter_type __beg, iter_type __end, int& __member,
		    const _CharT** __names, size_t __indexlen,
		    ios_base& __io, ios_base::iostate& __err) const
    {
      typedef char_traits<_CharT>		__traits_type;
      const locale& __loc = __io._M_getloc();
      const ctype<_CharT>& __ctype = use_facet<ctype<_CharT> >(__loc);

      int* __matches = static_cast<int*>(__builtin_alloca(sizeof(int)
							  * __indexlen));
      size_t __nmatches = 0;
      size_t __pos = 0;
      bool __testvalid = true;
      const char_type* __name;

      // Look for initial matches.
      // NB: Some of the locale data is in the form of all lowercase
      // names, and some is in the form of initially-capitalized
      // names. Look for both.
      if (__beg != __end)
	{
	  const char_type __c = *__beg;
	  for (size_t __i1 = 0; __i1 < __indexlen; ++__i1)
	    if (__c == __names[__i1][0]
		|| __c == __ctype.toupper(__names[__i1][0]))
	      __matches[__nmatches++] = __i1;
	}

      while (__nmatches > 1)
	{
	  // Find smallest matching string.
	  size_t __minlen = 10;
	  for (size_t __i2 = 0; __i2 < __nmatches; ++__i2)
	    __minlen = std::min(__minlen,
			      __traits_type::length(__names[__matches[__i2]]));
	  ++__beg;
	  if (__pos < __minlen && __beg != __end)
	    {
	      ++__pos;
	      for (size_t __i3 = 0; __i3 < __nmatches; ++__i3)
		{
		  __name = __names[__matches[__i3]];
		  if (__name[__pos] != *__beg)
		    __matches[__i3] = __matches[--__nmatches];
		}
	    }
	  else
	    break;
	}

      if (__nmatches == 1)
	{
	  // If there was only one match, the first compare is redundant.
	  if (__pos == 0)
	    {
	      ++__pos;
	      ++__beg;
	    }

	  // Make sure found name is completely extracted.
	  __name = __names[__matches[0]];
	  const size_t __len = __traits_type::length(__name);
	  while (__pos < __len && __beg != __end && __name[__pos] == *__beg)
	    ++__beg, ++__pos;

	  if (__len == __pos)
	    __member = __matches[0];
	  else
	    __testvalid = false;
	}
      else
	__testvalid = false;
      if (!__testvalid)
	__err |= ios_base::failbit;
      return __beg;
    }

  template<typename _CharT, typename _InIter>
    _InIter
    time_get<_CharT, _InIter>::
    do_get_time(iter_type __beg, iter_type __end, ios_base& __io,
		ios_base::iostate& __err, tm* __tm) const
    {
      _CharT __wcs[3];
      const char* __cs = "%X";
      const locale& __loc = __io._M_getloc();
      ctype<_CharT> const& __ctype = use_facet<ctype<_CharT> >(__loc);
      __ctype.widen(__cs, __cs + 3, __wcs);
      __beg = _M_extract_via_format(__beg, __end, __io, __err, __tm, __wcs);
      if (__beg == __end)
	__err |= ios_base::eofbit;
      return __beg;
    }

  template<typename _CharT, typename _InIter>
    _InIter
    time_get<_CharT, _InIter>::
    do_get_date(iter_type __beg, iter_type __end, ios_base& __io,
		ios_base::iostate& __err, tm* __tm) const
    {
      _CharT __wcs[3];
      const char* __cs = "%x";
      const locale& __loc = __io._M_getloc();
      ctype<_CharT> const& __ctype = use_facet<ctype<_CharT> >(__loc);
      __ctype.widen(__cs, __cs + 3, __wcs);
      __beg = _M_extract_via_format(__beg, __end, __io, __err, __tm, __wcs);
      if (__beg == __end)
	__err |= ios_base::eofbit;
      return __beg;
    }

  template<typename _CharT, typename _InIter>
    _InIter
    time_get<_CharT, _InIter>::
    do_get_weekday(iter_type __beg, iter_type __end, ios_base& __io,
		   ios_base::iostate& __err, tm* __tm) const
    {
      typedef char_traits<_CharT>		__traits_type;
      const locale& __loc = __io._M_getloc();
      const __timepunct<_CharT>& __tp = use_facet<__timepunct<_CharT> >(__loc);
      const ctype<_CharT>& __ctype = use_facet<ctype<_CharT> >(__loc);
      const char_type*  __days[7];
      __tp._M_days_abbreviated(__days);
      int __tmpwday;
      __beg = _M_extract_name(__beg, __end, __tmpwday, __days, 7, __io, __err);

      // Check to see if non-abbreviated name exists, and extract.
      // NB: Assumes both _M_days and _M_days_abbreviated organized in
      // exact same order, first to last, such that the resulting
      // __days array with the same index points to a day, and that
      // day's abbreviated form.
      // NB: Also assumes that an abbreviated name is a subset of the name.
      if (!__err)
	{
	  size_t __pos = __traits_type::length(__days[__tmpwday]);
	  __tp._M_days(__days);
	  const char_type* __name = __days[__tmpwday];
	  if (__name[__pos] == *__beg)
	    {
	      // Extract the rest of it.
	      const size_t __len = __traits_type::length(__name);
	      while (__pos < __len && __beg != __end
		     && __name[__pos] == *__beg)
		++__beg, ++__pos;
	      if (__len != __pos)
		__err |= ios_base::failbit;
	    }
	  if (!__err)
	    __tm->tm_wday = __tmpwday;
	}
      if (__beg == __end)
	__err |= ios_base::eofbit;
      return __beg;
     }

  template<typename _CharT, typename _InIter>
    _InIter
    time_get<_CharT, _InIter>::
    do_get_monthname(iter_type __beg, iter_type __end,
                     ios_base& __io, ios_base::iostate& __err, tm* __tm) const
    {
      typedef char_traits<_CharT>		__traits_type;
      const locale& __loc = __io._M_getloc();
      const __timepunct<_CharT>& __tp = use_facet<__timepunct<_CharT> >(__loc);
      const ctype<_CharT>& __ctype = use_facet<ctype<_CharT> >(__loc);
      const char_type*  __months[12];
      __tp._M_months_abbreviated(__months);
      int __tmpmon;
      __beg = _M_extract_name(__beg, __end, __tmpmon, __months, 12, 
			      __io, __err);

      // Check to see if non-abbreviated name exists, and extract.
      // NB: Assumes both _M_months and _M_months_abbreviated organized in
      // exact same order, first to last, such that the resulting
      // __months array with the same index points to a month, and that
      // month's abbreviated form.
      // NB: Also assumes that an abbreviated name is a subset of the name.
      if (!__err)
	{
	  size_t __pos = __traits_type::length(__months[__tmpmon]);
	  __tp._M_months(__months);
	  const char_type* __name = __months[__tmpmon];
	  if (__name[__pos] == *__beg)
	    {
	      // Extract the rest of it.
	      const size_t __len = __traits_type::length(__name);
	      while (__pos < __len && __beg != __end
		     && __name[__pos] == *__beg)
		++__beg, ++__pos;
	      if (__len != __pos)
		__err |= ios_base::failbit;
	    }
	  if (!__err)
	    __tm->tm_mon = __tmpmon;
	}

      if (__beg == __end)
	__err |= ios_base::eofbit;
      return __beg;
    }

  template<typename _CharT, typename _InIter>
    _InIter
    time_get<_CharT, _InIter>::
    do_get_year(iter_type __beg, iter_type __end, ios_base& __io,
		ios_base::iostate& __err, tm* __tm) const
    {
      const locale& __loc = __io._M_getloc();
      const ctype<_CharT>& __ctype = use_facet<ctype<_CharT> >(__loc);

      size_t __i = 0;
      int __value = 0;
      for (; __beg != __end && __i < 4; ++__beg, ++__i)
	{
	  const char __c = __ctype.narrow(*__beg, '*');
	  if (__c >= '0' && __c <= '9')
	    __value = __value * 10 + (__c - '0');
	  else
	    break;
	}
      if (__i == 2 || __i == 4)
	__tm->tm_year = __i == 2 ? __value : __value - 1900;
      else
	__err |= ios_base::failbit;
      if (__beg == __end)
	__err |= ios_base::eofbit;
      return __beg;
    }

  template<typename _CharT, typename _OutIter>
    _OutIter
    time_put<_CharT, _OutIter>::
    put(iter_type __s, ios_base& __io, char_type __fill, const tm* __tm,
	const _CharT* __beg, const _CharT* __end) const
    {
      const locale& __loc = __io._M_getloc();
      ctype<_CharT> const& __ctype = use_facet<ctype<_CharT> >(__loc);
      for (; __beg != __end; ++__beg)
	if (__ctype.narrow(*__beg, 0) != '%')
	  {
	    *__s = *__beg;
	    ++__s;
	  }
	else if (++__beg != __end)
	  {
	    char __format;
	    char __mod = 0;
	    const char __c = __ctype.narrow(*__beg, 0);
	    if (__c != 'E' && __c != 'O')
	      __format = __c;
	    else if (++__beg != __end)
	      {
		__mod = __c;
		__format = __ctype.narrow(*__beg, 0);
	      }
	    else
	      break;
	    __s = this->do_put(__s, __io, __fill, __tm, __format, __mod);
	  }
	else
	  break;
      return __s;
    }

  template<typename _CharT, typename _OutIter>
    _OutIter
    time_put<_CharT, _OutIter>::
    do_put(iter_type __s, ios_base& __io, char_type, const tm* __tm,
	   char __format, char __mod) const
    {
      const locale& __loc = __io._M_getloc();
      ctype<_CharT> const& __ctype = use_facet<ctype<_CharT> >(__loc);
      __timepunct<_CharT> const& __tp = use_facet<__timepunct<_CharT> >(__loc);

      // NB: This size is arbitrary. Should this be a data member,
      // initialized at construction?
      const size_t __maxlen = 64;
      char_type* __res = 
       static_cast<char_type*>(__builtin_alloca(sizeof(char_type) * __maxlen));

      // NB: In IEE 1003.1-200x, and perhaps other locale models, it
      // is possible that the format character will be longer than one
      // character. Possibilities include 'E' or 'O' followed by a
      // format character: if __mod is not the default argument, assume
      // it's a valid modifier.
      char_type __fmt[4];
      __fmt[0] = __ctype.widen('%');
      if (!__mod)
	{
	  __fmt[1] = __format;
	  __fmt[2] = char_type();
	}
      else
	{
	  __fmt[1] = __mod;
	  __fmt[2] = __format;
	  __fmt[3] = char_type();
	}

      __tp._M_put(__res, __maxlen, __fmt, __tm);

      // Write resulting, fully-formatted string to output iterator.
      return std::__write(__s, __res, char_traits<char_type>::length(__res));
    }


  // Generic version does nothing.
  template<typename _CharT>
    int
    collate<_CharT>::_M_compare(const _CharT*, const _CharT*) const
    { return 0; }

  // Generic version does nothing.
  template<typename _CharT>
    size_t
    collate<_CharT>::_M_transform(_CharT*, const _CharT*, size_t) const
    { return 0; }

  template<typename _CharT>
    int
    collate<_CharT>::
    do_compare(const _CharT* __lo1, const _CharT* __hi1,
	       const _CharT* __lo2, const _CharT* __hi2) const
    {
      // strcoll assumes zero-terminated strings so we make a copy
      // and then put a zero at the end.
      const string_type __one(__lo1, __hi1);
      const string_type __two(__lo2, __hi2);

      const _CharT* __p = __one.c_str();
      const _CharT* __pend = __one.data() + __one.length();
      const _CharT* __q = __two.c_str();
      const _CharT* __qend = __two.data() + __two.length();

      // strcoll stops when it sees a nul character so we break
      // the strings into zero-terminated substrings and pass those
      // to strcoll.
      for (;;)
	{
	  const int __res = _M_compare(__p, __q);
	  if (__res)
	    return __res;

	  __p += char_traits<_CharT>::length(__p);
	  __q += char_traits<_CharT>::length(__q);
	  if (__p == __pend && __q == __qend)
	    return 0;
	  else if (__p == __pend)
	    return -1;
	  else if (__q == __qend)
	    return 1;

	  __p++;
	  __q++;
	}
    }

  template<typename _CharT>
    typename collate<_CharT>::string_type
    collate<_CharT>::
    do_transform(const _CharT* __lo, const _CharT* __hi) const
    {
      // strxfrm assumes zero-terminated strings so we make a copy
      string_type __str(__lo, __hi);

      const _CharT* __p = __str.c_str();
      const _CharT* __pend = __str.data() + __str.length();

      size_t __len = (__hi - __lo) * 2;

      string_type __ret;

      // strxfrm stops when it sees a nul character so we break
      // the string into zero-terminated substrings and pass those
      // to strxfrm.
      for (;;)
	{
	  // First try a buffer perhaps big enough.
	  _CharT* __c =
	    static_cast<_CharT*>(__builtin_alloca(sizeof(_CharT) * __len));
	  size_t __res = _M_transform(__c, __p, __len);
	  // If the buffer was not large enough, try again with the
	  // correct size.
	  if (__res >= __len)
	    {
	      __len = __res + 1;
	      __c = static_cast<_CharT*>(__builtin_alloca(sizeof(_CharT)
							  * __len));
	      __res = _M_transform(__c, __p, __res + 1);
	    }

	  __ret.append(__c, __res);
	  __p += char_traits<_CharT>::length(__p);
	  if (__p == __pend)
	    return __ret;

	  __p++;
	  __ret.push_back(_CharT());
	}
    }

  template<typename _CharT>
    long
    collate<_CharT>::
    do_hash(const _CharT* __lo, const _CharT* __hi) const
    {
      unsigned long __val = 0;
      for (; __lo < __hi; ++__lo)
	__val = *__lo + ((__val << 7) |
		       (__val >> (numeric_limits<unsigned long>::digits - 7)));
      return static_cast<long>(__val);
    }

  // Construct correctly padded string, as per 22.2.2.2.2
  // Assumes
  // __newlen > __oldlen
  // __news is allocated for __newlen size
  // Used by both num_put and ostream inserters: if __num,
  // internal-adjusted objects are padded according to the rules below
  // concerning 0[xX] and +-, otherwise, exactly as right-adjusted
  // ones are.

  // NB: Of the two parameters, _CharT can be deduced from the
  // function arguments. The other (_Traits) has to be explicitly specified.
  template<typename _CharT, typename _Traits>
    void
    __pad<_CharT, _Traits>::_S_pad(ios_base& __io, _CharT __fill,
				   _CharT* __news, const _CharT* __olds,
				   const streamsize __newlen,
				   const streamsize __oldlen, const bool __num)
    {
      const size_t __plen = static_cast<size_t>(__newlen - __oldlen);
      const ios_base::fmtflags __adjust = __io.flags() & ios_base::adjustfield;

      // Padding last.
      if (__adjust == ios_base::left)
	{
	  _Traits::copy(__news, const_cast<_CharT*>(__olds), __oldlen);
	  _Traits::assign(__news + __oldlen, __plen, __fill);
	  return;
	}

      size_t __mod = 0;
      if (__adjust == ios_base::internal && __num)
	{
	  // Pad after the sign, if there is one.
	  // Pad after 0[xX], if there is one.
	  // Who came up with these rules, anyway? Jeeze.
          const locale& __loc = __io._M_getloc();
	  const ctype<_CharT>& __ctype = use_facet<ctype<_CharT> >(__loc);

	  const bool __testsign = (__ctype.widen('-') == __olds[0]
				   || __ctype.widen('+') == __olds[0]);
	  const bool __testhex = (__ctype.widen('0') == __olds[0]
				  && __oldlen > 1
				  && (__ctype.widen('x') == __olds[1]
				      || __ctype.widen('X') == __olds[1]));
	  if (__testhex)
	    {
	      __news[0] = __olds[0];
	      __news[1] = __olds[1];
	      __mod = 2;
	      __news += 2;
	    }
	  else if (__testsign)
	    {
	      __news[0] = __olds[0];
	      __mod = 1;
	      ++__news;
	    }
	  // else Padding first.
	}
      _Traits::assign(__news, __plen, __fill);
      _Traits::copy(__news + __plen, const_cast<_CharT*>(__olds + __mod),
		    __oldlen - __mod);
    }

  bool
  __verify_grouping(const char* __grouping, size_t __grouping_size,
		    const string& __grouping_tmp)
  {
    const size_t __n = __grouping_tmp.size() - 1;
    const size_t __min = std::min(__n, __grouping_size - 1);
    size_t __i = __n;
    bool __test = true;
    
    // Parsed number groupings have to match the
    // numpunct::grouping string exactly, starting at the
    // right-most point of the parsed sequence of elements ...
    for (size_t __j = 0; __j < __min && __test; --__i, ++__j)
      __test = __grouping_tmp[__i] == __grouping[__j];
    for (; __i && __test; --__i)
      __test = __grouping_tmp[__i] == __grouping[__min];
    // ... but the last parsed grouping can be <= numpunct
    // grouping.
    __test &= __grouping_tmp[0] <= __grouping[__min];
    return __test;
  }

  template<typename _CharT>
    _CharT*
    __add_grouping(_CharT* __s, _CharT __sep,
		   const char* __gbeg, size_t __gsize,
		   const _CharT* __first, const _CharT* __last)
    {
      if (__last - __first > *__gbeg)
	{
	  const bool __bump = __gsize != 1;
	  __s = std::__add_grouping(__s,  __sep, __gbeg + __bump,
				    __gsize - __bump, __first,
				    __last - *__gbeg);
	  __first = __last - *__gbeg;
	  *__s++ = __sep;
	}
      do
	*__s++ = *__first++;
      while (__first != __last);
      return __s;
    }

  // Inhibit implicit instantiations for required instantiations,
  // which are defined via explicit instantiations elsewhere.
  // NB: This syntax is a GNU extension.
#if _GLIBCXX_EXTERN_TEMPLATE
  extern template class moneypunct<char, false>;
  extern template class moneypunct<char, true>;
  extern template class moneypunct_byname<char, false>;
  extern template class moneypunct_byname<char, true>;
  extern template class money_get<char>;
  extern template class money_put<char>;
  extern template class numpunct<char>;
  extern template class numpunct_byname<char>;
  extern template class num_get<char>;
  extern template class num_put<char>;
  extern template class __timepunct<char>;
  extern template class time_put<char>;
  extern template class time_put_byname<char>;
  extern template class time_get<char>;
  extern template class time_get_byname<char>;
  extern template class messages<char>;
  extern template class messages_byname<char>;
  extern template class ctype_byname<char>;
  extern template class codecvt_byname<char, char, mbstate_t>;
  extern template class collate<char>;
  extern template class collate_byname<char>;

  extern template
    const codecvt<char, char, mbstate_t>&
    use_facet<codecvt<char, char, mbstate_t> >(const locale&);

  extern template
    const collate<char>&
    use_facet<collate<char> >(const locale&);

  extern template
    const numpunct<char>&
    use_facet<numpunct<char> >(const locale&);

  extern template
    const num_put<char>&
    use_facet<num_put<char> >(const locale&);

  extern template
    const num_get<char>&
    use_facet<num_get<char> >(const locale&);

  extern template
    const moneypunct<char, true>&
    use_facet<moneypunct<char, true> >(const locale&);

  extern template
    const moneypunct<char, false>&
    use_facet<moneypunct<char, false> >(const locale&);

  extern template
    const money_put<char>&
    use_facet<money_put<char> >(const locale&);

  extern template
    const money_get<char>&
    use_facet<money_get<char> >(const locale&);

  extern template
    const __timepunct<char>&
    use_facet<__timepunct<char> >(const locale&);

  extern template
    const time_put<char>&
    use_facet<time_put<char> >(const locale&);

  extern template
    const time_get<char>&
    use_facet<time_get<char> >(const locale&);

  extern template
    const messages<char>&
    use_facet<messages<char> >(const locale&);

  extern template
    bool
    has_facet<ctype<char> >(const locale&);

  extern template
    bool
    has_facet<codecvt<char, char, mbstate_t> >(const locale&);

  extern template
    bool
    has_facet<collate<char> >(const locale&);

  extern template
    bool
    has_facet<numpunct<char> >(const locale&);

  extern template
    bool
    has_facet<num_put<char> >(const locale&);

  extern template
    bool
    has_facet<num_get<char> >(const locale&);

  extern template
    bool
    has_facet<moneypunct<char> >(const locale&);

  extern template
    bool
    has_facet<money_put<char> >(const locale&);

  extern template
    bool
    has_facet<money_get<char> >(const locale&);

  extern template
    bool
    has_facet<__timepunct<char> >(const locale&);

  extern template
    bool
    has_facet<time_put<char> >(const locale&);

  extern template
    bool
    has_facet<time_get<char> >(const locale&);

  extern template
    bool
    has_facet<messages<char> >(const locale&);

#ifdef _GLIBCXX_USE_WCHAR_T
  extern template class moneypunct<wchar_t, false>;
  extern template class moneypunct<wchar_t, true>;
  extern template class moneypunct_byname<wchar_t, false>;
  extern template class moneypunct_byname<wchar_t, true>;
  extern template class money_get<wchar_t>;
  extern template class money_put<wchar_t>;
  extern template class numpunct<wchar_t>;
  extern template class numpunct_byname<wchar_t>;
  extern template class num_get<wchar_t>;
  extern template class num_put<wchar_t>;
  extern template class __timepunct<wchar_t>;
  extern template class time_put<wchar_t>;
  extern template class time_put_byname<wchar_t>;
  extern template class time_get<wchar_t>;
  extern template class time_get_byname<wchar_t>;
  extern template class messages<wchar_t>;
  extern template class messages_byname<wchar_t>;
  extern template class ctype_byname<wchar_t>;
  extern template class codecvt_byname<wchar_t, char, mbstate_t>;
  extern template class collate<wchar_t>;
  extern template class collate_byname<wchar_t>;

  extern template
    const codecvt<wchar_t, char, mbstate_t>&
    use_facet<codecvt<wchar_t, char, mbstate_t> >(locale const&);

  extern template
    const collate<wchar_t>&
    use_facet<collate<wchar_t> >(const locale&);

  extern template
    const numpunct<wchar_t>&
    use_facet<numpunct<wchar_t> >(const locale&);

  extern template
    const num_put<wchar_t>&
    use_facet<num_put<wchar_t> >(const locale&);

  extern template
    const num_get<wchar_t>&
    use_facet<num_get<wchar_t> >(const locale&);

  extern template
    const moneypunct<wchar_t, true>&
    use_facet<moneypunct<wchar_t, true> >(const locale&);

  extern template
    const moneypunct<wchar_t, false>&
    use_facet<moneypunct<wchar_t, false> >(const locale&);

  extern template
    const money_put<wchar_t>&
    use_facet<money_put<wchar_t> >(const locale&);

  extern template
    const money_get<wchar_t>&
    use_facet<money_get<wchar_t> >(const locale&);

  extern template
    const __timepunct<wchar_t>&
    use_facet<__timepunct<wchar_t> >(const locale&);

  extern template
    const time_put<wchar_t>&
    use_facet<time_put<wchar_t> >(const locale&);

  extern template
    const time_get<wchar_t>&
    use_facet<time_get<wchar_t> >(const locale&);

  extern template
    const messages<wchar_t>&
    use_facet<messages<wchar_t> >(const locale&);

 extern template
    bool
    has_facet<ctype<wchar_t> >(const locale&);

  extern template
    bool
    has_facet<codecvt<wchar_t, char, mbstate_t> >(const locale&);

  extern template
    bool
    has_facet<collate<wchar_t> >(const locale&);

  extern template
    bool
    has_facet<numpunct<wchar_t> >(const locale&);

  extern template
    bool
    has_facet<num_put<wchar_t> >(const locale&);

  extern template
    bool
    has_facet<num_get<wchar_t> >(const locale&);

  extern template
    bool
    has_facet<moneypunct<wchar_t> >(const locale&);

  extern template
    bool
    has_facet<money_put<wchar_t> >(const locale&);

  extern template
    bool
    has_facet<money_get<wchar_t> >(const locale&);

  extern template
    bool
    has_facet<__timepunct<wchar_t> >(const locale&);

  extern template
    bool
    has_facet<time_put<wchar_t> >(const locale&);

  extern template
    bool
    has_facet<time_get<wchar_t> >(const locale&);

  extern template
    bool
    has_facet<messages<wchar_t> >(const locale&);
#endif
#endif
} // namespace std

#endif
