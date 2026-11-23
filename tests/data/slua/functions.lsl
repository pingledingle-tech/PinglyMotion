// user functions, a call before the function is written, return in the middle
integer twice(integer x)
{
    return helper(x) * 2;
}

integer helper(integer x)
{
    if (x < 0)
        return 0;
    return x;
}

string describe(integer on, key who)
{
    if (on == TRUE && who != NULL_KEY)
        return "on for " + (string)who;
    return "off";
}

integer end = 1;

default
{
    state_entry()
    {
        integer flag = TRUE;
        flag = !flag;
        llOwnerSay(describe(flag, llGetOwner()) + (string)twice(end));
        while (TRUE)
        {
            if (twice(3) == 6)
                return;
        }
    }
}
