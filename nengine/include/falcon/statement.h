/*
   FALCON - The Falcon Programming Language.
   FILE: statement.h

   Syntactic tree item definitions -- statements.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sun, 02 Jan 2011 20:37:39 +0100

   -------------------------------------------------------------------
   (C) Copyright 2011: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#ifndef FALCON_STATEMENT_H
#define FALCON_STATEMENT_H

#include <falcon/pstep.h>
#include <falcon/pcode.h>
#include <falcon/vm.h>

namespace Falcon
{

class Expression;
class SynTree;

/** Statement.
 * Statements are PStep that may require other sub-sequences to be evaluated.
 * In other words, they are
 */
class FALCON_DYN_CLASS Statement: public PStep
{

public:
   typedef enum {
      breakpoint_t,
      autoexpr_t,
      if_t,
      while_t,
      return_t
   } statement_t ;

   Statement( statement_t type, int32 line=0, int32 chr=0 ):
      PStep( line, chr ),
      m_type(type),
      m_step0(0), m_step1(0), m_step2(0), m_step3(0)
   {}

   inline virtual ~Statement() {}
   inline statement_t type() { return m_type; }

protected:
   /** Steps being prepared by the statement */
   PStep* m_step0;
   PStep* m_step1;
   PStep* m_step2;
   PStep* m_step3;


   inline void prepare( VMachine* vm ) const
   {
      register VMContext* ctx = vm->currentContext();

      if ( m_step0 )
      {
         ctx->pushCode(m_step0);
         if ( m_step1 )
         {
            ctx->pushCode(m_step1);
            if ( m_step2 )
            {
               ctx->pushCode(m_step2);
               if ( m_step3 )
               {
                  ctx->pushCode(m_step3);
               }
            }
         }
      }
   }

   friend class SynTree;
private:
   statement_t m_type;
};

/** Statement causing the VM to return.

 This is a debug feature that causes the VM to return from its main
 loop when it meets this statement.
 */
class FALCON_DYN_CLASS Breakpoint: public Statement
{
public:
   Breakpoint(int32 line=0, int32 chr = 0);
   virtual ~Breakpoint();

   void describe( String& tgt ) const;

   static void apply_( const PStep*, VMachine* vm );
};


/** Autoexpression.
 *
 * This statement is needed to wrap an expression so that its
 * result is removed from the stack.
 *
 * The obvious un-necessity of this class advises for the usage of
 * registers where expressions place their results; but reasons
 * against are equally valid.
 */
class FALCON_DYN_CLASS StmtAutoexpr: public Statement
{
public:
   StmtAutoexpr( Expression* expr, int32 line=0, int32 chr = 0 );
   virtual ~StmtAutoexpr();

   void describe( String& tgt ) const;
   inline String describe() const { return PStep::describe(); }

   void oneLiner( String& tgt ) const;
   inline String oneLiner() const { return PStep::oneLiner(); }
private:
   Expression* m_expr;
   PCode m_pcExpr;
};


/** Return statement.
 *
 * Exits the current functions.
 */
class FALCON_DYN_CLASS StmtReturn: public Statement
{
public:
   /** Returns a value */
   StmtReturn( Expression* expr = 0, int32 line=0, int32 chr = 0 );
   virtual ~StmtReturn();

   void describe( String& tgt ) const;
   inline String describe() const { return PStep::describe(); }
   static void apply_( const PStep*, VMachine* vm );

private:
   Expression* m_expr;
   PCode m_pcExpr;
};


class FALCON_DYN_CLASS StmtWhile: public Statement
{
public:
   StmtWhile( Expression* check, SynTree* stmts, int32 line=0, int32 chr = 0 );
   virtual ~StmtWhile();

   void describe( String& tgt ) const;
   inline String describe() const { return PStep::describe(); }
   void oneLiner( String& tgt ) const;
   static void apply_( const PStep*, VMachine* vm );

private:
   Expression* m_check;
   PCode m_pcCheck;
   SynTree* m_stmts;
};


class FALCON_DYN_CLASS StmtIf: public Statement
{
public:
   StmtIf( Expression* check, SynTree* ifTrue, SynTree* ifFalse = 0, int32 line=0, int32 chr = 0 );
   virtual ~StmtIf();

   virtual void describe( String& tgt ) const;
   inline String describe() const { return PStep::describe(); }
   void oneLiner( String& tgt ) const;
   inline String oneLiner() const { return PStep::oneLiner(); }

   static void apply_( const PStep*, VMachine* vm );

   /** Adds an else-if branch to the if statement */
   StmtIf& addElif( Expression *check, SynTree* ifTrue, int32 line=0, int32 chr = 0 );

   /** Sets the else branch for this if statement. */
   StmtIf& setElse( SynTree* ifFalse );

private:
   SynTree* m_ifFalse;

   class ElifBranch
   {
   public:
      Expression* m_check;
      PCode m_pcCheck;
      SynTree* m_ifTrue;
      SourceRef m_sr;

      ElifBranch( Expression *check, SynTree* ifTrue, int32 line=0, int32 chr = 0 ):
         m_check( check ),
         m_ifTrue( ifTrue ),
         m_sr( line, chr )
      {
         compile();
      }

      ElifBranch( const ElifBranch& other ):
         m_check( other.m_check ),
         m_ifTrue( other.m_ifTrue ),
         m_sr(other.m_sr)
      {
         compile();
      }

      ~ElifBranch();

      void compile();
   };

   std::vector<ElifBranch* > m_elifs;
};

}

#endif

/* end of statement.h */
