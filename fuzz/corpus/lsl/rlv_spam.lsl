default
{
    state_entry()
    {
        integer i;
        llListen(5, "", NULL_KEY, "");
        for (i = 0; i < 300; ++i)
        {
            llOwnerSay("@sendim:" + (string)i + "=n,detach=n,version=5,clear=send");
        }
    }
    listen(integer c, string n, key k, string m) { llOwnerSay("@getstatus=5"); }
}
