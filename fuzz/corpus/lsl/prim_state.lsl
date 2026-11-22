default
{
    state_entry()
    {
        llSetText("hover", <2, -1, 0.5>, 9.0);
        llSetAlpha(0.5, ALL_SIDES);
        llSetColor(<1, 0, 0>, 99);
        llSetTexture("tex", -7);
        llSetPrimitiveParams([PRIM_COLOR, ALL_SIDES, <0, 1, 0>, 1.0, PRIM_TEXTURE, 1, "t", <1, 1, 0>, <0, 0, 0>, 0.0,
                              PRIM_POSITION, <300, 300, 5000>, PRIM_ROTATION, <0, 0, 0, 0>, PRIM_SIZE, <0, 100, 1>, PRIM_LINK_TARGET, 2,
                              PRIM_NAME, "n", PRIM_DESC, "d", PRIM_FULLBRIGHT, 3, TRUE, PRIM_GLOW, 0, 5.0]);
        llSetLinkPrimitiveParamsFast(LINK_SET, [PRIM_TEXT, "all", <1, 1, 1>, 1.0, 12345]);
        llOwnerSay(llList2CSV(llGetLinkPrimitiveParams(LINK_SET, [PRIM_TEXT])));
        llOwnerSay(llList2CSV(llGetLinkPrimitiveParams(2, [PRIM_COLOR, ALL_SIDES, PRIM_TEXTURE, 0, PRIM_BUMP_SHINY, 0])));
        llSetPos(<1000, 1000, 1000>);
        llSetRot(<0, 0, 0, 0>);
        llOwnerSay((string)llGetPos() + (string)llGetRot() + (string)llGetLocalRot() + (string)llGetLocalPos());
        llSetStatus(-1, TRUE);
        llOwnerSay((string)llGetStatus(STATUS_PHYSICS));
        llSetSitText("sit");
        llTextBox(NULL_KEY, "name?", 5);
        llDialog(NULL_KEY, "again", ["!!llTextBox!!"], 6);
        llRequestPermissions(llGetOwner(), PERMISSION_DEBIT);
        llTransferLindenDollars(llGetOwner(), 5);
        llParticleSystem([1, 2]);
        llTargetOmega(<0, 0, 1>, 1.0, 1.0);
        llLoadURL(NULL_KEY, "text", "url");
    }
    transaction_result(key id, integer ok, string data)
    {
        llOwnerSay(data);
    }
}
