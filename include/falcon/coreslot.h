/*
   FALCON - The Falcon Programming Language.
   FILE: coreslot.h

   Core Slot - Messaging slot system.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sun, 11 Jan 2009 18:28:35 +0100

   -------------------------------------------------------------------
   (C) Copyright 2009: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#ifndef FALCON_CORESLOT_H
#define FALCON_CORESLOT_H

#include <falcon/setup.h>
#include <falcon/types.h>
#include <falcon/itemlist.h>
#include <falcon/string.h>
#include <falcon/traits.h>
#include <falcon/mt.h>
#include <falcon/falconobject.h>

namespace Falcon {

class VMachine;
class VMMessage;

/** Slot for messages sent around by the VM.
   This class provide abstract support for low level messaging system.

   The slot represents an end of the communication process where the incoming
   message is definitely
*/
class CoreSlot: public ItemList
{
   String m_name;
   mutable Mutex m_mtx;
   mutable int32 m_refcount;

   Item m_assertion;
   bool m_bHasAssert;

public:
   CoreSlot( const String &name ):
      m_name( name ),
      m_refcount(1),
      m_bHasAssert( false )
   {}

   const String& name() const { return m_name; }

   /** Prepares a broadcast from the current frame.

      Meant to be called from inside extension functions going to
      perform broadcasts, this function prepares the multi-call frame
      used for repeated broadcast-based calls.

      @param vm The VM on which the broadcast is going to be performed.
      @param pfirst The first parameter in the current call frame that must be repeated.
      @param pcount Parameter count to be passed in the broadcast.
      \param msg The message that caused the slot to be broadcast (can be none if internally broadcast).
   */
   void prepareBroadcast( VMachine *vm, uint32 pfirst, uint32 pcount, VMMessage* msg = 0 );

   /** Remove a ceratin item from this slot.
      This will remove an item considered equal to the subscriber from this list.
   */
   bool remove( const Item &subsriber );


   void incref() const;  // can
   void decref();

   bool hasAssert() const { return m_bHasAssert; }
   const Item &assertion() const { return m_assertion; }
   /** Sets an assertion for this slot.
      No action is taken.
   */
   void assert( const Item &a ) { m_assertion = a; m_bHasAssert = true; }

   /** Performs an assertion for this slot.
      Also, prepares the VM to run a broadcast loop with the asserted item.
   */
   void assert( VMachine* vm, const Item &a );

   void retract() { m_bHasAssert = false; }

   virtual FalconData *clone() const;
   virtual void gcMark( uint32 mark );
};


/** Traits for the core slots. */
class CoreSlotPtrTraits: public ElementTraits
{
public:
   virtual ~CoreSlotPtrTraits() {}
	virtual uint32 memSize() const;
	virtual void init( void *itemZone ) const;
	virtual void copy( void *targetZone, const void *sourceZone ) const;
	virtual int compare( const void *first, const void *second ) const;
	virtual void destroy( void *item ) const;
   virtual bool owning() const;
};

namespace traits
{
   extern CoreSlotPtrTraits &t_coreslotptr();
}

bool coreslot_broadcast_internal( VMachine *vm );

/** Class taking care of finalizing the core slots when they are published to scripts. */
class FALCON_DYN_CLASS CoreSlotCarrier: public FalconObject
{
public:
   CoreSlotCarrier( const CoreClass* generator, CoreSlot* cs, bool bSeralizing = false );
   CoreSlotCarrier( const CoreSlotCarrier &other );
   virtual ~CoreSlotCarrier();
   virtual CoreObject *clone() const;

   /** Change slot after the creation of this carrier (for VMSlot_init) */
   void setSlot( CoreSlot* cs );
};

extern "C"
{
   CoreObject* CoreSlotFactory( const CoreClass *cls, void *user_data, bool bDeserial );
}

}

#endif

/* end of coreslot.h */
