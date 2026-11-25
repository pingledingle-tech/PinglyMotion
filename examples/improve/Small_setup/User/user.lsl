// user needs hub's messages: it adds up their numbers.
// It asks Second Life to keep only 16 KB for it with llSetMemoryLimit.
integer got;

default
{
    state_entry()
    {
        llSetMemoryLimit(16384);
    }

    link_message(integer sender, integer number, string message, key id)
    {
        got = got + number;
        if (got == 6) llOwnerSay("All three messages came.");
    }
}
