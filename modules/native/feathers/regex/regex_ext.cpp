/*
   FALCON - The Falcon Programming Language.
   FILE: regex_ext.cpp

   Regular expression module.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: sab mar 11 2006

   -------------------------------------------------------------------
   (C) Copyright 2004: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

/** \file
   Regex
*/

#include <falcon/vm.h>
#include <falcon/string.h>
#include <falcon/memory.h>
#include <falcon/fassert.h>
#include <falcon/autocstring.h>

#include <string.h>

#include "regex_ext.h"
#include "regex_mod.h"
#include "regex_st.h"

/*#
   @beginmodule feathers.regex
*/

namespace Falcon {
namespace Ext {

/*#
   @class Regex
   @brief Regular expression binding encapsulation.
   @param pattern The regular expression pattern to be compiled.
   @optparam options Pattern compilation options.
   @raise RegexError if the pattern is invalid.

   The class constructor creates a Regex instance that can be then used to match, find,
   extract  and substitute strings. Compilation of regular expressions can be an heavy step,
   so it's better to do it once and for all. In a program using repeatedly a set of well
   defined patterns, an option worth being considered is that of creating object instances
   that will have the VM to compile the pattern in the link step:

   @code
   load regex

   object bigPattern from Regex( "A pattern (.*) to be compiled" )
      // we may eventually consider also a study step in init
      init
         self.study()
      end
   end

   //...
   if bigPattern.match( "a string" )
      ...
   end
   @endcode

   In case of regular expression pattern error, a RegexError instance will be raised.
   The option parameter can be provided to pass pattern compilation options to the
   constructor. It generally follows the PERL regular expression parameter
   specification, so  that:
   @code
   PERL: /a pattern/i
   Falcon: Regex( "a pattern", "i" )
   @endcode

   The recognized options are (case sensitive):
   - @b a: anchored pattern. Usually, if only part of a pattern matches, a new search is
      attempted in the rest of the string. When this option is set, the pattern must either match
      completely or be rejected as the first match begins. So, the pattern "abc" will usually
      match "abdabc" starting from character 3, but if anchored option is set it won't match, as
      "abc" will start to match at the beginning of string, but then will fail when "d" is met.
   - @b i: ignore case while matching. Case ignoring is currently supported only for Unicode
      characters below 128; this means that accented latin case ignoring is not supported. For
      example, "è" and "È" won't match even if "i" option is set.
   - @b m: multiline match. Usually, the special characters "^" and "$" matches respectively
      the begin and the end of the string, so that "^pattern$" will match only "pattern", and not
      "this is a pattern". With "m" option, "^" and "$" matches the begin and the end of a line,
      being lines separated by Falcon newline characters ("\n").

   - @b s: dot match all. Usually, the line separator "\n" is not captured in wide
      parenthesized expressions as (.*), and the generic "any character" wildcard (the dot ".")
      doesn't matches it. With "s" option, "\n" is considered as a normal character and included
      in matches (although this doesn't alter the behavior of "^" and "$" which are controlled by
      the "m" option).
   - @b f: first line. Match must start before or at the first "\n", or else it will fail.
   - @b g: ungreedy match. Repetition patterns behave normally in a way that is defined
      "greedy", unless followed by a question mark. Greedy matching will force quantifiers to
      match as many characters as possible in the target string. For example, normally
      'field:\s*(.*);', if applied to a string with two ";" after "field" will return the longest
      possible match. If applied to "field: a; b; c; and the rest" would return "a; b; c" as first
      submatch. Using a question mark would force to match the shortest alternative:
      'field:\s*(.*?);' would return only "a" as first submatch. The "g" option inverts this
      behavior, so that repetition quantifiers are ungreedy by default, and "?" will make them
      greedy.
   - @b S: study the pattern. Tells Regex constructor to perform also a study step after
      compilation. According to PCRE documentation, this has currently little utility except for
      some very complex pattern involving recursive searches.

   In case of error in compilation of the pattern (or eventually in the study step, if
   required), the constructor will raise an error of class @a RegexError.
*/

FALCON_FUNC Regex_init( ::Falcon::VMachine *vm )
{
   CoreObject *self = vm->self().asObject();
   Item *param = vm->param(0);
   Item *options = vm->param(1);

   if( param == 0 || ! param->isString() || ( options != 0 && ! options->isString() ) )
   {
      throw new  ParamError( ErrorParam( e_inv_params, __LINE__ ).
         extra( "S, [S]" ) );
   }

   // read pattern options

   int optVal = 0;
   bool bStudy = false;

   if( options != 0 )
   {
      String *optString = options->asString();
      for( uint32 i = 0; i < optString->length(); i++ )
      {
         switch ( optString->getCharAt( i ) )
         {
            case 'a': optVal |= PCRE_ANCHORED; break;
            case 'i': optVal |= PCRE_CASELESS; break;
            case 'm': optVal |= PCRE_MULTILINE; break;
            case 's': optVal |= PCRE_DOTALL; break;
            case 'f': optVal |= PCRE_FIRSTLINE; break;
            case 'g': optVal |= PCRE_UNGREEDY; break;
            case 'S': bStudy = true; break;
            default:
               throw new ParamError( ErrorParam( e_param_range, __LINE__ ).
                  extra( FAL_STR( re_msg_optunknown ) ) );
         }
      }
   }

   // determine the type of the string.
   String *source = param->asString();
   pcre *pattern;
   int errCode;
   const char *errDesc;
   int errOffset;

   AutoCString cstr( *source );
   pattern = pcre_compile2(
      cstr.c_str(),
      optVal | PCRE_UTF8 | PCRE_NO_UTF8_CHECK,
      &errCode,
      &errDesc,
      &errOffset,
      0 );

   if ( pattern == 0 )
   {
      throw new RegexError( ErrorParam( FALRE_ERR_INVALID, __LINE__ )
         .desc( FAL_STR( re_msg_invalid ) )
         .extra( errDesc ) );
      return;
   }

   RegexCarrier *data = new RegexCarrier( pattern );
   self->setUserData( data );

   if ( bStudy )
   {
      data->m_extra = pcre_study( pattern, 0, &errDesc );
      if ( data->m_extra == 0 && errDesc != 0 )
      {
         throw new RegexError( ErrorParam( FALRE_ERR_STUDY, __LINE__ )
            .desc( FAL_STR( re_msg_errstudy ) )
            .extra( errDesc ) );
         return;
      }
   }
}

/*#
   @method study Regex
   @brief Study the pattern after compilation.

   It perform an extra compilation step; PCRE 7.6 manual suggests that this is
   useful only with recursive pattern.
*/
FALCON_FUNC Regex_study( ::Falcon::VMachine *vm )
{
   CoreObject *self = vm->self().asObject();
   RegexCarrier *data = ( RegexCarrier *) self->getUserData();

   if( data->m_extra != 0 )
   {
      // already studied
      return;
   }

   const char *errDesc;
   data->m_extra = pcre_study( data->m_pattern, 0, &errDesc );
   if ( data->m_extra == 0 && errDesc != 0 )
   {
      throw new RegexError( ErrorParam( FALRE_ERR_STUDY, __LINE__ )
            .desc( FAL_STR( re_msg_errstudy ) )
            .extra( errDesc ) );
   }
}


static int utf8_fwd_displacement( const char *stringData, int pos )
{
   int ret = 0;
   while( pos > 0  && stringData[ ret ] != 0 )
   {
      byte in = stringData[ ret ];

      // pattern 1111 0xxx
      if ( (in & 0xF8) == 0xF0 )
      {
         ret += 4;
      }
      // pattern 1110 xxxx
      else if ( (in & 0xF0) == 0xE0 )
      {
         ret += 3;
      }
      // pattern 110x xxxx
      else if ( (in & 0xE0) == 0xC0 )
      {
         ret += 2;
      }
      else if( in < 0x80 )
      {
         ret += 1;
      }
      // invalid pattern
      else {
         return -1;
      }

      pos--;
   }

   if ( pos == 0 )
      return ret;
   else
      return -1;
}



static int utf8_back_displacement( const char *stringData, int pos )
{
   int ret = 0;
   int pos1 = 0;
   while( pos1 < pos )
   {
      byte in = stringData[ pos1 ];

      // pattern 1111 0xxx
      if ( (in & 0xF8) == 0xF0 )
      {
         pos1 += 4;
      }
      // pattern 1110 xxxx
      else if ( (in & 0xF0) == 0xE0 )
      {
         pos1 += 3;
      }
      // pattern 110x xxxx
      else if ( (in & 0xE0) == 0xC0 )
      {
         pos1 += 2;
      }
      else if( in < 0x80 )
      {
         pos1 += 1;
      }
      // invalid pattern
      else {
         return -1;
      }

      ret ++;
   }

   return ret;
}


static void internal_regex_match( RegexCarrier *data, String *source, int from )
{
   AutoCString cstr( *source );

   // displace the from indicator to match utf-8
   from = utf8_fwd_displacement( cstr, from );
   if ( from == -1 )
   {
      data->m_matches = PCRE_ERROR_BADUTF8;
      return;
   }

   data->m_matches = pcre_exec(
      data->m_pattern,
      data->m_extra,
      cstr.c_str(),
      cstr.length(),
      from,
      PCRE_NO_UTF8_CHECK,
      data->m_ovector,
      data->m_ovectorSize );

   for( int i = 0; i < data->m_matches; i++ )
   {
      data->m_ovector[ i * 2 ] = utf8_back_displacement( cstr, data->m_ovector[ i * 2 ] );
      data->m_ovector[ i * 2 + 1 ] = utf8_back_displacement( cstr, data->m_ovector[ i * 2 + 1] );
   }
}

/*#
   @method match Regex
   @brief Matches this regular expression against a string.
   @param string String where to scan for the pattern.
   @return True if the string is matched by the pattern, false otherwise.

   This method searches for the pattern in the string given as parameter.
   If the match is successful, the method returns true.
   The match point can then be retrieved using the captured(0) method.
*/
FALCON_FUNC Regex_match( ::Falcon::VMachine *vm )
{
   CoreObject *self = vm->self().asObject();
   RegexCarrier *data = ( RegexCarrier *) self->getUserData();

   Item *source = vm->param(0);

   if( source == 0 || ! source->isString() )
   {
      throw new  ParamError( ErrorParam( e_inv_params, __LINE__ ).
         extra( "S" ) );
   }

   internal_regex_match( data, source->asString(), 0 );

   if ( data->m_matches == PCRE_ERROR_NOMATCH )
   {
      vm->regA().setBoolean( false );
      return;
   }

   if ( data->m_matches < 0 )
   {
      String errVal = FAL_STR( re_msg_internal );
      errVal.writeNumber( (int64) data->m_matches );
      throw new RegexError( ErrorParam( FALRE_ERR_ERRMATCH, __LINE__ )
         .desc( FAL_STR( re_msg_errmatch ) )
         .extra( errVal ) );
   }

   vm->regA().setBoolean( true );
}

/*#
   @method find Regex
   @brief Finds a range matching this regular expression in a string.
   @param string A string in which the pattern has to be found.
   @optparam start  An optional starting point in the string.
   @return A range where the pattern matches, or nil.

   This function works as the method match(), but on success it immediately returns the range
   containing the area in the string parameter where the pattern has matched.
   Also, this function can be provided with an optional start parameter that
   can be used to begin the search for the pattern from an arbitrary point in the string.

   Finds the first occourence of the pattern in the string. The returned ranged
   may be applied to the string in order to extract the desired substring.

   If the pattern doesn't matches, returns nil.
*/
FALCON_FUNC Regex_find( ::Falcon::VMachine *vm )
{
   CoreObject *self = vm->self().asObject();
   RegexCarrier *data = ( RegexCarrier *) self->getUserData();

   Item *source = vm->param(0);
   Item *from_i = vm->param(1);

   if( source == 0 || ! source->isString() || ( from_i != 0 && ! from_i->isOrdinal() ) )
   {
      throw new  ParamError( ErrorParam( e_inv_params, __LINE__ ).
         extra( "S, [I]" ) );
   }

   int from = 0;
   if ( from_i != 0 )
   {
      from = (int) from_i->forceInteger();
      if ( from < 0 )
         from = 0;
   }

   internal_regex_match( data, source->asString(), from );

   if ( data->m_matches >= 0  )
   {
      // we know by hypotesis that oVector is at least 1 entry.
      Item rng;
      rng.setRange( new CoreRange( data->m_ovector[0], data->m_ovector[1] ) );
      vm->retval( rng );
   }
   else if ( data->m_matches == PCRE_ERROR_NOMATCH ){
      vm->retnil();
   }
   else
   {
      String errVal = FAL_STR( re_msg_internal );
      errVal.writeNumber( (int64) data->m_matches );
      throw new RegexError( ErrorParam( FALRE_ERR_ERRMATCH, __LINE__ )
         .desc( FAL_STR( re_msg_errmatch ) )
         .extra( errVal ) );
   }
}

/*#
   @method split Regex
   @brief Splits a string at match points.
   @param string The string to be split.
   @optparam count  Maximum number of split instances.
   @optparam getoken Return also the found token.
   @return An array containing the string slices, or nil.

   If the pattern matches the string, the part before the match
   is isolated. The operation is iteratively performed until the
   match can't be found anymore; at that point, the last string is
   returned.

   If @b gettoken parameter is given and true, the found match is
   inserted between the isolated strings.

   If @b count is given, a maximum number of matches is performed,
   then the array of split entities is returned. Notice that the
   this doesn't count tokens returned if the @b gettoken
   option is specified.

   If the pattern doesn't matches, this method returns nil.
*/
FALCON_FUNC Regex_split( ::Falcon::VMachine *vm )
{
   CoreObject *self = vm->self().asObject();
   RegexCarrier *data = ( RegexCarrier *) self->getUserData();

   Item *source_i = vm->param(0);
   Item *count_i = vm->param(1);
   Item *gettoken_i = vm->param(2);

   if( source_i == 0 || ! source_i->isString()
      || ( count_i != 0 && ! ( count_i->isOrdinal() || count_i->isNil() ) )
      )
   {
      throw new  ParamError( ErrorParam( e_inv_params, __LINE__ ).
         extra( "S, [N], [B]" ) );
   }

   // Fast path: if we don't match, get away now.
   String* src = source_i->asString();
   internal_regex_match( data, src, 0 );

   if ( data->m_matches == PCRE_ERROR_NOMATCH ){
      vm->retnil();
      return;
   }
   else if( data->m_matches < 0 )
   {
      String errVal = FAL_STR( re_msg_internal );
      errVal.writeNumber( (int64) data->m_matches );
      throw new RegexError( ErrorParam( FALRE_ERR_ERRMATCH, __LINE__ )
         .desc( FAL_STR( re_msg_errmatch ) )
         .extra( errVal ) );
   }

   uint32 count = 0xFFFFFFFF;
   if ( count_i != 0 && ! count_i->isNil() )
   {
      uint32 ncount = (uint32) count_i->forceInteger();
      if( ncount != 0 )
         count = ncount;
   }

   bool bgt = gettoken_i != 0 && gettoken_i->isTrue();

   CoreArray* ret = new CoreArray;
   uint32 maxLen = src->length();
   uint32 from = 0;
   do
   {
      ret->append( new CoreString( *src, from, data->m_ovector[0] ) );
      if( bgt )
         ret->append( new CoreString( *src, data->m_ovector[0], data->m_ovector[1] ) );

      from = data->m_ovector[1];
      internal_regex_match( data, src, from );
      count--;
   }
   while( data->m_matches > 0 && count > 0 && from < maxLen );

   if( from < maxLen )
      ret->append( new CoreString( *src, from ) );

   vm->retval( ret );
}


static void internal_findAll( Falcon::VMachine *vm, bool overlapped )
{
   CoreObject *self = vm->self().asObject();
   RegexCarrier *data = ( RegexCarrier *) self->getUserData();
   Item *source = vm->param(0);
   Item *from_i = vm->param(1);
   Item *maxCount_i = vm->param(2);

   if( source == 0 || ! source->isString() ||
      ( from_i != 0 && ! from_i->isOrdinal() ) ||
      ( maxCount_i != 0 && ! maxCount_i->isOrdinal() )
      )
   {
      throw new  ParamError( ErrorParam( e_inv_params, __LINE__ ).
         extra( "S, [I], [I]" ) );
   }

   int from = 0;
   if ( from_i != 0 )
   {
      from = (int) from_i->forceInteger();
      if ( from < 0 )
         from = 0;
   }

   uint32 max = 0xFFFFFFFF;
   if ( maxCount_i != 0 )
   {
      max = (int) maxCount_i->forceInteger();
      if ( max <= 0 )
         max = 0xFFFFFFFF;
   }

   CoreArray *ca = new CoreArray();
   int frontOrBack = overlapped ? 0 : 1;
   uint32 maxLen = source->asString()->length();

   do {
      internal_regex_match( data, source->asString(), from );
      if( data->m_matches > 0 )
      {
         Item rng;
         rng.setRange( new CoreRange( data->m_ovector[0], data->m_ovector[1] ) );
         ca->append( rng );
         // restart from the end of the patter
         from = data->m_ovector[frontOrBack];
         // as we're going to exit.
      }
      max--;
   } while( data->m_matches > 0 && max > 0 && from < (int32) maxLen );

   if ( data->m_matches < 0 && data->m_matches != PCRE_ERROR_NOMATCH )
   {
      String errVal = FAL_STR( re_msg_internal );
      errVal.writeNumber( (int64) data->m_matches );
      throw new RegexError( ErrorParam( FALRE_ERR_ERRMATCH, __LINE__ )
         .desc( FAL_STR( re_msg_errmatch ) )
         .extra( errVal ) );
   }

   // always return an array, even if empty
   vm->retval( ca );
}

/*#
   @method findAll Regex
   @brief Find all ranges where this regular expression can mach the string.
   @param string String where to scan for the pattern.
   @optparam start Optional start position in the string.
   @optparam maxcount Optional maximum matches allowed .
   @return A vector of ranges where the pattern matches, or nil.

   This function returns an array containing all the ranges where
   the pattern has matched; if the pattern could not match the
   string, an empty array is returned.

   This method only returns the whole match, ignoring parenthesized
   expressions.
*/

FALCON_FUNC Regex_findAll( ::Falcon::VMachine *vm )
{
   internal_findAll( vm, false );
}

/*#
   @method findAllOverlapped Regex
   @brief Find all ranges where this regular expression can mach the string, with possibly overlapped matches.
   @param string String where to scan for the pattern.
   @optparam start Optional start position in the string.
   @optparam maxcount Optional maximum matches allowed .
   @return A vector of ranges where the pattern matches, or nil.

   This function returns an array containing all the ranges where
   the pattern has matched; if the pattern could not match the string,
   an empty array is returned.

   This method only returns the whole match, ignoring parenthesized
   expressions.
*/
FALCON_FUNC Regex_findAllOverlapped( ::Falcon::VMachine *vm )
{
   internal_findAll( vm, true );
}


/*#
   @method replace Regex
   @brief Replace a substring matching this regular expression with another string.
   @param string String where to scan for the pattern.
   @param replacer The string to replace the matched pattern with.
   @optparam start Optional initial scan position.
   @return The string with the matching content replaced.

   This method scans for the pattern in string, and if it's found, it
   is replaced with the string in the replacer parameter. The original
   string is untouched, and a new copy with the replaced value is
   returned. If the pattern can't match string, nil is returned. An optional
   start parameter can be given to begin the search for the pattern in
   string from a given position.
*/
FALCON_FUNC Regex_replace( ::Falcon::VMachine *vm )
{
   CoreObject *self = vm->self().asObject();
   RegexCarrier *data = ( RegexCarrier *) self->getUserData();

   Item *source_i = vm->param(0);
   Item *dest_i = vm->param(1);
   Item *from_i = vm->param(2);

   if( source_i == 0 || ! source_i->isString() || dest_i == 0 || ! dest_i->isString() ||
      ( from_i != 0 && ! from_i->isOrdinal() )
      )
   {
      throw new  ParamError( ErrorParam( e_inv_params, __LINE__ ).
         extra( "S, S, [I]" ) );
   }

   int from = 0;
   if ( from_i != 0  )
   {
      from = (int) from_i->forceInteger();
      if ( from < 0 )
         from = 0;
   }

   String *source = source_i->asString();
   String *dest = dest_i->asString();

   internal_regex_match( data, source, from );

   if ( data->m_matches == PCRE_ERROR_NOMATCH )
   {
      vm->retval( source );
      return;
   }

   if ( data->m_matches < 0 )
   {
      String errVal = FAL_STR( re_msg_internal );
      errVal.writeNumber( (int64) data->m_matches );
      throw new RegexError( ErrorParam( FALRE_ERR_ERRMATCH, __LINE__ )
         .desc( FAL_STR( re_msg_errmatch ) )
         .extra( errVal ) );
   }

   CoreString* ret = new CoreString(*source);
   ret->change( data->m_ovector[0], data->m_ovector[1], *dest );
   vm->retval( ret );
}


void s_expand( RegexCarrier *data, const String &orig, String &expanded )
{
   uint32 pos = 0;

   // use length method as actual size will change
   while( pos < expanded.length() )
   {
      if ( expanded.getCharAt( pos ) == '\\' )
      {
         pos++;
         if ( pos != expanded.length() )
         {
            // convert \\ into \;
            if( expanded.getCharAt( pos ) == '\\' )
            {
               expanded.remove( pos, 1 );
               continue;
            }

            uint32 val = expanded.getCharAt(pos) - 0x30;
            if ( ((uint32)data->m_matches) > val && val < 10 )
            {
               // is a valid number?
               expanded.change( pos-1, pos+1, orig.subString( data->m_ovector[val*2], data->m_ovector[val*2+1] ) );
               pos+= data->m_ovector[val*2+1] - data->m_ovector[val*2];
            }
         }
      }
      else
         pos++;
   }
}

static void s_replaceall( VMachine* vm, bool bExpand )
{
   CoreObject *self = vm->self().asObject();
   RegexCarrier *data = ( RegexCarrier *) self->getUserData();

   Item *source_i = vm->param(0);
   Item *dest_i = vm->param(1);

   if( source_i == 0 || ! source_i->isString() || dest_i == 0 || ! dest_i->isString() )
   {
      throw new  ParamError( ErrorParam( e_inv_params, __LINE__ ).
         extra( "S, S" ) );
   }

   String *source = source_i->asString();
   String *clone = 0;
   String *dest = dest_i->asString();
   uint32 destLen = dest->length();

   int from = 0;
   int oldFrom = 0;
   do {
      internal_regex_match( data, source, from );
      if( data->m_matches > 0 )
      {
         if ( data->m_ovector[0] == data->m_ovector[1] )
              break;

         if ( clone == 0 ) {
            clone = new CoreString( *source );
            source = clone;
         }
         
         if( bExpand )
         {
            String expanded( *dest );
            s_expand( data, *source, expanded );
            destLen = expanded.length();
            source->change( data->m_ovector[0], data->m_ovector[1], expanded );
         }
         else
            source->change( data->m_ovector[0], data->m_ovector[1], *dest );

         oldFrom = from;
         from = data->m_ovector[0] + destLen + 1;
         // as we're going to exit.
      }
   } while( data->m_matches > 0 && from < (int32) source->length() );


   if ( data->m_matches < 0 && data->m_matches != PCRE_ERROR_NOMATCH )
   {
      String errVal = FAL_STR( re_msg_internal );
      errVal.writeNumber( (int64) data->m_matches );
      throw new RegexError( ErrorParam( FALRE_ERR_ERRMATCH, __LINE__ )
         .desc( FAL_STR( re_msg_errmatch ) )
         .extra( errVal ) );
   }

   if ( clone != 0 )
      vm->retval( clone );
   else
      vm->retval( *source_i );
}


/*#
   @method replaceAll Regex
   @brief Replaces all the possible matches of this regular expression in a target with a given string.
   @param string String where to scan for the pattern.
   @param replacer The string to replace the matched pattern with.
   @return The string with the matching content replaced, or nil if no change is perfomed.

   This method replaces all the occurrences of the pattern in string with the
   replacer parameter. If a change can be performed, a modified instance
   of string is returned, else nil is returned.
*/
FALCON_FUNC Regex_replaceAll( ::Falcon::VMachine *vm )
{
   s_replaceall( vm, false );
}

/*#
   @method subst Regex
   @brief Replaces all the matches expanding placeholders.
   @param string String where to scan for the pattern.
   @param replacer The string to replace the matched pattern with.
   @return The string with the matching content replaced, or nil if no change is perfomed.

   This method works exacly like @a Regex.replaceAll, but it expands backslash
   placeholders with captured expressions. Each captured expression can be addressed
   via standard substitution backslashes (\0 is the whole expression, \1 is the first captured 
   expression, \2 the second and so on). 
   
   @code
   load regex
   
   r = Regex("a([0-9]+)b")
   > r.subst( 'a100b', 'Number was \1.' )
   @endcode
   
   @note Remember to use double backslash on double quoted strings.
*/
FALCON_FUNC Regex_subst( ::Falcon::VMachine *vm )
{
   s_replaceall( vm, true );
}


/*#
   @method capturedCount Regex
   @brief Return the count of captured (parenthesized) expressions.
   @return Count of captured subranges.

   This method returns available number of captured ranges after a
   successful match. This number is the amount of parenthesized
   expressions in the regular expression plus one.

   @see Regex.captured
*/

FALCON_FUNC Regex_capturedCount( ::Falcon::VMachine *vm )
{
   CoreObject *self = vm->self().asObject();
   RegexCarrier *data = ( RegexCarrier *) self->getUserData();

   if( data->m_matches > 0 )
      vm->retval( (int64) data->m_matches );
   else
      vm->retval( (int64) 0 );
}

/*#
   @method captured Regex
   @brief Return one of the captured (parenthesized) expressions.
   @param count Id of the captured substring, starting from 1; 0 represents all the matched string.
   @return A range defining a captured match.

   This method returns one of the match ranges that has been determined by
   the last match, find or replace operation. If 0 is passed as count parameter,
   the whole match range is returned; each parenthesized expression match
   range can be retrieved passing 1 or above as count parameter. The order of
   parenthesized expression is given considering the first parenthesis. The
   returned value is a closed range describing the area where the capture
   had effect in the target string.
*/
FALCON_FUNC Regex_captured( ::Falcon::VMachine *vm )
{
   CoreObject *self = vm->self().asObject();
   RegexCarrier *data = ( RegexCarrier *) self->getUserData();

   Item *pos_i = vm->param(0);
   if( pos_i == 0 || ! pos_i->isOrdinal() )
   {
      throw new  ParamError( ErrorParam( e_inv_params, __LINE__ ).
         extra( "I" ) );
   }


   // valid also if we didn't get a good count
   int maxCount = data->m_matches;
   int count = (int) pos_i->forceInteger();

   if ( count < 0 ||  count >= maxCount )
   {
      throw new  ParamError( ErrorParam( e_param_range, __LINE__ )
         .extra( FAL_STR( re_msg_outofrange ) ) );
   }

   Item rng;
   rng.setRange( new CoreRange( data->m_ovector[ count * 2 ] , data->m_ovector[ count * 2 + 1 ] ) );
   vm->retval( rng );
}


/*#
   @method grab Regex
   @brief Returns the part of a target string matched by this regular expression.
   @param string String where to scan for the pattern.
   @return The matching substring, or nil if the pattern doesn't match the string.

   Searches for the pattern and stores all the captured subexpressions in
   an array that is then returned. If the match is negative, returns nil.
*/
FALCON_FUNC Regex_grab( Falcon::VMachine *vm )
{
   CoreObject *self = vm->self().asObject();
   RegexCarrier *data = ( RegexCarrier *) self->getUserData();
   Item *source = vm->param(0);

   if( source == 0 || ! source->isString() )
   {
      throw new  ParamError( ErrorParam( e_inv_params, __LINE__ )
         .extra( "S" ) );
   }

   internal_regex_match( data, source->asString(), 0 );

   if ( data->m_matches == PCRE_ERROR_NOMATCH )
   {
      vm->retnil();
      return;
   }

   if ( data->m_matches < 0 )
   {
      String errVal = FAL_STR( re_msg_internal );
      errVal.writeNumber( (int64) data->m_matches );
      throw new RegexError( ErrorParam( FALRE_ERR_ERRMATCH, __LINE__ )
         .desc( FAL_STR( re_msg_errmatch ) )
         .extra( errVal ) );
   }

   // grab all the strings

   CoreArray *ca = new CoreArray();
   for( int32 capt = 0; capt < data->m_matches; capt++ )
   {

      String *grabbed = new CoreString(
            source->asString()->subString(
               data->m_ovector[ capt * 2 ], data->m_ovector[ capt * 2 + 1 ] )
               );
      ca->append( grabbed );
   }

   vm->retval( ca );
}

/*#
   @method compare Regex
   @brief Checks if a given strings can be matched by this expression.
   @param string A string.
   @return 0 if the string is matched by the regex pattern.

   This method overloads the BOM compare method, so that this Regex instance can be
   used in direct comparations. Switch tests and equality tests will succeed if the pattern
   matches agains the given string.
*/
FALCON_FUNC Regex_compare( Falcon::VMachine *vm )
{
   CoreObject *self = vm->self().asObject();
   RegexCarrier *data = ( RegexCarrier *) self->getUserData();
   Item *source = vm->param(0);

   if( source == 0 )
   {
      throw new  ParamError( ErrorParam( e_inv_params, __LINE__ ).
         extra( "X" ) );
   }

   int ovector[300];

   // If the source is a string, perform a non-recorded match
   if ( source->isString() )
   {
      bool match;
      String *str = source->asString();
      AutoCString src( *str );

      match = 0 < pcre_exec(
         data->m_pattern,
         data->m_extra,
         src.c_str(),
         src.length(),
         0,
         PCRE_NO_UTF8_CHECK,
         ovector,
         300 );

      if ( match )
         vm->retval( (int64) 0 ); // zero means compare ==
      else
         vm->retnil(); // we can't decide. Let the VM do that for us.
   }
   // Otherwise return nil, that will tell VM to use default matching algos
   else {
      vm->retnil();
   }
}

/*#
   @method version Regex
   @brief Returns the PCRE version used by this binding.
   @return A string containing a descriptive PCRE version message.

   This function can be used to retreive the PCRE version that is currently
   used by the REGEX module.
*/
FALCON_FUNC Regex_version( Falcon::VMachine *vm )
{
   const char *ver = pcre_version();
   vm->retval( new CoreString( ver, -1 ) );
}

/*#
   @class RegexError
   @brief Error generated by regular expression compilation and execution errors.
   @optparam code A numeric error code.
   @optparam description A textual description of the error code.
   @optparam extra A descriptive message explaining the error conditions.
   @from Error code, description, extra

   See the Error class in the core module.
*/

FALCON_FUNC  RegexError_init ( ::Falcon::VMachine *vm )
{
   CoreObject *einst = vm->self().asObject();
   if( einst->getUserData() == 0 )
      einst->setUserData( new RegexError );

   ::Falcon::core::Error_init( vm );
}

}
}

/* end of regex_ext.cpp */
