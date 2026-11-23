// jump has no SLua form
default
{
    state_entry()
    {
        integer i = 0;
        @again;
        i++;
        if (i < 3)
            jump again;
        llOwnerSay((string)i);
    }
}
