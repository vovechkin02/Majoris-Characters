/** Correct the registry finish method's historical ownership annotation at the adapter boundary. */
[CCode (cheader_filename = "ibus.h")]
namespace Characters.IBusRegistry {
    /** Own the returned list and each referenced engine descriptor, including stale responses. */
    [CCode (cname = "ibus_bus_list_engines_async_finish")]
    public static GLib.List<IBus.EngineDesc>? finish_engines (IBus.Bus bus,
        GLib.AsyncResult result) throws GLib.Error;
}
