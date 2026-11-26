// Cuff set: four cuffs with leash points, chains between any two cuffs,
// a chain to a ceiling ring (hang by the chain) and a leash another avatar holds.
// One script in the root runs it all; the other prims only show it.
//
// Touch any part (or the box, the HUD) to open the menu. Every button is in
// the menu, so it works with a click in the 3D view and with lsl_event.
//
// A chain is a particle stream from one prim to another (PSYS_SRC_TARGET_KEY),
// as LockGuard and LockMeister chains work in Second Life. Every prim emits at
// most one stream, so each cuff has two chain points: its ring and its leash
// point. The ceiling ring has one.
//
// Written from scratch for PinglyMotion (no OpenCollar or other code copied).
// Tested only in the Script Lab sandbox, never in Second Life.

integer DIALOG = -7311;     // the channel the menu answers on

// the four cuffs and the ceiling; index 4 is the ceiling
list NAMES = ["Wrist L", "Wrist R", "Ankle L", "Ankle R", "Ceiling"];
list RINGS = ["Wrist_left", "Wrist_right", "Ankle_left", "Ankle_right"];
list POINTS = ["Leash_wrist_left", "Leash_wrist_right", "Leash_ankle_left", "Leash_ankle_right"];
// the attachment point of each cuff, for the RLV lock of that cuff
list ATTACH = ["l forearm", "r forearm", "l lower leg", "r lower leg"];
integer CEILING = 4;

// link numbers, found by name; the numbers after the names are the links of
// the folder model (prims in name order after the root)
list ringLink;
list pointLink;
integer ceilingLink;

integer locked;             // one bit per cuff
list chains;                // [a, b, emitter link] per chain; a < b, b is CEILING for the ceiling
list leashes;               // [cuff, holder key, holder name, emitter link] per leash
list applied;               // RLV restrictions this script holds now
integer playing;            // the hang animation plays
integer mayAnimate;

key menuUser;
string step = "main";
integer first;              // the first point picked for a chain

integer linkOf(string name, integer fallback)
{
    integer n = llGetNumberOfPrims();
    integer i;
    for (i = 2; i <= n; ++i)
        if (llGetLinkName(i) == name) return i;
    return fallback;
}

string pointName(integer i) { return llList2String(NAMES, i); }

integer isLocked(integer cuff) { return (locked >> cuff) & 1; }

integer busy(integer link)
{
    integer i;
    for (i = 2; i < llGetListLength(chains); i += 3)
        if (llList2Integer(chains, i) == link) return TRUE;
    for (i = 3; i < llGetListLength(leashes); i += 4)
        if (llList2Integer(leashes, i) == link) return TRUE;
    return FALSE;
}

// the first chain point of a cuff that emits nothing yet, or 0
integer freePoint(integer cuff, integer pointFirst)
{
    integer ring = llList2Integer(ringLink, cuff);
    integer point = llList2Integer(pointLink, cuff);
    if (pointFirst)
    {
        if (!busy(point)) return point;
        if (!busy(ring)) return ring;
        return 0;
    }
    if (!busy(ring)) return ring;
    if (!busy(point)) return point;
    return 0;
}

// where a chain ends: the ring of a cuff, or the ceiling ring
integer endLink(integer p)
{
    if (p == CEILING) return ceilingLink;
    return llList2Integer(ringLink, p);
}

emit(integer link, key target)
{
    llLinkParticleSystem(link, [
        PSYS_PART_FLAGS, PSYS_PART_FOLLOW_SRC_MASK | PSYS_PART_FOLLOW_VELOCITY_MASK | PSYS_PART_TARGET_POS_MASK | PSYS_PART_TARGET_LINEAR_MASK,
        PSYS_SRC_PATTERN, PSYS_SRC_PATTERN_DROP,
        PSYS_SRC_TARGET_KEY, target,
        PSYS_PART_START_SCALE, <0.06, 0.06, 0.0>,
        PSYS_PART_START_COLOR, <0.7, 0.7, 0.7>,
        PSYS_PART_MAX_AGE, 1.0,
        PSYS_SRC_BURST_RATE, 0.01,
        PSYS_SRC_BURST_PART_COUNT, 1]);
}

integer chainIndex(integer a, integer b)
{
    integer i;
    for (i = 0; i < llGetListLength(chains); i += 3)
        if (llList2Integer(chains, i) == a && llList2Integer(chains, i + 1) == b) return i;
    return -1;
}

integer leashIndex(integer cuff)
{
    integer i;
    for (i = 0; i < llGetListLength(leashes); i += 4)
        if (llList2Integer(leashes, i) == cuff) return i;
    return -1;
}

// adds a chain between two points (a cuff, or the ceiling); says why when it cannot
integer addChain(integer a, integer b, integer quiet)
{
    if (a > b) { integer t = a; a = b; b = t; }
    if (a == b || a == CEILING) return FALSE;
    string pair = pointName(a) + " - " + pointName(b);
    if (chainIndex(a, b) >= 0)
    {
        if (!quiet) llOwnerSay(pair + " is chained already.");
        return FALSE;
    }
    integer emitter = freePoint(a, FALSE);
    integer target = endLink(b);
    if (!emitter)
    {
        if (b == CEILING)
        {
            if (!busy(ceilingLink)) emitter = ceilingLink;
        }
        else emitter = freePoint(b, FALSE);
        target = endLink(a);
    }
    if (!emitter)
    {
        if (!quiet) llOwnerSay("No free chain point for " + pair + ". Unchain something first.");
        return FALSE;
    }
    chains += [a, b, emitter];
    emit(emitter, llGetLinkKey(target));
    if (!quiet) llOwnerSay("Chained " + pair + ".");
    return TRUE;
}

removeChainAt(integer i)
{
    llLinkParticleSystem(llList2Integer(chains, i + 2), []);
    chains = llDeleteSubList(chains, i, i + 2);
}

integer addLeash(integer cuff, key holder, string name, integer quiet)
{
    if (leashIndex(cuff) >= 0)
    {
        if (!quiet) llOwnerSay(pointName(cuff) + " is on a leash already.");
        return FALSE;
    }
    integer emitter = freePoint(cuff, TRUE);
    if (!emitter)
    {
        if (!quiet) llOwnerSay("No free chain point on " + pointName(cuff) + ". Unchain something first.");
        return FALSE;
    }
    leashes += [cuff, holder, name, emitter];
    emit(emitter, holder);
    if (!quiet)
    {
        llOwnerSay("Leash on " + pointName(cuff) + ", held by " + name + ".");
        if (holder != llGetOwner()) llRegionSayTo(holder, 0, "You hold the leash on " + pointName(cuff) + ".");
    }
    return TRUE;
}

removeLeashAt(integer i)
{
    llLinkParticleSystem(llList2Integer(leashes, i + 3), []);
    leashes = llDeleteSubList(leashes, i, i + 3);
}

integer hanging()
{
    integer i;
    for (i = 1; i < llGetListLength(chains); i += 3)
        if (llList2Integer(chains, i) == CEILING) return TRUE;
    return FALSE;
}

// a wrist chained to the ceiling: the arms are up, so the hang pose plays
integer hangingByWrist()
{
    return chainIndex(0, CEILING) >= 0 || chainIndex(1, CEILING) >= 0;
}

list addOnce(list l, list items)
{
    integer i;
    for (i = 0; i < llGetListLength(items); ++i)
        if (llListFindList(l, llList2List(items, i, i)) < 0) l += llList2List(items, i, i);
    return l;
}

// the RLV restrictions the cuffs need now
list wanted()
{
    list r;
    integer c;
    if (locked) r = ["detach"];
    for (c = 0; c < 4; ++c)
        if (isLocked(c)) r += ["detach:" + llList2String(ATTACH, c)];
    integer i;
    for (i = 0; i < llGetListLength(chains); i += 3)
    {
        integer a = llList2Integer(chains, i);
        integer b = llList2Integer(chains, i + 1);
        if (b == CEILING) r = addOnce(r, ["fly", "tplm", "tploc", "tplure", "sittp"]);
        else if (a < 2 && b < 2) r = addOnce(r, ["rez", "edit"]);
        else if (a >= 2 && b >= 2) r = addOnce(r, ["fly"]);
        else r = addOnce(r, ["fly", "rez", "edit"]);
    }
    if (llGetListLength(leashes)) r = addOnce(r, ["tplm", "tploc", "tplure"]);
    return r;
}

// sends only what changed: =n for new restrictions, =y for ones no longer needed
applyRlv()
{
    list want = wanted();
    list lines;
    integer i;
    for (i = 0; i < llGetListLength(applied); ++i)
        if (llListFindList(want, llList2List(applied, i, i)) < 0) lines += [llList2String(applied, i) + "=y"];
    for (i = 0; i < llGetListLength(want); ++i)
        if (llListFindList(applied, llList2List(want, i, i)) < 0) lines += [llList2String(want, i) + "=n"];
    if (llGetListLength(lines)) llOwnerSay("@" + llDumpList2String(lines, ","));
    applied = want;
}

showColours()
{
    integer c;
    for (c = 0; c < 4; ++c)
    {
        if (isLocked(c)) llSetLinkColor(llList2Integer(ringLink, c), <1.0, 0.0, 0.0>, ALL_SIDES);   // red: locked
        else llSetLinkColor(llList2Integer(ringLink, c), <0.0, 1.0, 0.0>, ALL_SIDES);              // green: free
        if (leashIndex(c) >= 0) llSetLinkColor(llList2Integer(pointLink, c), <1.0, 0.8, 0.0>, ALL_SIDES);  // gold: leash held
        else llSetLinkColor(llList2Integer(pointLink, c), <0.6, 0.6, 0.6>, ALL_SIDES);
    }
    if (hanging()) llSetLinkColor(ceilingLink, <1.0, 0.0, 0.0>, ALL_SIDES);
    else llSetLinkColor(ceilingLink, <0.6, 0.6, 0.6>, ALL_SIDES);
    if (locked) llSetColor(<1.0, 0.0, 0.0>, ALL_SIDES);
    else llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES);
}

updateHang()
{
    integer want = hangingByWrist();
    if (want == playing) return;
    if (!mayAnimate) return;
    if (want) llStartAnimation("cuffs_hang");
    else llStopAnimation("cuffs_hang");
    playing = want;
}

// keeps the state in Linkset Data, so it comes back after a reset (saving a
// script in Second Life resets it) or when the set is rezzed again
save()
{
    list c;
    integer i;
    for (i = 0; i < llGetListLength(chains); i += 3)
        c += [llList2String(chains, i) + "-" + llList2String(chains, i + 1)];
    list l;
    for (i = 0; i < llGetListLength(leashes); i += 4)
        l += [llDumpList2String(llList2List(leashes, i, i + 2), "|")];
    llLinksetDataWrite("cuffs", llDumpList2String([locked, llDumpList2String(c, " "), llDumpList2String(l, " ")], ";"));
}

restore()
{
    string data = llLinksetDataRead("cuffs");
    if (data == "") return;
    list parts = llParseStringKeepNulls(data, [";"], []);
    locked = (integer)llList2String(parts, 0);
    list c = llParseString2List(llList2String(parts, 1), [" "], []);
    integer i;
    for (i = 0; i < llGetListLength(c); ++i)
    {
        list ab = llParseString2List(llList2String(c, i), ["-"], []);
        addChain((integer)llList2String(ab, 0), (integer)llList2String(ab, 1), TRUE);
    }
    list l = llParseString2List(llList2String(parts, 2), [" "], []);
    for (i = 0; i < llGetListLength(l); ++i)
    {
        list one = llParseStringKeepNulls(llList2String(l, i), ["|"], []);
        addLeash((integer)llList2String(one, 0), (key)llList2String(one, 1), llList2String(one, 2), TRUE);
    }
    llOwnerSay("Restored: " + status());
}

changedNow()
{
    showColours();
    applyRlv();
    updateHang();
    save();
}

string listNames(list cuffs)
{
    if (cuffs == []) return "none";
    return llDumpList2String(cuffs, ", ");
}

string status()
{
    list l;
    integer c;
    for (c = 0; c < 4; ++c)
        if (isLocked(c)) l += [pointName(c)];
    list ch;
    integer i;
    for (i = 0; i < llGetListLength(chains); i += 3)
        ch += [pointName(llList2Integer(chains, i)) + " - " + pointName(llList2Integer(chains, i + 1))];
    list le;
    for (i = 0; i < llGetListLength(leashes); i += 4)
        le += [pointName(llList2Integer(leashes, i)) + " (" + llList2String(leashes, i + 2) + ")"];
    string h = "no";
    if (hanging()) h = "yes";
    return "Locked: " + listNames(l) + ". Chains: " + listNames(ch) + ". Leash: " + listNames(le) + ". Hanging: " + h + ".";
}

menu(string message, list buttons, string next)
{
    step = next;
    llDialog(menuUser, message, buttons, DIALOG);
}

mainMenu()
{
    menu("Cuff set\n" + status(), ["Lock", "Unlock", "Chain", "Unchain", "Leash", "Unleash", "Hang", "Release", "Free all"], "main");
}

list cuffButtons() { return ["Wrist L", "Wrist R", "Ankle L", "Ankle R"]; }

setLock(integer cuff, integer on)
{
    if (on) locked = locked | (1 << cuff);
    else locked = locked & ~(1 << cuff);
}

freeAll()
{
    while (llGetListLength(chains)) removeChainAt(0);
    while (llGetListLength(leashes)) removeLeashAt(0);
    locked = 0;
    changedNow();
    llOwnerSay("Everything is free.");
}

answer(string button, string who)
{
    integer p = llListFindList(NAMES, [button]);
    if (button == "Back") { mainMenu(); return; }
    if (step == "main")
    {
        if (button == "Lock") menu("Lock which cuff?", cuffButtons() + ["All", "Back"], "lock");
        else if (button == "Unlock") menu("Unlock which cuff?", cuffButtons() + ["All", "Back"], "unlock");
        else if (button == "Chain") menu("Chain from where?", NAMES + ["Back"], "chain1");
        else if (button == "Unchain")
        {
            if (chains == []) { llOwnerSay("There are no chains."); mainMenu(); return; }
            list b;
            integer i;
            for (i = 0; i < llGetListLength(chains); i += 3)
                b += [pointName(llList2Integer(chains, i)) + " - " + pointName(llList2Integer(chains, i + 1))];
            menu("Take off which chain?", b + ["All chains", "Back"], "unchain");
        }
        else if (button == "Leash") menu("Put the leash on which cuff? You hold it.", cuffButtons() + ["Back"], "leash");
        else if (button == "Unleash")
        {
            if (leashes == []) { llOwnerSay("There is no leash."); mainMenu(); return; }
            list b;
            integer i;
            for (i = 0; i < llGetListLength(leashes); i += 4) b += [pointName(llList2Integer(leashes, i))];
            menu("Take the leash off which cuff?", b + ["Back"], "unleash");
        }
        else if (button == "Hang")
        {
            addChain(0, CEILING, TRUE);
            addChain(1, CEILING, TRUE);
            if (hangingByWrist()) llOwnerSay("Hanging by the wrists from the ceiling.");
            else llOwnerSay("No free chain point on the wrists. Unchain something first.");
            changedNow();
            mainMenu();
        }
        else if (button == "Release")
        {
            if (!hanging()) llOwnerSay("Nothing hangs from the ceiling.");
            else
            {
                integer i = 0;
                while (i < llGetListLength(chains))
                {
                    if (llList2Integer(chains, i + 1) == CEILING) removeChainAt(i);
                    else i += 3;
                }
                llOwnerSay("Released from the ceiling.");
                changedNow();
            }
            mainMenu();
        }
        else if (button == "Free all") { freeAll(); mainMenu(); }
        return;
    }
    if (step == "lock" || step == "unlock")
    {
        integer on = step == "lock";
        string word = "unlocked";
        if (on) word = "locked";
        if (button == "All")
        {
            integer c;
            for (c = 0; c < 4; ++c) setLock(c, on);
            llOwnerSay("All cuffs are " + word + ".");
        }
        else if (p >= 0 && p < 4)
        {
            setLock(p, on);
            llOwnerSay(button + " is " + word + ".");
        }
        changedNow();
        mainMenu();
        return;
    }
    if (step == "chain1" && p >= 0)
    {
        first = p;
        list b;
        integer i;
        for (i = 0; i <= CEILING; ++i)
            if (i != p) b += [pointName(i)];
        if (p == CEILING) b = cuffButtons();
        menu("Chain " + button + " to where?", b + ["Back"], "chain2");
        return;
    }
    if (step == "chain2" && p >= 0)
    {
        if (addChain(first, p, FALSE)) changedNow();
        mainMenu();
        return;
    }
    if (step == "unchain")
    {
        if (button == "All chains")
        {
            while (llGetListLength(chains)) removeChainAt(0);
            llOwnerSay("All chains are off.");
        }
        else
        {
            list ab = llParseString2List(button, [" - "], []);
            integer i = chainIndex(llListFindList(NAMES, llList2List(ab, 0, 0)), llListFindList(NAMES, llList2List(ab, 1, 1)));
            if (i >= 0)
            {
                removeChainAt(i);
                llOwnerSay("Unchained " + button + ".");
            }
        }
        changedNow();
        mainMenu();
        return;
    }
    if (step == "leash" && p >= 0 && p < 4)
    {
        if (addLeash(p, menuUser, who, FALSE)) changedNow();
        mainMenu();
        return;
    }
    if (step == "unleash" && p >= 0 && p < 4)
    {
        integer i = leashIndex(p);
        if (i >= 0)
        {
            removeLeashAt(i);
            llOwnerSay("Leash off " + button + ".");
            changedNow();
        }
        mainMenu();
        return;
    }
    mainMenu();
}

default
{
    state_entry()
    {
        integer c;
        ringLink = [];
        pointLink = [];
        for (c = 0; c < 4; ++c)
        {
            ringLink += [linkOf(llList2String(RINGS, c), llList2Integer([9, 10, 2, 3], c))];
            pointLink += [linkOf(llList2String(POINTS, c), llList2Integer([7, 8, 5, 6], c))];
        }
        ceilingLink = linkOf("Ceiling_ring", 4);
        llListen(DIALOG, "", NULL_KEY, "");
        llRequestPermissions(llGetOwner(), PERMISSION_TRIGGER_ANIMATION);
        restore();
        showColours();
        applyRlv();
        llOwnerSay("Cuff set ready. Touch a cuff or the box.");
    }

    run_time_permissions(integer perms)
    {
        mayAnimate = perms & PERMISSION_TRIGGER_ANIMATION;
        updateHang();
    }

    attach(key id)
    {
        // worn again (also after a relog): the viewer has forgotten the restrictions, so send them all again
        if (id == NULL_KEY) return;
        applied = [];
        applyRlv();
        playing = FALSE;
        updateHang();
    }

    touch_start(integer count)
    {
        menuUser = llDetectedKey(0);
        mainMenu();
    }

    listen(integer channel, string name, key id, string message)
    {
        if (id != menuUser) return;
        answer(message, name);
    }
}
