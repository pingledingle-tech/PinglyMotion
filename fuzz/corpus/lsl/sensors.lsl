default
{
    state_entry()
    {
        llSensor("", NULL_KEY, AGENT | ACTIVE | PASSIVE | SCRIPTED, 1.0e9, 1.0e9);
        llSensor("s", "not a key", -1, -5.0, -1.0);
        llSensorRepeat("", NULL_KEY, AGENT, 96.0, PI, 0.0001);
        llSensorRepeat("", NULL_KEY, SCRIPTED, 10.0, 0.0, 1.0e-30);
        llSetRot(<1, 2, 3, 4>);
    }
    sensor(integer n)
    {
        integer i;
        for (i = -1; i < n + 1; ++i)
            llOwnerSay(llDetectedName(i) + (string)llDetectedKey(i) + (string)llDetectedPos(i) + (string)llDetectedRot(i)
                       + (string)llDetectedVel(i) + (string)llDetectedType(i) + (string)llDetectedGroup(i) + (string)llDetectedOwner(i));
        llSensor("", NULL_KEY, AGENT, 20.0, PI);
    }
    no_sensor() { llSensorRemove(); llSensorRepeat("", NULL_KEY, AGENT, 5.0, PI, 0.5); }
    touch_start(integer n) { state other; }
}
state other { sensor(integer n) {} state_entry() { llSensorRepeat("", NULL_KEY, AGENT, 5.0, PI, 0.5); } }
