/*
   FALCON - The Falcon Programming Language.
   FILE: conbox_ext.cpp

   Basic Console I/O support
   Main module file, providing the module object to
   the Falcon engine.
   -------------------------------------------------------------------
   Author: Unknown author
   Begin: Thu, 05 Sep 2008 20:12:14 +0200

   -------------------------------------------------------------------
   (C) Copyright 2008: The above AUTHOR

      Licensed under the Falcon Programming Language License,
   Version 1.1 (the "License"); you may not use this file
   except in compliance with the License. You may obtain
   a copy of the License at

      http://www.falconpl.org/?page_id=license_1_1

   Unless required by applicable law or agreed to in writing,
   software distributed under the License is distributed on
   an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
   KIND, either express or implied. See the License for the
   specific language governing permissions and limitations
   under the License.

*/

/** \file
   Main module file, providing the module object to
   the Falcon engine.
*/

#include <falcon/module.h>
#include "conbox_ext.h"
#include "conbox_st.h"

#include "version.h"

/*#
   @module conio_conbox conbox
   @brief Extended GUI-like interface for text-based terminals.
   @inmodule conio
   
   @note Currently this module is under development and not released.

   This module accompains the conio module and provides extended
   text-oriented interface functionalities to create boxes,
   virtual windows, mouse buttons and so on in text interfaces.
*/

FALCON_MODULE_DECL( const Falcon::EngineData &data )
{
   #define FALCON_DECLARE_MODULE self

   // set the static engine data
   data.set();

   // initialize the module
   Falcon::Module *self = new Falcon::Module();
   self->name( "conbox" );
   self->language( "en_US" );
   self->engineVersion( FALCON_VERSION_NUM );
   self->version( VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION );

   //============================================================
   // Here declare the international string table implementation
   //
   #include "conbox_st.h"

   //============================================================
   // Here declare skeleton api
   //
   self->addExtFunc( "cb_skeleton", Falcon::Ext::cb_skeleton );
   self->addExtFunc( "cb_skeletonString", Falcon::Ext::cb_skeletonString );

   //============================================================
   // Declare dependency (private --- we just need the service)
   //
   self->addDepend( "conio", true );

   return self;
}

/* end of conbox.cpp */
