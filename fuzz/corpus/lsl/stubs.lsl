default
{
    state_entry()
    {
        llRequestPermissions(llGetOwner(), PERMISSION_OVERRIDE_ANIMATIONS);
        llSetTextureAnim(ANIM_ON | LOOP, ALL_SIDES, 4, 4, 0.0, 16.0, 10.0);
        llTriggerSoundLimited("snd", 1.0, <1, 1, 1>, <0, 0, 0>);
        llLoopSoundMaster("snd", 0.5);
        llSetCameraParams([CAMERA_ACTIVE, 1, CAMERA_DISTANCE, 5.0]);
        llSetCameraEyeOffset(<0, 0, 1>);
        llClearCameraParams();
        llMakeFountain(10, 0.1, 1.0, 1.0, 0, 0, "", NULL_KEY, 0.0);
        llLookAt(<1, 2, 3>, 1.0, 1.0);
        llSetClickAction(CLICK_ACTION_TOUCH);
        llSetPayPrice(PAY_DEFAULT, [1, 2, 3, 4]);
        llOffsetTexture(0.5, 0.5, ALL_SIDES);
        llRotateTexture(PI, -1);
        llSetAnimationOverride("Walking", "walk");
        llSetAnimationOverride("Hopping", "x");
        llOwnerSay(llGetAnimationOverride("Walking") + llGetAnimationOverride("nope"));
        llResetAnimationOverride("ALL");
        llResetAnimationOverride("Walking");
    }
    touch_start(integer n) { llSetAnimationOverride("Sitting", llGetInventoryName(INVENTORY_ANIMATION, 0)); }
}
