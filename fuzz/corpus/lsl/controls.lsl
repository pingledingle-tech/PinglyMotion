default
{
    state_entry()
    {
        llRequestPermissions(llGetOwner(), PERMISSION_TAKE_CONTROLS);
        llTakeControls(-1, -1, 0);
    }
    run_time_permissions(integer p)
    {
        llTakeControls(CONTROL_FWD | CONTROL_BACK | CONTROL_ROT_LEFT, TRUE, FALSE);
        llTakeControls(0, FALSE, FALSE);
        llTakeControls(CONTROL_LBUTTON | CONTROL_ML_LBUTTON, TRUE, TRUE);
    }
    control(key id, integer level, integer edge)
    {
        llOwnerSay((string)id + (string)level + (string)edge);
        if (edge & CONTROL_FWD) llReleaseControls();
        llReleaseControls();
    }
}
