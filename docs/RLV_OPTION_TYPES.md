# RLV option value types (Script Lab)

What the option after ":" in an RLV command must be, how RLVa checks it, and what the
Script Lab sandbox does with a value that does not fit. Script Lab lane 1, job C
(approved by the project owner on 23 November 2026).

Source: Firestorm's RLVa, `FirestormViewer/phoenix-firestorm` commit `e5a7184ae263`
(23 November 2026), read file by file. The same commit gives `tests/data/rlv/rlva.tsv`
(`tools/rlv_sources.py`); its names and parameter types are the same as in the earlier
mirror commit, only the option notes are new. Nothing is copied: the readers in
`src/scriptlab/rlvbehaviours.cpp` are written anew and follow the rules below.

## What RLVa does with a bad value

Confirmed in `indra/newview/rlvhandler.cpp`:

- **Add (`=n`, `=add`).** `RlvHandler::processCommand()` first adds the command to the
  object, then calls `processAddRemCommand()`. When that returns a failure (a bad option
  gives `RLV_RET_FAILED_OPTION`), it removes the command again, and the object too when
  it holds nothing else. So nothing changes and the command is reported as failed.
- **Remove (`=y`, `=rem`).** `processCommand()` looks for the same command text on the
  object. A command with a bad option was never added, so nothing is found and the
  answer is `RLV_RET_SUCCESS_UNSET`. The handler is not called.
- **Force (`=force`).** `processForceCommand()` calls the force handler, which returns
  `RLV_RET_FAILED_OPTION` for a bad option before it does anything.

The sandbox does the same (`RlvState::run()`, `Rlv::optionProblem()`): an add or force
with a bad value is `failed` and changes nothing (no restriction, no recorded value, no
`changed` signal); a remove with a bad value is `unset`. The reason is a plain-words
warning that names the command, the value and the fix, for example:

```
@detach:x=n: "x" is not an attachment point. Use a name such as spine or left hand.
```

The warning goes to the Script Lab log line of the command and to `rlv_state` and
`lsl_run` answers as `reason`. A warning never stops the script: the other commands of
the line and the rest of the script run on. The static check (`lsl_lint`) gives the same
warning, with the id `rlv-param`, for constant `llOwnerSay("@...")` text; since lane 1
job R3 its message starts with the command, as above (`Rlv::plainProblem()`). The script
editor shows these warnings in the script (wavy line, icon, the text in the problem list)
and uses the same value type for its suggestions after `:`: the names for `AttachPoint`,
`Layer` and `CameraMode`, and one sample value with a plain sentence for the other types
([SCRIPT_LAB.md](SCRIPT_LAB.md), "RLV in the editor").

Only RLVa mode checks values. In plain RLV mode the sandbox accepts any value as before,
because the RLV viewer's own handling of bad values is not confirmed (no source read).

## The value types

Each behaviour has its value type in the behaviour table, `Rlv::Behaviour::value`, one
for the restriction forms (`n`, `y`, `add`, `rem`) and one for `=force`
(`Rlv::OptionValue`). `Rlv::valueType()`, `Rlv::optionFits()` and `Rlv::optionProblem()`
read it, so the editor can use the same field for suggestions and warnings. An empty
option is never a value problem; whether an option is needed is the table's
`OptionUse`, as before.

| Type | What fits | RLVa function |
|---|---|---|
| `Unchecked` | any text | (see "Not checked") |
| `None` | no option at all | `RlvBehaviourGenericHandler<RLV_OPTION_NONE>::onCommand()` |
| `Uuid` | 36 characters, hex digits with any character at places 9, 14, 19 and 24; also the old 35 character form without the last separator | `RlvBehaviourGenericHandler<RLV_OPTION_EXCEPTION>` with `RlvCommandOptionHelper::parseOption<LLUUID>()`, which is `LLUUID::set(text, false)` (`indra/llcommon/lluuid.cpp`) |
| `TextureUuid` | as `Uuid`, spaces around it allowed | `RlvBehaviourModifier::convertOptionValue()` with `LLUUID::parseUUID()` (trims, then `LLUUID::validate()`) |
| `UuidOrDistance` | a key, or `<min>[;<max>]` metres, each a number of 0 or more, at most two parts | `RlvBehaviourRecvSendStartIMHandler::onCommand()` |
| `UuidOrNumber` | a key, or a number | `RlvBehaviourHandler<RLV_BHVR_SHOWNAMETAGS>::onCommand()` |
| `Number` | a decimal number at the start; text after it is ignored; out of range fails | `std::stof` in `convertOptionValue()` and `parseOption<float>()` |
| `Integer` | a whole number at the start; out of the int range fails | `std::stoi` in `convertOptionValue()` |
| `Channel` | a whole number of 1 or more | `RlvBehaviourSendChannelHandler::onCommand()` (`parseOption<int>()`, then `> 0`) |
| `ReplyChannel` | a whole number from 1 to 2147483646 (2147483647 is the debug channel) | `processAddRemCommand()`, case `RLV_BHVR_REDIRCHAT`: `LLStringUtil::convertToS32()` and `RlvUtil::isValidReplyChannel()` |
| `Vector3` | three numbers `x/y/z`; text after the last is ignored | `sscanf("%f/%f/%f")` in `convertOptionValue()` and `parseOption<LLVector3>()` |
| `Vector4` | four numbers `a/b/c/d` | `sscanf("%f/%f/%f/%f")` in `convertOptionValue()` |
| `AttachPoint` | one of the 55 attachment point names of the viewer's avatar (`avatar_lad.xml`, lower case), or `root` | `RlvAttachPtLookup::getAttachPointIndex()` (`rlvlocks.h`, `rlvlocks.cpp`) |
| `Layer` | one of the 17 wearable type names, `universal` included | `parseOption<LLWearableType::EType>()`, `LLWearableType::typeNameToType()` |
| `CameraMode` | `mouselook`, `thirdperson` or `reset` | `RlvForceHandler<RLV_BHVR_SETCAM_MODE>::onCommand()` |
| `Notify` | `<channel>[;filter]` | checked by `RlvState` itself, as before (see "Known differences") |

Which behaviours have which type (the `_sec` forms share the type of their base, as in
RLVa, where they use the same handler):

- **Uuid** (restriction): `accepttp`, `accepttprequest`, `edit`, `editobj`, `recvchat`,
  `recvchatfrom`, `recvemote`, `recvemotefrom`, `recvimfrom`, `sendimto`, `share`,
  `showhovertext`, `shownames`, `startimto`, `touchattachother`, `touchhud`, `touchthis`,
  `touchworld`, `tplure`, `tprequest`. Force: `sit` (no option sits on the ground).
- **TextureUuid**: `setcam_textures`, `camtextures` (restriction); `setoverlay_texture` (force).
- **UuidOrDistance**: `recvim`, `sendim`, `startim`. **UuidOrNumber**: `shownametags`.
- **Number** (restriction): `fartouch`, `touchfar`, `sittp`, `tplocal`, `setcam_avdist`,
  `setcam_avdistmin`, `setcam_avdistmax`, `setcam_origindistmin`, `setcam_origindistmax`,
  `camavdist`, `camdistmin`, `camdistmax`, `setcam_fovmin`, `setcam_fovmax`, `camzoommin`,
  `camzoommax`, `setcam_eyeoffsetscale`. Force: `setcam_eyeoffsetscale`, `setcam_fov`,
  `setsphere_distmin`, `setsphere_distmax`, `setsphere_tween`, `setsphere_valuemin`,
  `setsphere_valuemax`, `setoverlay_alpha`.
- **Integer** (force): `setsphere_mode`, `setsphere_origin`, `setsphere_distextend`.
- **Channel**: `sendchannel`, `sendchannel_except`. **ReplyChannel**: `redirchat`, `rediremote`.
- **Vector3**: `setcam_eyeoffset` (both forms); force: `setcam_focusoffset`,
  `setsphere_color`, `setoverlay_tint`. **Vector4**: no row yet (see `setsphere_param`
  under "Known differences").
- **AttachPoint** (restriction): `detach`, `addattach`, `remattach`.
- **Layer** (restriction): `addoutfit`, `remoutfit`. **CameraMode** (force): `setcam_mode`.
- **None** (restriction; RLVa's dictionary line is `RLV_OPTION_NONE`, while the sandbox
  table allows an option for the plain RLV form or for `=force`): `sit`, `viewnote`,
  `viewscript`, `viewtexture`, `setgroup`, `pay`.

`tests/tst_rlvsources.cpp` checks every generic RLVa row against the table: the
`option-*` notes of `rlva.tsv` (the `RLV_OPTION_*` template argument) and the
`value-*` notes (the `typeid` of a local modifier). `tests/tst_rlvoptions.cpp` checks
every type with a value that fits and one that does not, the exact warning and the
state after a refused command.

## Not checked (not confirmed)

These keep today's behaviour (any text), because RLVa reads the value from the avatar,
the inventory or another part the sandbox does not have, or the rule was not confirmed:

- Folder paths and shared folder names: `attach*`, `detachall`, the `attachthis` /
  `detachthis` family and their `_except` forms, `addoutfit*` force forms.
- The force forms of `detach`, `remattach`, `remoutfit` and `addoutfit`: RLVa also takes
  a shared folder, an attachment point group or an object key there.
- `setgroup=force` (a group name or key), `tpto`, `adjustheight`, `setcam_focus`,
  `setoverlay_tween` (several parts), the `setdebug_` and `setenv_` families (the type
  depends on the setting).
- Every question (`=<channel>`): `getstatus` filters, `getattach` and `getoutfit` names,
  `getinv` paths and so on.
- Exact edge cases of the C library: a number with only an exponent mark (`1e/2/3`),
  hex floats inside a vector, numbers out of range inside a vector (`sscanf` is
  undefined there). The sandbox reads numbers like `strtof`, without the locale.

## Known differences

Found while reading the source; not changed in this job:

- An empty option for `RLV_OPTION_EXCEPTION` behaviours (`@sendimto=n`): RLVa accepts it
  as the null key, the sandbox says the option is needed.
- `touchattach`: RLVa takes no option or a key (an exception); the sandbox table takes no
  option.
- `fly=force`: RLVa takes `true` or `false` (or 1 or 0) as option; the sandbox table takes no option.
- `setcam_eyeoffset`, `setcam_eyeoffsetscale`, `setcam_focusoffset`, `setcam_fovmin`,
  `setcam_fovmax`: RLVa's restriction form needs an option; the sandbox accepts none,
  because the table's `OptionUse` is shared with the force form.
- `setcam_focusoffset` restriction: its RLVa modifier holds an `LLVector3d`, a type
  `convertOptionValue()` does not handle, so RLVa seems to refuse every value; the
  sandbox keeps any text.
- `notify`: RLVa reads the channel with `convertToS32()` (so `1.5` is channel 1), refuses
  channel 2147483647 and more than one `;`. The sandbox keeps its own stricter digits
  rule, which the reply channel depends on.
- `setsphere_*` and `setoverlay_*` force forms need the base behaviour (`setsphere`,
  `setoverlay`) held by the same object in RLVa (`RlvBehaviourInfo::processModifier()`);
  the sandbox does not ask for it.
- `setsphere_param=force`: RLVa's modifier is an `LLVector4`, so it needs exactly four
  numbers (`8/6/0/0`); `8/6` or `9` fail there. The sandbox's sphere view reads one to four
  numbers, and its hint for pixel blocks says `setsphere_param:<width>/<height>`. The value
  check is left off for this row until the sphere view and its hint text follow RLVa (that
  touches the translation file, which another job holds now).
- The camera, environment and view mode boxes read a value as a whole (`5m` is "not a
  distance"), while RLVa reads the number at the start (`5m` is 5).
- `@getattach` lists the 40 points of the sandbox's own list (`Rlv::attachmentPoints()`),
  not the 55 RLVa knows.
