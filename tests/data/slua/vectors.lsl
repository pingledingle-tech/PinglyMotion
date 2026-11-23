// vector and rotation members, dot and cross products
vector pos = <1, 2, 3>;
rotation rot;

default
{
    state_entry()
    {
        pos.x = 4.0;
        pos.z += 1.5;
        rot.s = 1.0;
        vector other = llGetPos();
        float dot = pos * other;
        vector cross = pos % other;
        vector turned = pos * rot;
        rot = llEuler2Rot(<0.0, 0.0, PI / 2>) * rot;
        if (pos)
            llOwnerSay((string)dot + (string)cross + (string)turned + (string)rot.z);
        llSetPos(pos + <0.0, 0.0, -1.0>);
    }
}
