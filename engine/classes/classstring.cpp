/*
   FALCON - The Falcon Programming Language.
   FILE: classstring.cpp

   String type handler.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sat, 15 Jan 2011 19:09:07 +0100

   -------------------------------------------------------------------
   (C) Copyright 2011: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#undef SRC
#define SRC "engine/classes/classstring.cpp"

#include <falcon/classes/classstring.h>
#include <falcon/range.h>
#include <falcon/itemid.h>
#include <falcon/vmcontext.h>
#include <falcon/optoken.h>
#include <falcon/errors/accesserror.h>
#include <falcon/errors/operanderror.h>
#include <falcon/errors/paramerror.h>

#include <falcon/itemarray.h>
#include <falcon/function.h>

#include <falcon/datareader.h>
#include <falcon/datawriter.h>

namespace Falcon {

//===============================================================================
// Opcodes
//

class ClassString::PStepInitNext: public PStep
{
public:
   PStepInitNext() { apply = apply_; }
   virtual ~PStepInitNext() {}
   virtual void describeTo(String& tg) const { tg = "ClassString::PStepInitNext";}
   static void apply_( const PStep*, VMContext* vm );
};


void ClassString::PStepInitNext::apply_( const PStep*, VMContext* ctx )
{
   ctx->opcodeParam(1).asString()->copy( *ctx->topData().asString() );
   // remove the locally pushed data and the parameters.
   ctx->popData( 2 + ctx->currentCode().m_seqId );
   ctx->popCode();
}


class ClassString::PStepNextOp: public PStep
{
public:
   PStepNextOp( ClassString* owner ): m_owner(owner) { apply = apply_; }
   virtual ~PStepNextOp() {}
   virtual void describeTo(String& tg) const { tg = "ClassString::PStepNextOp";}
   static void apply_( const PStep*, VMContext* vm );

private:
   ClassString* m_owner;
};


void ClassString::PStepNextOp::apply_( const PStep* ps, VMContext* ctx )
{
   const PStepNextOp* step = static_cast<const PStepNextOp*>(ps);

   // The result of a deep call is in A
   Item* op1, *op2;

   ctx->operands( op1, op2 ); // we'll discard op2

   String* deep = op2->asString();
   String* self = op1->asString();

   ctx->popData();
   InstanceLock::Token* tk = step->m_owner->m_lock.lock(self);
   self->append( *deep );
   step->m_owner->m_lock.unlock(tk);
   ctx->popCode();
}


//=====================================================================
// Properties
//

static void get_len( const Class*, const String&, void* instance, Item& value )
{
   value = (int64) static_cast<String*>( instance )->length();
}

static void get_isText( const Class*, const String&, void* instance, Item& value )
{
   value.setBoolean( static_cast<String*>( instance )->isText() );
}

static void set_isText( const Class*, const String&, void* instance, const Item& value )
{
   String* str = static_cast<String*>( instance );
   if( str->isImmutable() )
   {
      throw new OperandError( ErrorParam( e_prop_ro, __LINE__, SRC ).extra("Immutable string") );
   }

   if( value.isTrue() ) {
      if( ! str->isText() ) {
         str->manipulator( str->manipulator()->bufferedManipulator() );
      }
   }
   else {
      str->toMemBuf();
   }
}

/*#
   @property charSize String
   @brief Returns or changes the size in bytes in this string.

   This properties control the count bytes per character (1, 2 or 4) used to store
   each character in this string.
*/


static void get_charSize( const Class*, const String&, void* instance, Item& value )
{
   String* str = static_cast<String*>(instance);
   value.setInteger( str->manipulator()->charSize() );
}

static void set_charSize( const Class*, const String&, void* instance, const Item& value )
{
   String* str = static_cast<String*>(instance);

   if( ! value.isOrdinal() )
   {
      throw new OperandError( ErrorParam( e_inv_params, __LINE__, SRC )
         .extra( "N" ) );
   }

   uint32 bpc = (uint32) value.isOrdinal();
   if ( ! str->setCharSize( bpc ) )
   {
      throw new  OperandError( ErrorParam( e_param_range, __LINE__, SRC ) );
   }
}


//=========================================================================
//
//

namespace _classString
{

/*#
   @method front String
   @brief Returns the first characters in a string.
   @param count Number of characters to be returned.
   @return The first elements or an empty string if the string is empty

   This method returns a string containing characters from the beginning of the string.

   @note When used statically, it takes a the target string as first parameter.
*/
FALCON_DECLARE_FUNCTION( front, "string:S,count:N" );
FALCON_DEFINE_FUNCTION_P1(front)
{
   String *str;
   Item* i_len;
   int64 len;

   if( ctx->isMethodic() )
   {
      str = ctx->self().asString();
      i_len = ctx->param(0);
      if( ! i_len->isOrdinal() ) throw paramError( __LINE__, SRC, true );
      len = i_len->forceInteger();
   }
   else {
      Item* i_str = ctx->param(0);
      i_len = ctx->param(1);

      if( ! i_str->isString() || ! i_len->isOrdinal() ) {
         throw paramError( __LINE__, SRC, true );
      }

      str = i_str->asString();
      len = i_len->forceInteger();
   }

   if ( len <= 0 || str->length() == 0 )
   {
      ctx->returnFrame( FALCON_GC_HANDLE(new String) );
   }
   else if ( ((length_t) len) >= str->length() ) {
      ctx->returnFrame( FALCON_GC_HANDLE(new String( *str )) );
   }
   else {
      ctx->returnFrame( FALCON_GC_HANDLE(new String( *str, 0, len )) );
   }
}

/*#
   @method back String
   @brief Returns the last characters in a string.
   @param count Number of characters to be returned.
   @return The last elements or an empty string if the string is empty

   This method returns a string containing characters from the end of the string.

   @note When used statically, it takes a the target string as first parameter.
*/
FALCON_DECLARE_FUNCTION( back, "back:S,count:N" );
FALCON_DEFINE_FUNCTION_P1(back)
{
   String *str;
   Item* i_len;
   int64 len;

   if( ctx->isMethodic() )
   {
      str = ctx->self().asString();
      i_len = ctx->param(0);
      if( ! i_len->isOrdinal() ) throw paramError( __LINE__, SRC, true );
      len = i_len->forceInteger();
   }
   else {
      Item* i_str = ctx->param(0);
      i_len = ctx->param(1);

      if( ! i_str->isString() || ! i_len->isOrdinal() ) {
         throw paramError( __LINE__, SRC, true );
      }

      str = i_str->asString();
      len = i_len->forceInteger();
   }

   if ( len <= 0 || str->length() == 0 )
   {
      ctx->returnFrame( FALCON_GC_HANDLE(new String) );
   }
   else if ( ((length_t) len) >= str->length() ) {
      ctx->returnFrame( FALCON_GC_HANDLE(new String( *str )) );
   }
   else {
      ctx->returnFrame( FALCON_GC_HANDLE(new String( *str, str->length()-len )) );
   }
}



/*#
   @method splittr String
   @brief Subdivides a string in an array of substrings given a token substring.
   @optparam token The token by which the string should be split.
   @optparam count Optional maximum split count.
   @return An array of strings containing the split string.

   This function returns an array of strings extracted from the given parameter.
   The array is filled with strings extracted from the first parameter, by dividing
   it based on the occurrences of the token substring. A count parameter may be
   provided to limit the splitting, so to take into consideration only the first
   relevant  tokens.  Adjacent matching tokens will be ignored.
   If no matches are possible, the returned array contains
   at worst a single element containing a copy of the whole string passed as a
   parameter.

   Differently to @a String.split, this function will "eat up" adjacent tokens. While
   @a strSplit is more adequate to parse field-oriented strings (as i.e.
   colon separated fields in configuration files) this function is best employed
   in word extraction.

   @note See @a Tokenizer for a more adequate function to scan extensively
   wide strings.

   @note When used statically, it takes a the target string as first parameter.
*/
FALCON_DECLARE_FUNCTION( splittr, "token:[S],count:[N]" );
FALCON_DEFINE_FUNCTION_P1(splittr)
{
   Item *target;
   Item *splitstr;
   Item *count;

   // Parameter checking;
   if( ctx->isMethodic() )
   {
      target = &ctx->self();
      splitstr = ctx->param(0);
      count = ctx->param(1);
   }
   else
   {
      target = ctx->param(0);
      splitstr = ctx->param(1);
      count = ctx->param(2);
   }

   uint32 limit;

   if ( target == 0 || ! target->isString()
        || (splitstr != 0 && ! (splitstr->isString() || splitstr->isNil()))
        || ( count != 0 && ! count->isOrdinal() ) )
   {
      throw paramError(__LINE__, SRC, ctx->isMethodic() );
   }

   limit = count == 0 ? 0xffffffff: (uint32) count->forceInteger();

   // Parameter extraction.
   String *tg_str = target->asString();
   uint32 tg_len = target->asString()->length();

   // split in chars?
   if( splitstr == 0 || splitstr->isNil() || splitstr->asString()->size() == 0 )
   {
      // split the string in an array.
      if( limit > tg_len )
         limit = tg_len;

      ItemArray* ca = new ItemArray( limit );
      for( uint32 i = 0; i + 1 < limit; ++i )
      {
         String* elem = new String(1);
         elem->append( tg_str->getCharAt(i) );
         ca->append( FALCON_GC_HANDLE(elem) );
      }
      // add remains if there are any
      if(limit <= tg_len)
         ca->append(tg_str->subString(limit - 1));

      ctx->returnFrame(FALCON_GC_HANDLE(ca));
      return;
   }

   String *sp_str = splitstr->asString();
   uint32 sp_len = splitstr->asString()->length();

   // return item.
   ItemArray *retarr = new ItemArray;

   // if the split string is empty, return empty string
   if ( sp_len == 0 )
   {
      retarr->append( FALCON_GC_HANDLE(new String()) );
      ctx->returnFrame(FALCON_GC_HANDLE(retarr));
      return;
   }

   // if the token is wider than the string, just return the string
   if ( tg_len < sp_len )
   {
      retarr->append( FALCON_GC_HANDLE(new String( *tg_str )) );
      ctx->returnFrame(FALCON_GC_HANDLE(retarr));
      return;
   }

   uint32 pos = 0;
   uint32 last_pos = 0;
   bool lastIsEmpty = false;
   // scan the string
   while( limit > 1 && pos <= tg_len - sp_len  )
   {
      uint32 sp_pos = 0;
      // skip matching pattern-
      while ( tg_str->getCharAt( pos ) == sp_str->getCharAt( sp_pos ) &&
              sp_pos < sp_len && pos <= tg_len - sp_len ) {
         sp_pos++;
         pos++;
      }

      // a match?
      if ( sp_pos == sp_len ) {
         // put the item in the array.
         uint32 splitend = pos - sp_len;
         retarr->append( FALCON_GC_HANDLE(new String( String( *tg_str, last_pos, splitend ) ) ) );

         lastIsEmpty = (last_pos >= splitend);

         last_pos = pos;
         limit--;
         // skip matching pattern
         while( sp_pos == sp_len && pos <= tg_len - sp_len ) {
            sp_pos = 0;
            last_pos = pos;
            while ( tg_str->getCharAt( pos ) == sp_str->getCharAt( sp_pos )
                    && sp_pos < sp_len && pos <= tg_len - sp_len ) {
               sp_pos++;
               pos++;
            }
         }
         pos = last_pos;

      }
      else
         pos++;
   }

   // Residual element?
   // -- but only if we didn't already put a "" in the array
   if ( limit >= 1 && ! lastIsEmpty ) {
      retarr->append( FALCON_GC_HANDLE(new String( String( *tg_str, last_pos, (uint32) tg_len ) ) ) );
   }

   ctx->returnFrame(FALCON_GC_HANDLE(retarr));
}


/*#
   @method split String
   @brief Subdivides a string in an array of substrings given a token substring.
   @optparam token The token by which the string should be split.
   @optparam count Optional maximum split count.
   @return An array of strings containing the split string.

   This function returns an array of strings extracted from the given parameter.
   The array is filled with strings extracted from the first parameter, by dividing
   it based on the occurrences of the token substring. A count parameter may be
   provided to limit the splitting, so to take into consideration only the first
   relevant  tokens.  Adjacent matching tokens will cause the returned array to
   contains empty strings. If no matches are possible, the returned array contains
   at worst a single element containing a copy of the whole string passed as a
   parameter.

   For example, the following may be useful to parse a INI file where keys are
   separated from values by "=" signs:

   @code
   key, value = strSplit( string, "=", 2 )
   @endcode

   This code would return an array of 2 items at maximum; if no "=" signs are found
   in string, the above code would throw an error because of unpacking size, a
   thing that may be desirable in a parsing code. If there are more than one "=" in
   the string, only the first starting from left is considered, while the others
   are returned in the second item, unparsed.

   If the @b token is empty or not given, the string is returned as a sequence of
   1-character strings in an array.


   @note When used statically, it takes a the target string as first parameter.
*/

FALCON_DECLARE_FUNCTION( split, "token:[S],count:[N]" );
FALCON_DEFINE_FUNCTION_P1(split)

{
   Item *target;
   Item *splitstr;
   Item *count;

   // Parameter checking;
   if( ctx->isMethodic() )
   {
      target = &ctx->self();
      splitstr = ctx->param(0);
      count = ctx->param(1);
   }
   else
   {
      target = ctx->param(0);
      splitstr = ctx->param(1);
      count = ctx->param(2);
   }


   if ( (target == 0 || ! target->isString())
        || (splitstr != 0 && ! (splitstr->isString() || splitstr->isNil()))
        || ( count != 0 && ! count->isOrdinal() ) )
   {
      throw paramError(__LINE__, SRC, ctx->isMethodic() );
   }

   // Parameter extraction.
   String *tg_str = target->asString();
   uint32 tg_len = target->asString()->length();
   uint32 limit = count == 0 ? 0xffffffff: (uint32) count->forceInteger();

   // split in chars?
   if( splitstr == 0 || splitstr->isNil() || splitstr->asString()->size() == 0)
   {
      // split the string in an array.
      if( limit > tg_len )
         limit = tg_len;

      ItemArray* ca = new ItemArray( limit );
      for( uint32 i = 0; i+1 < limit; ++i )
      {
         String* elem = new String(1);
         elem->append( tg_str->getCharAt(i) );
         ca->append( FALCON_GC_HANDLE(elem) );
      }
      // add remains if there are any
      if(limit <= tg_len)
         ca->append(tg_str->subString(limit - 1));

      ctx->returnFrame( FALCON_GC_HANDLE(ca) );
      return;
   }

   String *sp_str = splitstr->asString();
   uint32 sp_len = sp_str->length();

   // return item.
   ItemArray *retarr = new ItemArray;

   // if the split string is empty, return empty string
   if ( sp_len == 0 )
   {
      retarr->append( FALCON_GC_HANDLE(new String()) );
      ctx->returnFrame( FALCON_GC_HANDLE(retarr) );
      return;
   }

   // if the token is wider than the string, just return the string
   if ( tg_len < sp_len )
   {
      retarr->append( FALCON_GC_HANDLE(new String( *tg_str ) ) );
      ctx->returnFrame( FALCON_GC_HANDLE(retarr) );
      return;
   }

   uint32 pos = 0;
   uint32 last_pos = 0;
   // scan the string
   while( limit > 1 && pos <= tg_len - sp_len  )
   {
      uint32 sp_pos = 0;
      // skip matching pattern-
      while ( tg_str->getCharAt( pos ) == sp_str->getCharAt( sp_pos ) &&
              sp_pos < sp_len && pos <= tg_len - sp_len ) {
         sp_pos++;
         pos++;
      }

      // a match?
      if ( sp_pos == sp_len ) {
         // put the item in the array.
         uint32 splitend = pos - sp_len;
         retarr->append( FALCON_GC_HANDLE(new String( *tg_str, last_pos, splitend )) );
         last_pos = pos;
         limit--;

      }
      else
         pos++;
   }

   // Residual element?
   if ( limit >= 1 || last_pos < tg_len ) {
      uint32 splitend = tg_len;
      retarr->append( FALCON_GC_HANDLE(new String( *tg_str, last_pos, splitend )) );
   }

   ctx->returnFrame( FALCON_GC_HANDLE(retarr) );
}



/*#
   @method merge String
   @brief Merges an array of strings into one.
   @param array An array of strings to be merged.
   @optparam separator The separator used to merge the strings.
   @optparam count Maximum count of merges.
   @return The merged string.

   The method will return a new string containing the concatenation
   of the strings inside the array parameter. If the array is empty,
   an empty string is returned. If a mergeStr parameter is given, it
   is added to each pair of string merged; mergeStr is never added at
   the end of the new string. A count parameter may be specified to
   limit the number of elements merged in the array.

   The function may be used in this way:
   @code
   a = " ".merge( [ "a", "string", "of", "words" ] )
   printl( a ) // prints "a string of words"
   @endcode

   Or, using the function statically from the class:

   @code
   a = String.merge( " ", [ "a", "string", "of", "words" ] )
   printl( a ) // prints "a string of words"
   @endcode

   If an element of the array is not a string, an error is raised.

   @note When used statically, it takes a the target string as first parameter.
*/

FALCON_DECLARE_FUNCTION( merge, "separator:[S],array:[S],count:[N]" );
FALCON_DEFINE_FUNCTION_P1(merge)
{
   Item *mergestr, *source, *count;

   // Parameter checking;
   if( ctx->isMethodic() )
   {
      mergestr = &ctx->self();
      source = ctx->param(0);
      count = ctx->param(1);
   }
   else {
      mergestr = ctx->param(0);
      source = ctx->param(1);
      count = ctx->param(2);
   }
   uint64 limit;

   if ( source == 0 || ! source->isArray()
        || ( mergestr != 0 && ! mergestr->isString() )
        || ( count != 0 && ! count->isNil() && ! count->isOrdinal() ) )
   {
      throw paramError(__LINE__,SRC, ctx->isMethodic() );
   }

   // Parameter estraction.
   limit = count == 0 || count->isNil() ? 0xffffffff: count->forceInteger();

   String* mr_str = mergestr->asString();
   ItemArray *elements = source->asArray();
   uint32 len = elements->length();
   if ( limit < len )
      len = (uint32) limit;

   String *ts = new String;

   // filling the target.
   for( uint32 i = 1; i <= len ; i ++ )
   {
      Item* item = &elements->at(i-1);

      if ( item->isString() )
         *ts += *item->asString();
      else
      {
         delete ts;
         throw FALCON_SIGN_XERROR( ParamError, e_param_type, .extra("Need to be string") );
      }

      if ( mr_str != 0 && i < len )
         ts->append( *mr_str );

      ts->reserve( len/i * ts->size() );
   }

   ctx->returnFrame( FALCON_GC_HANDLE(ts) );
}


/*#
   @method join String
   @brief Joins the parameters into a new string.
   @param ...
   @return A new string created joining the parameters, left to right.

   If this string is not empty, it is copied between each joined string.

   For example, the next code separates each value with ", "

   @code
   > ", ".join( "have", "a", "nice", "day" )
   @endcode

   If the parameters are not string, a standard @a toString conversion is tried.

   @note When used statically, it takes a the target string as first parameter.
*/

FALCON_DECLARE_FUNCTION( join, "separator:S,..." );
FALCON_DEFINE_FUNCTION_P1(join)
{
   Item *mergestr;
   int32 first;

   // Parameter checking;
   if( ctx->isMethodic() )
   {
      mergestr = &ctx->self();
      first = 0;
   }
   else {
      mergestr = ctx->param(0);
      first = 1;
   }

   if ( mergestr != 0 && ! mergestr->isString() )
   {
      throw paramError(__LINE__, SRC, ctx->isMethodic());
   }

   String* mr_str = mergestr->asString();
   int32 len = ctx->paramCount();
   String *ts = new String;

   // filling the target.
   for( int32 i = 1+first; i <= len ; i ++ )
   {
      Item* item = ctx->param(i-1);

      if ( item->isString() )
      {
         *ts += *item->asString();
      }
      else
      {
         delete ts;
         throw FALCON_SIGN_XERROR( ParamError, e_param_type, .extra("Parameters must be strings") );
      }

      if ( i < len )
      {
         ts->append( *mr_str );
      }

      ts->reserve( len/i * ts->size() );
   }

   ctx->returnFrame( FALCON_GC_HANDLE(ts) );
}

static void internal_find( VMContext* ctx, Function* func, bool mode )
{
   // Parameter checking;
   Item *target;
   Item *needle;
   Item *start_item;
   Item *end_item;

   if ( ctx->isMethodic() )
   {
      target = &ctx->self();
      needle = ctx->param(0);
      start_item = ctx->param(1);
      end_item = ctx->param(2);
   }
   else
   {
      target = ctx->param(0);
      needle = ctx->param(1);
      start_item = ctx->param(2);
      end_item = ctx->param(3);
   }

   if ( target == 0 || ! target->isString()
        || needle == 0 || ! needle->isString()
        || (start_item != 0 && ! start_item->isOrdinal())
        || (end_item != 0 && ! end_item->isOrdinal())
        )
   {
      throw func->paramError(__LINE__, SRC, ctx->isMethodic());
   }

   int64 start = start_item == 0 ? 0 : start_item->forceInteger();
   int64 end = end_item == 0 ? -1 : end_item->forceInteger();
   String *sTarget = target->asString();

   int64 len = sTarget->length();
   // negative? -- fix
   if ( start < 0 ) end = len + start + 1;

   // out of range?
   if ( start < 0 || start >= len )
   {
      ctx->returnFrame( -1 );
      return;
   }

   if ( end < 0 ) end = len + end+1;
   // again < than 0? -- it's out of range.
   if ( end < 0 )
   {
      ctx->returnFrame( -1 );
      return;
   }

   if( end > len ) end = len;

   uint32 pos = mode ?
            target->asString()->rfind( *needle->asString(), (uint32) start, (uint32) end ) :
            target->asString()->find( *needle->asString(), (uint32) start, (uint32) end );

   if ( pos != csh::npos )
      ctx->returnFrame( (int64)pos );
   else
      ctx->returnFrame( -1 );

}

/*#
   @method find String
   @brief Finds a substring.
   @param needle Substring to search for.
   @optparam start Optional position from which to start the search in string.
   @optparam end Optional position at which to end the search in string.
   @return The position where the substring is found, or -1.

   Returns the index in string were needle begins, or -1 if not present. Giving a
   start parameter will cause the search to start from the given position up to the
   end of the string; if a match can be made at start position, then the the
   returned value will be the same as start, so when repeating searches in a string
   for all the possible matches, repeat until the result is -1 by adding one to the
   returned value and using it as start position for the next search.

   If an end position is given, it is used as upper limit for the search, so that
   the search is in the interval [start, end-1].

   @note When used statically, it takes a the target string as first parameter.
*/
FALCON_DECLARE_FUNCTION( find, "string:S,needle:S,start:[N],end:[N]" );
FALCON_DEFINE_FUNCTION_P1(find)
{
   internal_find( ctx, this, false );
}

/*#
   @method rfind String
   @brief Finds a substring backwards.
   @param needle Substring to search for.
   @optparam start Optional position from which to start the search in string.
   @optparam end Optional position at which to end the search in string.
   @return The position where the substring is found, or -1.

   Works exactly as @a String.find, except for the fact that the last match
   in the string (or in the specified interval) is returned.

   @note When used statically, it takes a the target string as first parameter.
*/
FALCON_DECLARE_FUNCTION( rfind, "string:S,needle:S,start:[N],end:[N]" );
FALCON_DEFINE_FUNCTION_P1(rfind)

{
   internal_find( ctx, this, true );
}


static void internal_trim( String::t_trimmode mode, Function* func, VMContext* ctx, bool inPlace )
{
   String *self;
   Item *trimChars;

   if ( ctx->isMethodic() )
   {
      self = ctx->self().asString();
      trimChars = ctx->param(0);
   }
   else
   {
      Item *i_str = ctx->param( 0 );
      if ( i_str == 0 || ! i_str->isString() )
      {
         throw func->paramError(__LINE__, SRC, ctx->isMethodic());
      }

      self = i_str->asString();
      trimChars = ctx->param(1);
   }

   if ( trimChars != 0 && trimChars->isArray() && ! trimChars->isString() )
   {
      throw func->paramError(__LINE__, SRC, ctx->isMethodic());
   }

   ClassString* cstring = static_cast<ClassString*>(func->methodOf());

   InstanceLock::Token* tk;
   String *cs;
   if( inPlace )
   {
      if( self->isImmutable() )
      {
         throw FALCON_SIGN_XERROR( ParamError, e_param_type, .extra("Immutable string") );
      }
      tk = cstring->lockInstance( self );
      cs = self;
   }
   else {
      tk = cstring->lockInstance( self );
      cs = new String( *self );
      FALCON_GC_HANDLE( cs );
      cstring->unlockInstance( tk );
      tk = 0;
   }

   if ( trimChars == 0 || trimChars->isNil() ) {
      cs->trim( mode );
   }
   else
   {
      cs->trimFromSet( mode, *trimChars->asString() );
   }

   if( tk != 0 )
   {
      cstring->unlockInstance( tk );
   }

   ctx->returnFrame( Item(cs->handler(), cs) );

}


/*#
   @method trim String
   @brief Trims whitespaces from both ends of a string.
   @optparam trimSet A set of characters that must be removed.
   @return The trimmed version of the string.

   A new string, which is a copy of the original one with all characters in @b trimSet
   at both ends of the string removed, is returned. If @b trimSet is not supplied, it
   defaults to space, tabulation characters, new lines and carriage returns. The
   original string is unmodified.

   @note When used statically, it takes a the target string as first parameter.
*/
FALCON_DECLARE_FUNCTION( trim, "trimSet:[S]" );
FALCON_DEFINE_FUNCTION_P1(trim)
{
   internal_trim( String::e_tm_all, this, ctx, false);
}

/*#
   @method ftrim String
   @brief Trims front whitespaces in a string.
   @optparam trimSet A set of characters that must be removed.
   @return The trimmed version of the string.

   A new string, which is a copy of the original one with all characters in @b trimSet
   at the beginning of the string removed, is returned. If @b trimSet is not supplied, it
   defaults to space, tabulation characters, new lines and carriage returns. The
   original string is unmodified.

   @note When used statically, it takes a the target string as first parameter.
*/

FALCON_DECLARE_FUNCTION( ftrim, "trimSet:[S]" );
FALCON_DEFINE_FUNCTION_P1( ftrim )
{
   internal_trim(String::e_tm_front, this, ctx, false);
}

/*#
   @method rtrim String
   @brief Trims trailing whitespaces in a string.
   @optparam trimSet A set of characters that must be removed.
   @return The trimmed version of the string.

   A new string, which is a copy of the original one with all characters in @b trimSet
   at the end of the string removed, is returned. If @b trimSet is not supplied, it
   defaults to space, tabulation characters, new lines and carriage returns. The
   original string is unmodified.

   @note When used statically, it takes a the target string as first parameter.
*/

FALCON_DECLARE_FUNCTION( rtrim, "trimSet:[S]" );
FALCON_DEFINE_FUNCTION_P1( rtrim )
{
   internal_trim(String::e_tm_back, this, ctx, false);
}


//=======================================================================================
// Mutable Strings
//


/*#
   @method atrim String
   @brief Trims whitespaces from both ends of a string in place.
   @optparam trimSet A set of characters that must be removed.
   @return The same string.

   This method removes all characters in @b trimSet at the beginning and
   at the end of the string.
   If @b trimSet is not supplied, it defaults to space, tabulation characters,
   new lines and carriage returns.

   @note When used statically, it takes a the target string as first parameter.
*/
FALCON_DECLARE_FUNCTION( atrim, "trimSet:[S]" );
FALCON_DEFINE_FUNCTION_P1(atrim)
{
   internal_trim( String::e_tm_all, this, ctx, true);
}

/*#
   @method aftrim String
   @brief Trims front whitespaces in a string in place.
   @optparam trimSet A set of characters that must be removed.
   @return This string.

   This method removes all characters in @b trimSet at the beginning of the string.
   If @b trimSet is not supplied, it
   defaults to space, tabulation characters, new lines and carriage returns.

   @note When used statically, it takes a the target string as first parameter.
*/

FALCON_DECLARE_FUNCTION( aftrim, "trimSet:[S]" );
FALCON_DEFINE_FUNCTION_P1( aftrim )
{
   internal_trim(String::e_tm_front, this, ctx, true);
}

/*#
   @method rtrim String
   @brief Trims trailing whitespaces in a string.
   @optparam trimSet A set of characters that must be removed.
   @return The trimmed version of the string.

   This method removes all characters in @b trimSet at the end of the string.
   If @b trimSet is not supplied, it
   defaults to space, tabulation characters, new lines and carriage returns.

   @note When used statically, it takes a the target string as first parameter.
*/

FALCON_DECLARE_FUNCTION( artrim, "trimSet:[S]" );
FALCON_DEFINE_FUNCTION_P1( artrim )
{
   internal_trim(String::e_tm_back, this, ctx, true);
}


/*#
   @method fill String
   @brief Fills a string with a given character or substring.
   @Prime chr The character (unicode value) or substring used to refill this string.
   @return The string itself.

   This method fills the physical storage of the given string with a single
   character or a repeated substring. This can be useful to clean a string used repeatedly
   as input buffer.

   @note When used statically as a class method, the first parameter can be a mutable string.
*/

FALCON_DECLARE_FUNCTION( fill, "target:MString,chr:N|S" );
FALCON_DEFINE_FUNCTION_P1(fill)
{
   Item *i_string;
   Item *i_chr;

   // Parameter checking;
   if ( ctx->isMethodic() )
   {
      i_string = &ctx->self();
      i_chr = ctx->param(0);
   }
   else
   {
      i_string = ctx->param(0);
      i_chr = ctx->param(1);
   }

   if( i_string == 0 || ! i_string->asClass()->isDerivedFrom( methodOf() )
      || i_chr == 0 || ( ! i_chr->isOrdinal() && !i_chr->isString())
      )
   {
      throw paramError(__LINE__,SRC, ctx->isMethodic() );
   }

   String *string = i_string->asString();

   if( string->isImmutable() )
   {
      throw new OperandError( ErrorParam( e_prop_ro, __LINE__, SRC ).extra("Immutable string") );
   }

   if ( i_chr->isOrdinal() )
   {
      uint32 chr = (uint32) i_chr->forceInteger();
      for( uint32 i = 0; i < string->length(); i ++ )
      {
         string->setCharAt( i, chr );
      }
   }
   else
   {
      String* rep = i_chr->asString();

      if ( rep->length() == 0 )
      {
          throw new ParamError( ErrorParam( e_param_range, __LINE__ )
            .extra( "Empty fill character" ) );
      }

      uint32 pos = 0;
      uint32 pos2 = 0;
      while( pos < string->length() )
      {
         string->setCharAt( pos++, rep->getCharAt( pos2++ ) );
         if ( pos2 >= rep->length() )
         {
            pos2 = 0;
         }
      }
   }

   ctx->returnFrame( Item(methodOf(), string) );
}
}

//
// Class properties used for enumeration
//

ClassString::ClassString():
   Class( "String", FLC_CLASS_ID_STRING )
{
   init();
}


ClassString::ClassString( const String& subname ):
         Class( subname, FLC_CLASS_ID_STRING )
{
   init();
}


void ClassString::init()
{
   m_initNext = new PStepInitNext;
   m_nextOp = new PStepNextOp(this);

   addProperty( "isText", &get_isText, &set_isText );
   addProperty( "len", &get_len );
   addProperty( "charSize", &get_charSize, &set_charSize );

   addMethod( new _classString::Function_front, true );
   addMethod( new _classString::Function_back, true );
   addMethod( new _classString::Function_split, true );
   addMethod( new _classString::Function_splittr, true );
   addMethod( new _classString::Function_merge, true );
   addMethod( new _classString::Function_join, true );

   addMethod( new _classString::Function_find, true );
   addMethod( new _classString::Function_rfind, true );

   addMethod( new _classString::Function_trim, true );
   addMethod( new _classString::Function_atrim, true );
   addMethod( new _classString::Function_ftrim, true );
   addMethod( new _classString::Function_aftrim, true );
   addMethod( new _classString::Function_rtrim, true );
   addMethod( new _classString::Function_artrim, true );

   addMethod( new _classString::Function_fill, true );
}


ClassString::~ClassString()
{
   delete m_initNext;
   delete m_nextOp;
}

int64 ClassString::occupiedMemory( void* instance ) const
{
   /* NO LOCK */
   String* s = static_cast<String*>( instance );
   return sizeof(String) + s->allocated() + 16 + (s->allocated()?16:0);
}


void ClassString::dispose( void* self ) const
{
   /* NO LOCK */
   delete static_cast<String*>( self );
}


void* ClassString::clone( void* source ) const
{
   String* temp;
   {
      InstanceLock::Locker( &m_lock, source );
      temp = new String( *( static_cast<String*>( source ) ) );
   }

   return temp;
}

void* ClassString::createInstance() const
{
   return new String;
}

void ClassString::store( VMContext*, DataWriter* dw, void* data ) const
{
#ifdef FALCON_MT_UNSAFE
   String& value = *static_cast<String*>( data );
   TRACE2( "ClassString::store -- (unsafe) \"%s\"", value.c_ize() );
#else
   String* orig = static_cast<String*>( data );
   InstanceLock::Token* tk = m_lock.lock(data);
   String value(*orig);
   value.setImmutable(orig->isImmutable());
   m_lock.unlock(tk);

   TRACE2( "ClassString::store -- \"%s\"", value.c_ize() );
#endif

   dw->write( value );
}


void ClassString::restore( VMContext* ctx, DataReader* dr ) const
{
   String* str = new String;

   try
   {
      dr->read( *str );
      TRACE2( "ClassString::restore -- \"%s\"", str->c_ize() );
      ctx->pushData( Item( this, str ) );
   }
   catch( ... )
   {
      delete str;
      throw;
   }
}


void ClassString::describe( void* instance, String& target, int, int maxlen ) const
{
#ifdef FALCON_MT_UNSAFE
   String* self = static_cast<String*>( instance );
#else
   InstanceLock::Token* tk = m_lock.lock(instance);
   String copy( *static_cast<String*>( instance ) );
   m_lock.unlock(tk);

   String* self = &copy;
#endif

   target.size( 0 );

   if( self->isText() )
   {
      String escaped;
      self->escape(escaped);

      if ( ! static_cast<String*>( instance )->isImmutable() ){
         target.append("m");
      }

      target.append( '"' );
      if ( (int) self->length() > maxlen && maxlen > 0 )
      {
         target.append( escaped.subString( 0, maxlen ) );
         target.append( "..." );
      }
      else
      {
         target.append( escaped );
      }
      target.append( '"' );
   }
   else {
      target.append( "m{" );

      length_t pos = 0;
      byte* data = self->getRawStorage();
      while( pos < self->size() && (maxlen <=0 || pos*3 < (unsigned int) maxlen) ) {
         if( pos > 0 ) target.append(' ');
         if( data[pos] < 16 )
         {
            target.append('0');
         }
         target.writeNumberHex( data[pos], true );
         ++pos;
      }

      target.append( '}' );
   }
}


void ClassString::gcMarkInstance( void* instance, uint32 mark ) const
{
   /* NO LOCK */
   static_cast<String*>( instance )->gcMark( mark );
}


bool ClassString::gcCheckInstance( void* instance, uint32 mark ) const
{
   /* NO LOCK */
   return static_cast<String*>( instance )->currentMark() >= mark;
}


//=======================================================================
// Addition

void ClassString::op_add( VMContext* ctx, void* self ) const
{
   String* str = static_cast<String*>( self );

   Item* op1, *op2;

   ctx->operands( op1, op2 );

   Class* cls;
   void* inst;

   if ( ! op2->asClassInst( cls, inst ) )
   {
      InstanceLock::Token* tk = m_lock.lock(str);
      String* copy = new String( *str );
      m_lock.unlock(tk);

      copy->append( op2->describe() );

      ctx->stackResult( 2, FALCON_GC_HANDLE_IN(ctx,copy) );

      return;
   }

   if ( cls->typeID() == typeID() )
   {
      // it's a string!
      InstanceLock::Token* tk = m_lock.lock(str);
      String* copy = new String( *str );
      m_lock.unlock(tk);

      tk = m_lock.lock(inst);
      copy->append( *static_cast<String*>( inst ) );
      m_lock.unlock(tk);

      ctx->stackResult( 2, FALCON_GC_HANDLE_IN(ctx, copy) );

      return;
   }

   // else we surrender, and we let the virtual system to find a way.
   ctx->pushCode( m_nextOp );

   // this will transform op2 slot into its string representation.
   cls->op_toString( ctx, inst );

   if ( ! ctx->wentDeep( m_nextOp ) )
   {
      ctx->popCode();

      // op2 has been transformed
      String* deep = (String*)op2->asInst();

      InstanceLock::Token* tk = m_lock.lock(str);
      deep->prepend( *str );
      m_lock.unlock(tk);
   }
}

//=======================================================================
// Operands
//

bool ClassString::op_init( VMContext* ctx, void* instance, int pcount ) const
{
   String* self = static_cast<String*>(instance);

   // no param?
   if ( pcount > 0 )
   {
      // the parameter is a string?
      Item* itm = ctx->opcodeParams( pcount );

      if ( itm->isString() )
      {
         // copy it.
         self->copy( *itm->asString() );
      }
      else
      {
         if( pcount > 1 ) {
            ctx->popData( pcount-1 );
            // just to be sure.
            itm = &ctx->topData();
         }

         // apply the op_toString on the item.
         ctx->pushCode( m_initNext );
         ctx->currentCode().m_seqId = pcount;
         long depth = ctx->codeDepth();

         // first get the required data...
         Class* cls;
         void* data;
         itm->forceClassInst( cls, data );

         // then ensure that the stack is as we need.
         ctx->pushData( *self );
         ctx->pushData( *itm );

         // and finally invoke stringify operation.
         cls->op_toString( ctx, data );
         if( depth == ctx->codeDepth() )
         {
            // we can get the string here.
            fassert( ctx->topData().isString() );
            fassert( ctx->opcodeParam(1).isString() );

            String* result = ctx->topData().asString();
            ctx->opcodeParam(1).asString()->copy( *result );

            // and clean the stack
            ctx->popData(2 + pcount);
            ctx->popCode();
         }

         // we took care of the stack.
         return true;
      }
   }

   return false;
}

//===============================================================
//

void ClassString::op_mul( VMContext* ctx, void* instance ) const
{
   // self count => new
   Item& i_count = ctx->topData();
   if( ! i_count.isOrdinal() )
   {
      throw new OperandError( ErrorParam( e_op_params, __LINE__ ).extra( "N" ) );
   }

   int64 count = i_count.forceInteger();
   ctx->popData();
   if( count == 0 )
   {
      ctx->topData() = FALCON_GC_HANDLE(new String);
      return;
   }

   String* self = static_cast<String*>(instance);
   InstanceLock::Token* tk = m_lock.lock(self);

   String copy(*self);
   m_lock.unlock(tk);

   String* result = new String;
   result->reserve(copy.size() * count);
   for( int64 i = 0; i < count; ++i )
   {
      result->append(copy);
   }

   ctx->topData() = FALCON_GC_HANDLE(result);
}


void ClassString::op_div( VMContext* ctx, void* instance ) const
{
   // self count => new
   Item& i_count = ctx->topData();
   if( ! i_count.isOrdinal() )
   {
      throw new OperandError( ErrorParam( e_op_params, __LINE__ ).extra( "N" ) );
   }

   int64 count = i_count.forceInteger();
   ctx->popData();

   if ( count < 0 || count >= 0xFFFFFFFFLL )
   {
      throw new OperandError( ErrorParam( e_op_params, __LINE__ ).extra( "out of range" ) );
   }

   String* self = static_cast<String*>(instance);
   InstanceLock::Token* tk = m_lock.lock(self);
   String* target = new String(*self);
   target->append((char_t) count);
   ctx->topData() = FALCON_GC_HANDLE( target );
   m_lock.unlock(tk);
}


void ClassString::op_getIndex( VMContext* ctx, void* self ) const
{
   Item *index, *stritem;

   ctx->operands( stritem, index );

   String& str = *static_cast<String*>( self );

   if ( index->isOrdinal() )
   {
      int64 v = index->forceInteger();
      uint32 chr = 0;

      {
         InstanceLock::Locker( &m_lock, &str );

         if ( v < 0 ) v = str.length() + v;

         if ( v >= str.length() )
         {
            throw new AccessError( ErrorParam( e_arracc, __LINE__ ).extra( "index out of range" ) );
         }

         chr = str.getCharAt( (length_t) v );
      }

      if( str.isText() ) {
         String *s = new String();
         s->append( chr );
         ctx->stackResult( 2, FALCON_GC_HANDLE_IN(ctx,s) );
      }
      else {
         ctx->stackResult(2, Item((int64) chr) );
      }
   }
   else if ( index->isUser() ) // index is a range
   {
      // if range is moving from a smaller number to larger (start left move right in the array)
      //      give values in same order as they appear in the array
      // if range is moving from a larger number to smaller (start right move left in the array)
      //      give values in reverse order as they appear in the array

      Class *cls;
      void *udata;
      index->forceClassInst( cls, udata );

      // Confirm we have a range
      if ( cls->typeID() != FLC_CLASS_ID_RANGE )
      {
         throw new OperandError( ErrorParam( e_op_params, __LINE__ ).extra( "Range" ) );
      }

      Range& rng = *static_cast<Range*>( udata );

      int64 step = ( rng.step() == 0 ) ? 1 : rng.step(); // assume 1 if no step given
      int64 start = rng.start();
      int64 end = rng.end();

      bool reverse = false;
      String *s = new String();

      {
         InstanceLock::Locker( &m_lock, &str );
         int64 strLen = str.length();

         // do some validation checks before proceeding
         if ( start >= strLen || start < ( strLen * -1 )  || end > strLen || end < ( strLen * -1 ) )
         {
            delete s;
            throw new AccessError( ErrorParam( e_arracc, __LINE__ ).extra( "index out of range" ) );
         }

         if ( rng.isOpen() )
         {
            // If negative number count from the end of the array
            if ( start < 0 ) start = strLen + start;

            end = strLen;
         }
         else // non-open range
         {
            if ( start < 0 ) start = strLen + start;

            if ( end < 0 ) end = strLen + end;

            if ( start > end )
            {
               reverse = true;
               if ( rng.step() == 0 ) step = -1;
            }
         }

         if ( reverse )
         {
            while ( start >= end )
            {
               s->append( str.getCharAt( (length_t) start ) );
               start += step;
            }
         }
         else
         {
            while ( start < end )
            {
               s->append( str.getCharAt( (length_t) start ) );
               start += step;
            }
         }

         if( ! str.isText() )
         {
            s->toMemBuf();
         }
      }

      ctx->stackResult( 2, FALCON_GC_HANDLE_IN(ctx,s) );
   }
   else
   {
      throw new AccessError( ErrorParam( e_arracc, __LINE__ ).extra( "invalid index" ) );
   }
}


//=======================================================================
// Comparison
//

void ClassString::op_compare( VMContext* ctx, void* self ) const
{
   Item* op1, *op2;

   OpToken token( ctx, op1, op2 );

   String* string = static_cast<String*>( self );

   Class* otherClass;
   void* otherData;

   if ( op2->asClassInst( otherClass, otherData ) )
   {
      if ( otherClass->typeID() == typeID() )
      {
         token.exit( string->compare(*static_cast<String*>( otherData ) ) );
      }
      else
      {
         token.exit( typeID() - otherClass->typeID() );
      }
   }
   else
   {
      token.exit( typeID() - op2->type() );
   }
}


void ClassString::op_toString( VMContext* ctx, void* data ) const
{
   // this op is generally called for temporary items,
   // ... so, even if we shouldn't be marked,
   // ... we won't be marked long if we're temporary.
   ctx->topData().setUser( this, data );
}


void ClassString::op_isTrue( VMContext* ctx, void* str ) const
{
   /* No lock -- we can accept sub-program level uncertainty */
   ctx->topData().setBoolean( static_cast<String*>( str )->size() != 0 );
}


void ClassString::op_in( VMContext* ctx, void* instance ) const
{
   if( ! ctx->topData().isString() )
   {
      throw new OperandError( ErrorParam( e_op_params, __LINE__ ).extra( "S" ) );
      return;
   }


   /* No lock -- we can accept sub-program level uncertainty */
   String* self = static_cast<String*>(instance);
   String* other = ctx->topData().asString();
   ctx->popData();
   if( other == self )
   {
      ctx->topData().setBoolean(true);
      return;
   }

   InstanceLock::Token* l1 = m_lock.lock(self);
   InstanceLock::Token* l2 = m_lock.lock(other);

   length_t pos = self->find(*other);

   m_lock.unlock(l2);
   m_lock.unlock(l1);

   ctx->topData().setBoolean( pos != String::npos );
}

void ClassString::op_iter( VMContext* ctx, void* self ) const
{
   /* No lock -- we can accept sub-program level uncertainty */
   length_t size = static_cast<String*>( self )->size();
   if( size == 0 ) {
      ctx->pushData(Item()); // we should not loop
   }
   else
   {
      ctx->pushData(Item(0));
   }
}


void ClassString::op_next( VMContext* ctx, void* instance ) const
{
   length_t pos = (length_t) ctx->topData().asInteger();

   String* self = static_cast<String*>(instance);
   InstanceLock::Token* tk = m_lock.lock(self);
   char_t chr = self->getCharAt(pos);
   ++pos;
   bool isLast = self->length() <= pos;
   m_lock.unlock(tk);

   ctx->topData().setInteger(pos);
   String* schr = new String;
   schr->append(chr);
   ctx->pushData( FALCON_GC_HANDLE(schr));
   if( ! isLast ) ctx->topData().setDoubt();
}

//========================================================================================
// Mutable operators
//

void ClassString::op_aadd( VMContext* ctx, void* self ) const
{
   String* str = static_cast<String*>( self );
   if( str->isImmutable() )
   {
      throw new OperandError( ErrorParam( e_prop_ro, __LINE__, SRC ).extra("Immutable string") );
   }

   Item* op1, *op2;
   ctx->operands( op1, op2 );

   Class* cls=0;
   void* inst=0;

   if ( op2->isString() )
   {
#ifdef FALCON_MT_UNSAFE
         op1->asString()->append( *op2->asString() );
#else
         InstanceLock::Token* tk = m_lock.lock(op2->asString());
         String copy( *op2->asString() );
         m_lock.unlock(tk);

         tk = m_lock.lock(op1->asString());
         op1->asString()->append(copy);
         m_lock.unlock(tk);
#endif
      ctx->popData();
      return;
   }
   else if ( ! op2->asClassInst( cls, inst ) )
   {
      InstanceLock::Token* tk = m_lock.lock(op1->asString());
      op1->asString()->append( op2->describe() );
      m_lock.unlock(tk);
      ctx->popData();
      return;
   }

   // else we surrender, and we let the virtual system to find a way.
   ctx->pushCode( m_nextOp );
   long depth = ctx->codeDepth();

   // this will transform op2 slot into its string representation.
   cls->op_toString( ctx, inst );

   if( ctx->codeDepth() == depth )
   {
      ctx->popCode();

      // op2 has been transformed (and is ours)
      String* deep = (String*) op2->asInst();

      InstanceLock::Token* tk = m_lock.lock(str);
      deep->prepend( *str );
      m_lock.unlock(tk);
   }
}

void ClassString::op_amul( VMContext* ctx, void* instance ) const
{
   String* self = static_cast<String*>(instance);
   if( self->isImmutable() )
   {
      throw new OperandError( ErrorParam( e_prop_ro, __LINE__, SRC ).extra("Immutable string") );
   }

   // self count => self
   Item& i_count = ctx->topData();
   if( ! i_count.isOrdinal() )
   {
      throw new OperandError( ErrorParam( e_op_params, __LINE__ ).extra( "N" ) );
   }

   int64 count = i_count.forceInteger();
   ctx->popData();

   String* target;
   InstanceLock::Token* tk = m_lock.lock(self);

   String copy(*self);
   target = self;

   if( count == 0 )
   {
      target->size(0);
   }
   else
   {
      target->reserve( target->size() * count);
      // start from 1: we have already 1 copy in place
      for( int64 i = 1; i < count; ++i )
      {
         target->append(copy);
      }
   }

   if( tk != 0 )
   {
      m_lock.unlock(tk);
   }
}


void ClassString::op_adiv( VMContext* ctx, void* instance ) const
{
   String* self = static_cast<String*>(instance);
   if( self->isImmutable() )
   {
      throw new OperandError( ErrorParam( e_prop_ro, __LINE__, SRC ).extra("Immutable string") );
   }

   // self count => new
   Item& i_count = ctx->topData();
   if( ! i_count.isOrdinal() )
   {
      throw new OperandError( ErrorParam( e_op_params, __LINE__ ).extra( "N" ) );
   }

   int64 count = i_count.forceInteger();
   ctx->popData();

   if ( count < 0 || count >= 0xFFFFFFFFLL )
   {
      throw new OperandError( ErrorParam( e_op_params, __LINE__ ).extra( "out of range" ) );
   }

   InstanceLock::Token* tk = m_lock.lock(self);
   self->append((char_t) count);
   m_lock.unlock(tk);
}


void ClassString::op_setIndex( VMContext* ctx, void* self ) const
{
   String& str = *static_cast<String*>( self );
   if( str.isImmutable() )
   {
      throw new OperandError( ErrorParam( e_prop_ro, __LINE__, SRC ).extra("Immutable string") );
   }

   Item* value, *arritem, *index;
   ctx->operands( value, arritem, index );

   if ( ! value->isString() && ! value->isOrdinal())
   {
      throw new OperandError( ErrorParam( e_op_params, __LINE__ ).extra( "S" ) );
   }

   if ( index->isOrdinal() )
   {
      // simple index assignment: a[x] = value
      {
         InstanceLock::Locker( &m_lock, &str );

         int64 v = index->forceInteger();

         if ( v < 0 ) v = str.length() + v;

         if ( v >= str.length() )
         {
            throw new AccessError( ErrorParam( e_arracc, __LINE__ ).extra("index out of range") );
         }

         if( value->isOrdinal() ) {
            str.setCharAt( (length_t) v, (char_t) value->forceInteger() );
         }
         else {
            str.setCharAt( (length_t) v, (char_t) value->asString()->getCharAt( 0 ) );
         }
      }

      ctx->stackResult( 3, *value );
   }
   else if ( index->isRange() )
   {
      Range& rng = *static_cast<Range*>( index->asInst() );

      {
         InstanceLock::Locker( &m_lock, &str );

         int64 strLen = str.length();
         int64 start = rng.start();
         int64 end = ( rng.isOpen() ) ? strLen : rng.end();

         // handle negative indexes
         if ( start < 0 ) start = strLen + start;
         if ( end < 0 ) end = strLen + end;

         // do some validation checks before proceeding
         if ( start >= strLen  || end > strLen )
         {
            throw new AccessError( ErrorParam( e_arracc, __LINE__ ).extra("index out of range") );
         }

         if ( value->isString() )  // should be a string
         {
            String& strVal = *value->asString();
            str.change( (Falcon::length_t)start, (Falcon::length_t)end, strVal );
         }
         else
         {
            String temp;
            temp.append((char_t)value->forceInteger());
            str.change((Falcon::length_t)start, (Falcon::length_t)end, temp );
         }
      }

      ctx->stackResult( 3, *value );
   }
   else
   {
      throw new OperandError( ErrorParam( e_op_params, __LINE__ ).extra( "I|R" ) );
   }
}

}

/* end of classstring.cpp */