/**
 *  \file gtk_Statusbar.cpp
 */

#include "gtk_Statusbar.hpp"

#include "gtk_HBox.hpp"

/*#
   @beginmodule gtk
*/

namespace Falcon {
namespace Gtk {

/**
 *  \brief module init
 */
void Statusbar::modInit( Falcon::Module* mod )
{
    Falcon::Symbol* c_Statusbar = mod->addClass( "GtkStatusbar", &Statusbar::init );

    Falcon::InheritDef* in = new Falcon::InheritDef( mod->findGlobalSymbol( "GtkHBox" ) );
    c_Statusbar->getClassDef()->addInheritance( in );

    c_Statusbar->getClassDef()->factory( &Statusbar::factory );

    Gtk::MethodTab methods[] =
    {
    { "get_context_id",     &Statusbar::get_context_id },
    { "push",               &Statusbar::push },
    { "pop",                &Statusbar::pop },
    { "remove",             &Statusbar::remove },
    { "set_has_resize_grip",&Statusbar::set_has_resize_grip },
    { "get_has_resize_grip",&Statusbar::get_has_resize_grip },
#if GTK_CHECK_VERSION( 2, 20, 0 )
    { "get_message_area",   &Statusbar::get_message_area },
#endif
    { NULL, NULL }
    };

    for ( Gtk::MethodTab* meth = methods; meth->name; ++meth )
        mod->addClassMethod( c_Statusbar, meth->name, meth->cb );
}


Statusbar::Statusbar( const Falcon::CoreClass* gen, const GtkStatusbar* sbar )
    :
    Gtk::CoreGObject( gen, (GObject*) sbar )
{}


Falcon::CoreObject* Statusbar::factory( const Falcon::CoreClass* gen, void* sbar, bool )
{
    return new Statusbar( gen, (GtkStatusbar*) sbar );
}


/*#
    @class GtkStatusbar
    @brief Report messages of minor importance to the user

    A GtkStatusbar is usually placed along the bottom of an application's main
    GtkWindow. It may provide a regular commentary of the application's status
    (as is usually the case in a web browser, for example), or may be used to
    simply output a message when the status changes, (when an upload is complete
    in an FTP client, for example). It may also have a resize grip (a triangular
    area in the lower right corner) which can be clicked on to resize the window
    containing the statusbar.

    Status bars in GTK+ maintain a stack of messages. The message at the top of
    the each bar's stack is the one that will currently be displayed.

    Any messages added to a statusbar's stack must specify a context id that is
    used to uniquely identify the source of a message. This context id can be
    generated by gtk_statusbar_get_context_id(), given a message and the statusbar
    that it will be added to. Note that messages are stored in a stack, and when
    choosing which message to display, the stack structure is adhered to,
    regardless of the context identifier of a message.

    One could say that a statusbar maintains one stack of messages for display
    purposes, but allows multiple message producers to maintain sub-stacks of
    the messages they produced (via context ids).

    Status bars are created using gtk_statusbar_new().

    Messages are added to the bar's stack with gtk_statusbar_push().

    The message at the top of the stack can be removed using gtk_statusbar_pop().
    A message can be removed from anywhere in the stack if its message_id was
    recorded at the time it was added. This is done using gtk_statusbar_remove().
 */
FALCON_FUNC Statusbar::init( VMARG )
{
    NO_ARGS
    MYSELF;
    GtkWidget* wdt = gtk_statusbar_new();
    self->setObject( (GObject*) wdt );
}


/*#
    @method signal_text_popped GtkScale
    @brief Is emitted whenever a new message is popped off a statusbar's stack.
 */
FALCON_FUNC Statusbar::signal_text_popped( VMARG )
{
    NO_ARGS
    CoreGObject::get_signal( "text_popped", (void*) &Statusbar::on_text_popped, vm );
}


void Statusbar::on_text_popped( GtkStatusbar* obj, guint ctxt_id, gchar* text, gpointer _vm )
{
    GET_SIGNALS( obj );
    CoreSlot* cs = _signals->getChild( "text_popped", false );

    if ( !cs || cs->empty() )
        return;

    VMachine* vm = (VMachine*) _vm;
    Iterator iter( cs );
    Item it;

    do
    {
        it = iter.getCurrent();

        if ( !it.isCallable() )
        {
            if ( !it.isComposed()
                || !it.asObject()->getMethod( "on_text_popped", it ) )
            {
                printf(
                "[GtkStatusbar::on_text_popped] invalid callback (expected callable)\n" );
                return;
            }
        }
        vm->pushParam( (int64) ctxt_id );
        vm->pushParam( new String( text ) );
        vm->callItem( it, 2 );
        iter.next();
    }
    while ( iter.hasCurrent() );
}


/*#
    @method signal_text_pushed GtkScale
    @brief Is emitted whenever a new message gets pushed onto a statusbar's stack.
 */
FALCON_FUNC Statusbar::signal_text_pushed( VMARG )
{
    NO_ARGS
    CoreGObject::get_signal( "text_pushed", (void*) &Statusbar::on_text_pushed, vm );
}


void Statusbar::on_text_pushed( GtkStatusbar* obj, guint ctxt_id, gchar* text, gpointer _vm )
{
    GET_SIGNALS( obj );
    CoreSlot* cs = _signals->getChild( "text_pushed", false );

    if ( !cs || cs->empty() )
        return;

    VMachine* vm = (VMachine*) _vm;
    Iterator iter( cs );
    Item it;

    do
    {
        it = iter.getCurrent();

        if ( !it.isCallable() )
        {
            if ( !it.isComposed()
                || !it.asObject()->getMethod( "on_text_pushed", it ) )
            {
                printf(
                "[GtkStatusbar::on_text_pushed] invalid callback (expected callable)\n" );
                return;
            }
        }
        vm->pushParam( (int64) ctxt_id );
        vm->pushParam( new String( text ) );
        vm->callItem( it, 2 );
        iter.next();
    }
    while ( iter.hasCurrent() );
}


/*#
    @method get_context_id GtkStatusbar
    @brief Returns a new context identifier, given a description of the actual context.
    @param context_description textual description of what context the new message is being used in
    @return an integer id

    Note that the description is not shown in the UI.
 */
FALCON_FUNC Statusbar::get_context_id( VMARG )
{
    Gtk::ArgCheck1 args( vm, "S" );
    char* desc = args.getCString( 0 );
    MYSELF;
    GET_OBJ( self );
    vm->retval( (int64) gtk_statusbar_get_context_id( (GtkStatusbar*)_obj, desc ) );
}


/*#
    @method push GtkStatusbar
    @brief Pushes a new message onto a statusbar's stack.
    @param context_id the message's context id, as returned by get_context_id()
    @param text the message to add to the statusbar
    @return a message id that can be used with remove().
 */
FALCON_FUNC Statusbar::push( VMARG )
{
    Gtk::ArgCheck1 args( vm, "I,S" );
    guint id = args.getInteger( 0 );
    gchar* txt = args.getCString( 1 );
    MYSELF;
    GET_OBJ( self );
    vm->retval( (int64) gtk_statusbar_push( (GtkStatusbar*)_obj, id, txt ) );
}


/*#
    @method pop GtkStatusbar
    @brief Removes the first message in the GtkStatusBar's stack with the given context id.
    @param context_id a context identifier

    Note that this may not change the displayed message, if the message at the top
    of the stack has a different context id.
 */
FALCON_FUNC Statusbar::pop( VMARG )
{
    Item* i_id = vm->param( 0 );
#ifndef NO_PARAMETER_CHECK
    if ( !i_id || i_id->isNil() || !i_id->isInteger() )
        throw_inv_params( "I" );
#endif
    MYSELF;
    GET_OBJ( self );
    gtk_statusbar_pop( (GtkStatusbar*)_obj, i_id->asInteger() );
}


/*#
    @method remove GtkStatusbar
    @brief Forces the removal of a message from a statusbar's stack.
    @param context_id a context identifier
    @param message_id a message identifier, as returned by push()

    The exact context_id and message_id must be specified.
 */
FALCON_FUNC Statusbar::remove( VMARG )
{
    Item* i_cid = vm->param( 0 );
    Item* i_mid = vm->param( 1 );
#ifndef NO_PARAMETER_CHECK
    if ( !i_cid || i_cid->isNil() || !i_cid->isInteger()
        || !i_mid || i_mid->isNil() || !i_mid->isInteger() )
        throw_inv_params( "I,I" );
#endif
    MYSELF;
    GET_OBJ( self );
    gtk_statusbar_remove( (GtkStatusbar*)_obj, i_cid->asInteger(), i_mid->asInteger() );
}


/*#
    @method set_has_resize_grip GtkStatusbar
    @brief Sets whether the statusbar has a resize grip. true by default.
    @param setting true to have a resize grip
 */
FALCON_FUNC Statusbar::set_has_resize_grip( VMARG )
{
    Item* i_bool = vm->param( 0 );
#ifndef NO_PARAMETER_CHECK
    if ( !i_bool || i_bool->isNil() || !i_bool->isInteger() )
        throw_inv_params( "B" );
#endif
    MYSELF;
    GET_OBJ( self );
    gtk_statusbar_set_has_resize_grip( (GtkStatusbar*)_obj, i_bool->asBoolean() ? TRUE : FALSE );
}


/*#
    @method get_has_resize_grip GtkStatusbar
    @brief Returns whether the statusbar has a resize grip.
    @return true if the statusbar has a resize grip.
 */
FALCON_FUNC Statusbar::get_has_resize_grip( VMARG )
{
    NO_ARGS
    MYSELF;
    GET_OBJ( self );
    vm->retval( (bool) gtk_statusbar_get_has_resize_grip( (GtkStatusbar*)_obj ) );
}


#if GTK_CHECK_VERSION( 2, 20, 0 )
/*#
    @method get_message_area GtkStatusbar
    @brief Retrieves the box containing the label widget.
    @return a GtkHBox
 */
FALCON_FUNC Statusbar::get_message_area( VMARG )
{
    NO_ARGS
    MYSELF;
    GET_OBJ( self );
    GtkWidget* box = gtk_statusbar_get_message_area( (GtkStatusbar*)_obj );
    vm->retval( new Gtk::HBox( vm->findWKI( "GtkHBox" )->asClass(), (GtkHBox*) box ) );
}
#endif

} // Gtk
} // Falcon

// vi: set ai et sw=4:
// kate: replace-tabs on; shift-width 4;
