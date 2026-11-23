// Plain help lines in our own words for the Script Lab editor's suggestions (lane 1, job F2).
// Beginner mode shows one of these instead of the definition's own sentence when the name is
// listed here: what it does in Second Life, one plain sentence, at most 12 words, no word from
// the avoided-word table (docs/TEACHING.md). tests/test_gen_lsl_help.py checks every line.
// Names that are not listed get the first sentence of Linden Lab's description
// (lslhelp_data.inc). English first; translations come through tr() in labsuggest.cpp.
//
// LSL_PLAIN(name, sentence)              a function, event or constant from the definitions
// LSL_PLAIN_TYPE(name, sentence)         a type
// LSL_PLAIN_KEYWORD(name, sentence)      a keyword

// --- types
LSL_PLAIN_TYPE("integer", "A whole number, like 3 or -12.")
LSL_PLAIN_TYPE("float", "A number with a decimal point, like 0.5.")
LSL_PLAIN_TYPE("string", "Text, written between double quotes.")
LSL_PLAIN_TYPE("key", "The ID of an avatar, object or item.")
LSL_PLAIN_TYPE("vector", "Three numbers, like a position or a colour.")
LSL_PLAIN_TYPE("rotation", "Which way something faces, as four numbers.")
LSL_PLAIN_TYPE("list", "Several values in square brackets, in order.")

// --- keywords
LSL_PLAIN_KEYWORD("default", "The state every script starts in.")
LSL_PLAIN_KEYWORD("state", "Names a state, or moves the script to another state.")
LSL_PLAIN_KEYWORD("if", "Runs the next part only when the test is true.")
LSL_PLAIN_KEYWORD("else", "Runs when the test after if was not true.")
LSL_PLAIN_KEYWORD("for", "Repeats a part a set number of times.")
LSL_PLAIN_KEYWORD("while", "Repeats a part as long as the test is true.")
LSL_PLAIN_KEYWORD("do", "Runs a part once, then repeats it while the test is true.")
LSL_PLAIN_KEYWORD("jump", "Goes on at a label in the same event.")
LSL_PLAIN_KEYWORD("return", "Leaves the function or event, maybe with a value.")

// --- events
LSL_PLAIN("state_entry", "Runs when the script starts, or a state begins.")
LSL_PLAIN("state_exit", "Runs just before the script leaves a state.")
LSL_PLAIN("touch_start", "Runs when someone starts clicking the object.")
LSL_PLAIN("touch", "Runs again and again while someone holds the click.")
LSL_PLAIN("touch_end", "Runs when someone lets go of the click.")
LSL_PLAIN("listen", "Runs when chat comes in on a channel the script listens to.")
LSL_PLAIN("timer", "Runs each time the timer set with llSetTimerEvent ends.")
LSL_PLAIN("on_rez", "Runs when the object is rezzed or worn again.")
LSL_PLAIN("attach", "Runs when the object is worn or taken off.")
LSL_PLAIN("changed", "Runs when something about the object changes, like its owner.")
LSL_PLAIN("link_message", "Runs when another script in the object sends a message.")
LSL_PLAIN("run_time_permissions", "Runs when the avatar answers a permission request.")
LSL_PLAIN("dataserver", "Runs when an answer comes back, like a notecard line.")
LSL_PLAIN("sensor", "Runs when a scan finds avatars or objects nearby.")
LSL_PLAIN("no_sensor", "Runs when a scan finds nothing nearby.")
LSL_PLAIN("collision_start", "Runs when something starts bumping into the object.")
LSL_PLAIN("http_response", "Runs when a web request gets its answer.")
LSL_PLAIN("money", "Runs when someone pays the object.")
LSL_PLAIN("control", "Runs when the avatar presses a key the script took.")
LSL_PLAIN("object_rez", "Runs when the object rezzes another object.")

// --- chat and messages
LSL_PLAIN("llOwnerSay", "Sends a chat line only the owner sees. RLV lines use it.")
LSL_PLAIN("llSay", "Says a chat line out loud, heard within 20 metres.")
LSL_PLAIN("llWhisper", "Whispers a chat line, heard within 10 metres.")
LSL_PLAIN("llShout", "Shouts a chat line, heard within 100 metres.")
LSL_PLAIN("llRegionSay", "Sends a chat line to the whole region on a channel.")
LSL_PLAIN("llRegionSayTo", "Sends a chat line to one avatar or object in the region.")
LSL_PLAIN("llInstantMessage", "Sends a private message to one avatar.")
LSL_PLAIN("llListen", "Starts listening to chat on a channel.")
LSL_PLAIN("llListenRemove", "Stops a listen that llListen started.")
LSL_PLAIN("llDialog", "Shows a blue menu with buttons to an avatar.")
LSL_PLAIN("llTextBox", "Shows a box where an avatar can type an answer.")
LSL_PLAIN("llMessageLinked", "Sends a message to other scripts in the same object.")
LSL_PLAIN("llSetText", "Shows floating text above the object.")

// --- owner, avatars and touches
LSL_PLAIN("llGetOwner", "Gives the ID of the object's owner.")
LSL_PLAIN("llGetKey", "Gives the ID of this object.")
LSL_PLAIN("llDetectedKey", "Gives the ID of who touched or was found.")
LSL_PLAIN("llDetectedName", "Gives the name of who touched or was found.")
LSL_PLAIN("llKey2Name", "Gives the name for an ID, if they are nearby.")
LSL_PLAIN("llGetDisplayName", "Gives the display name of an avatar in the region.")
LSL_PLAIN("llRequestPermissions", "Asks an avatar for a permission, like playing animations.")
LSL_PLAIN("llGetPermissions", "Gives the permissions the script has now.")
LSL_PLAIN("llStartAnimation", "Starts an animation on the avatar who gave permission.")
LSL_PLAIN("llStopAnimation", "Stops an animation on the avatar who gave permission.")
LSL_PLAIN("llTakeControls", "Takes the avatar's movement keys for the script.")
LSL_PLAIN("llGetAttached", "Tells where the object is worn, or 0 if not.")
LSL_PLAIN("llAttachToAvatar", "Wears the object on the avatar who gave permission.")
LSL_PLAIN("llDetachFromAvatar", "Takes the worn object off the avatar.")

// --- time
LSL_PLAIN("llSetTimerEvent", "Starts a timer that runs the timer event every few seconds.")
LSL_PLAIN("llSleep", "Waits a number of seconds before going on.")
LSL_PLAIN("llGetTime", "Gives the seconds since the script started or was reset.")
LSL_PLAIN("llResetTime", "Sets the script's own stopwatch back to zero.")
LSL_PLAIN("llGetUnixTime", "Gives the current time as a count of seconds.")
LSL_PLAIN("llResetScript", "Starts the script again from the beginning.")

// --- the object
LSL_PLAIN("llGetPos", "Gives where the object is in the region.")
LSL_PLAIN("llSetPos", "Moves the object to a new place.")
LSL_PLAIN("llGetRot", "Gives which way the object faces.")
LSL_PLAIN("llSetRot", "Turns the object to face a new way.")
LSL_PLAIN("llSetAlpha", "Makes the object more or less see-through.")
LSL_PLAIN("llSetColor", "Changes the colour of the object.")
LSL_PLAIN("llSetLinkPrimitiveParamsFast", "Changes many looks of a prim in the object at once.")
LSL_PLAIN("llGetObjectName", "Gives the name of this object.")
LSL_PLAIN("llSetObjectName", "Gives this object a new name.")
LSL_PLAIN("llDie", "Deletes the object for good.")
LSL_PLAIN("llRezObject", "Rezzes an object from this object's inventory.")
LSL_PLAIN("llPlaySound", "Plays a sound from the object.")
LSL_PLAIN("llGetInventoryName", "Gives the name of an item in the object's inventory.")
LSL_PLAIN("llGiveInventory", "Gives an item from the object to an avatar.")
LSL_PLAIN("llGetNotecardLine", "Reads one line of a notecard; the answer comes in dataserver.")
LSL_PLAIN("llSensor", "Scans once for avatars or objects nearby.")
LSL_PLAIN("llSensorRepeat", "Scans again and again for avatars or objects nearby.")
LSL_PLAIN("llSensorRemove", "Stops the repeated scan.")
LSL_PLAIN("llHTTPRequest", "Asks a web address for something; the answer comes later.")

// --- text and lists
LSL_PLAIN("llStringLength", "Gives how many letters a text has.")
LSL_PLAIN("llGetSubString", "Gives a part of a text.")
LSL_PLAIN("llSubStringIndex", "Finds where a text starts inside another text.")
LSL_PLAIN("llToLower", "Gives the text in small letters.")
LSL_PLAIN("llToUpper", "Gives the text in capital letters.")
LSL_PLAIN("llParseString2List", "Cuts a text into a list at the given marks.")
LSL_PLAIN("llDumpList2String", "Joins a list into one text.")
LSL_PLAIN("llGetListLength", "Gives how many values a list has.")
LSL_PLAIN("llList2String", "Gives one value of a list as text.")
LSL_PLAIN("llList2Integer", "Gives one value of a list as a whole number.")
LSL_PLAIN("llListFindList", "Finds where values are in a list, or -1.")

// --- numbers
LSL_PLAIN("llFrand", "Gives a random number between 0 and the number given.")
LSL_PLAIN("llRound", "Rounds a number to the nearest whole number.")
LSL_PLAIN("llFloor", "Rounds a number down to a whole number.")
LSL_PLAIN("llAbs", "Gives a whole number without its minus sign.")
LSL_PLAIN("llVecDist", "Gives the distance between two positions in metres.")

// --- constants
LSL_PLAIN("TRUE", "Yes: the number 1.")
LSL_PLAIN("FALSE", "No: the number 0.")
LSL_PLAIN("PI", "The number pi, about 3.14.")
LSL_PLAIN("ZERO_VECTOR", "A vector with all three numbers at zero.")
LSL_PLAIN("ZERO_ROTATION", "Facing the starting way: no turn at all.")
LSL_PLAIN("NULL_KEY", "An empty ID that points to nothing.")
LSL_PLAIN("PUBLIC_CHANNEL", "Channel 0, the open chat everyone nearby can read.")
LSL_PLAIN("DEBUG_CHANNEL", "The channel for script errors.")
LSL_PLAIN("EOF", "What a notecard read gives after the last line.")
LSL_PLAIN("ALL_SIDES", "Means every side of the prim at once.")
LSL_PLAIN("PERMISSION_TRIGGER_ANIMATION", "Asks to play animations on the avatar.")
LSL_PLAIN("PERMISSION_TAKE_CONTROLS", "Asks to take the avatar's movement keys.")
LSL_PLAIN("CHANGED_OWNER", "In changed: the object got a new owner.")
LSL_PLAIN("CHANGED_INVENTORY", "In changed: the object's inventory changed.")
