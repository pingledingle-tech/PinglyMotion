default
{
    state_entry()
    {
        rotation q = llEuler2Rot(<0.3, 0.5, 0.7>);
        llOwnerSay((string)llRot2Euler(q) + (string)llRot2Fwd(q) + (string)llRotBetween(<1, 0, 0>, <-1, 0, 0>));
        llOwnerSay((string)llAxisAngle2Rot(<0, 0, 0>, 1.0));
        list l = llListReplaceList([1, 2, 3, 4], ["a"], -3, 9);
        l += llList2ListStrided(l, -2, 99, 3);
        l = llListSortStrided(l + l, 2, -1, TRUE);
        llOwnerSay(llList2CSV(l));
        string s = llReplaceSubString("aaaa bbbb aaaa", "aa", "<>", -2);
        llOwnerSay(s + llStringToBase64(s) + llBase64ToString("////") + llMD5String(s, -1) + llSHA1String(s));
        llOwnerSay(llEscapeURL(s + "å") + llUnescapeURL("%%41%4%C3%") + (string)llBase64ToInteger("AAAAAA=="));
        llSetObjectName(s);
        llSetObjectDesc(llEscapeURL(s));
        llOwnerSay(llGetObjectName() + llGetObjectDesc() + (string)llGetScale() + (string)llGetStartParameter());
    }
}
