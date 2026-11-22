default
{
    state_entry()
    {
        llSitTarget(<0, 0, 0.5>, <0, 0, 0, 0>);
        llLinkSitTarget(5, <1000, 0, 0>, ZERO_ROTATION);
        llSetSitText("Pose");
    }
    changed(integer c)
    {
        key av = llAvatarOnSitTarget();
        if (c & CHANGED_LINK)
        {
            if (av != NULL_KEY)
            {
                llRequestPermissions(av, PERMISSION_TRIGGER_ANIMATION | PERMISSION_TAKE_CONTROLS);
                llOwnerSay((string)llGetAgentInfo(av) + llGetAnimation(av) + (string)llGetNumberOfPrims());
            }
            else llUnSit(llAvatarOnLinkSitTarget(0));
        }
    }
    run_time_permissions(integer p)
    {
        key av = llGetPermissionsKey();
        if (p & PERMISSION_TRIGGER_ANIMATION)
        {
            llStartAnimation("pose");
            llOwnerSay(llDumpList2String(llGetAnimationList(av), ","));
            llStopAnimation("pose");
        }
    }
    touch_start(integer n) { llUnSit(llDetectedKey(0)); }
}
