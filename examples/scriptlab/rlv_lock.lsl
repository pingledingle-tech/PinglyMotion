// Locks detach and sendim with RLV, asks for the RLV version and reads the reply.
// llOwnerSay text that starts with @ goes to the RLV sandbox; the reply is heard on the channel after "=".
default
{
    state_entry()
    {
        llListen(2222, "", llGetOwner(), "");
        llOwnerSay("@detach=n,sendim=n");
        llOwnerSay("@version=2222");
    }

    listen(integer channel, string name, key id, string message)
    {
        llOwnerSay("RLV says: " + message);
    }
}
