integer n;
list items = ["a", 1, 2.5, <1, 2, 3>];
integer f(integer x) { if (x < 2) return x; return f(x - 1) + f(x - 2); }
default
{
    state_entry()
    {
        llListen(1, "", "", "");
        string s = llDumpList2String(items, ",");
        vector v = <1, 0, 0> * <0, 0, 0.7071068, 0.7071068>;
        @top;
        if (++n < 3) jump top;
        llOwnerSay(s + (string)f(10) + (string)v + llGetSubString(s, -3, 1));
    }
    listen(integer c, string name, key id, string m) { llSay(0, m); state two; }
}
state two
{
    state_entry() { llSetTimerEvent(1); }
    touch_start(integer t) { llResetScript(); }
    timer() { llOwnerSay(llList2CSV(llParseString2List("a b,c", [" "], [","]))); }
}
