/*
   FALCON - The Falcon Programming Language.
   FILE: parser_while.cpp

   Parser for Falcon source files -- while statement handler
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sun, 03 Jul 2011 18:13:22 +0200

   -------------------------------------------------------------------
   (C) Copyright 2011: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#undef SRC
#define SRC "engine/sp/parsercontext.cpp"

#include <falcon/setup.h>
#include <falcon/trace.h>

#include <falcon/expression.h>
#include <falcon/statement.h>
#include <falcon/stmtautoexpr.h>
#include <falcon/exprvalue.h>

#include <falcon/parser/rule.h>
#include <falcon/parser/parser.h>

#include <falcon/sp/sourceparser.h>
#include <falcon/sp/parsercontext.h>
#include <falcon/sp/parser_deletor.h>
#include <falcon/sp/parser_while.h>

namespace Falcon {

using namespace Parsing;

bool while_errhand(const NonTerminal&, Parser& p)
{
   TokenInstance* ti = p.getNextToken();
   TokenInstance* ti2 = p.getLastToken();

   if( p.lastErrorLine() < ti->line() )
   {
      // generate another error only if we didn't notice it already.
      p.addError( e_syn_if, p.currentSource(), ti->line(), ti2->chr() );
   }

   if( ! p.interactive() )
   {
      // put in a fake if statement (else, subsequent else/elif/end would also cause errors).
      SynTree* stWhile = new SynTree;
      StmtWhile* fakeWhile = new StmtWhile( new ExprValue(1), stWhile, ti->line(), ti->chr() );
      ParserContext* st = static_cast<ParserContext*>(p.context());
      st->openBlock( fakeWhile, stWhile );
   }
   else
   {
      MESSAGE2( "while_errhand -- Ignoring WHILE in interactive mode." );
   }
   // on interactive parsers, let the whole instruction to be destroyed.

   p.clearFrames();
   return true;
}

void apply_while_short( const Rule&, Parser& p )
{
   // << (r_while_short << "while_short" << apply_while_short << T_while << Expr << T_Colon << Expr << T_EOL )

   TokenInstance* twhile = p.getNextToken();
   TokenInstance* texpr = p.getNextToken();
   p.getNextToken();
   TokenInstance* tstatement = p.getNextToken();

   Expression* expr = static_cast<Expression*>(texpr->detachValue());
   Expression* sa = static_cast<Expression*>(tstatement->detachValue());
   ParserContext* st = static_cast<ParserContext*>(p.context());

   SynTree* whsyn = new SynTree;
   whsyn->append( new StmtAutoexpr(sa) );

   StmtWhile* stmt_wh = new StmtWhile(expr, whsyn);
   stmt_wh->decl( twhile->line(), twhile->chr() );
   st->addStatement( stmt_wh );

   // clear the stack
   p.simplify(5);
}


void apply_while( const Rule&, Parser& p )
{
   // << (r_while << "while" << apply_while << T_while << Expr << T_EOL )
   TokenInstance* twhile = p.getNextToken();
   TokenInstance* texpr = p.getNextToken();
   p.getNextToken();

   Expression* expr = static_cast<Expression*>(texpr->detachValue());
   ParserContext* st = static_cast<ParserContext*>(p.context());

   SynTree* whsyn = new SynTree;
   StmtWhile* stmt_while = new StmtWhile(expr, whsyn );
   stmt_while->decl( twhile->line(), twhile->chr() );
   st->openBlock( stmt_while, whsyn );

   // clear the stack
   p.simplify(3);
}

}

/* end of parser_while.cpp */