// Asks which restrictions are held and reads the answer on channel 3333.
// Run it after rlv_lock.lsl with keep_rlv true to see the locks in the answer.
default
{
    state_entry()
    {
        llListen(3333, "", llGetOwner(), "");
        llOwnerSay("@getstatus=3333");
    }

    listen(integer channel, string name, key id, string message)
    {
        llOwnerSay("Status: " + message);
    }
}
