/*
   FALCON - The Falcon Programming Language.
   FILE: corefunction.cpp

   Function object handler.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sat, 15 Jan 2011 19:09:07 +0100

   -------------------------------------------------------------------
   (C) Copyright 2011: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#include <falcon/corefunction.h>
#include <falcon/synfunc.h>
#include "falcon/itemid.h"

namespace Falcon {

CoreFunction::CoreFunction():
   Class("Function", FLC_CLASS_ID_FUNCTION )
{
}


CoreFunction::~CoreFunction()
{
}


void* CoreFunction::create(void* creationParams ) const
{
   cpars* cp = static_cast<cpars*>(creationParams);
   return new SynFunc( cp->m_name, cp->m_module );
}


void CoreFunction::dispose( void* self ) const
{
   Function* f = static_cast<Function*>(self);
   delete f;
}


void* CoreFunction::clone( void* source ) const
{
   //Function* f = static_cast<Function*>(self);
   //TODO
   return 0;
}


void CoreFunction::serialize( DataWriter* stream, void* self ) const
{
   // TODO
}


void* CoreFunction::deserialize( DataReader* stream ) const
{
   // TODO
   return 0;
}

void CoreFunction::describe( void* instance, String& target ) const
{
   Function* func = static_cast<Function*>(instance);
   target = func->name() + "()";
}

}

/* end of corefunction.cpp */