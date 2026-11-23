// lists, strings and the functions that count positions
list names = ["Ann", "Bo"];
string greeting = "Hi";

default
{
    state_entry()
    {
        names += "Cy";
        names = names + ["Di", 4, 2.5];
        integer i = llListFindList(names, ["Bo"]);
        if (i != -1)
            llOwnerSay(llList2String(names, i) + " is number " + (string)(i + 1));
        if (~llSubStringIndex(greeting, "i"))
            llOwnerSay("has an i");
        string first = llGetSubString(greeting, 0, 0);
        string last = llGetSubString(greeting, -1, -1);
        names = llDeleteSubList(names, i, i + 1);
        llOwnerSay(first + last + (string)llGetListLength(names));
        llOwnerSay(llDumpList2String(names, ", ") + (string)names);
        if (names == [])
            llOwnerSay("empty");
        greeting += "!";
    }
}
