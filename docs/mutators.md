# Mutators

A mutator changes the rules of a game: it sets values of the map's tags, poses
the nodes of its objects' models, changes how units die, adds weapons and
vehicles, and runs a script. Mods (`docs/mods.md`) change only what one machine plays and shows. A mutator changes what every machine of a
network game must agree on, so a game plays the host's mutators on every
machine.

The game's code is `port/linux/game/mutators.c` (what a game plays),
`mutator_tags.c` (the tag edits), `object_pose.c` (the poses), `explosion_decals.c` (the explosions), `object_death.c`
(the deaths),
`mutator_scripts.c` (the scripts), `network_mutators.c` (the host's mutators
sent to its clients) and
`port/linux/src/port_mutators.c` (the mutators' files). The stock mutators'
files are `port/assets/mutators`.

## What a mutator can do

A mutator is data. It sets values of the map's tags, of the fields that the
game lists (the tag edits, below); it scales and moves the nodes of objects'
models (the poses, below); it can make a unit's death play an effect and its
body vanish (`[death]`, below); its numbers can be scaled by options the host
sets (`[[option]]`, below); it can add a vehicle (`format = 3`) or a weapon
(`format = 4`), with every tag it uses, to the maps it is played on (an
add-on: [addons.md](addons.md)), checked as a map's tags are; and it can run a
Lua script (the scripts, below). The game refuses a mutator with a key, a
group or a field that it does not have, or a value outside its range.

The game has no mutator's rules built in. What it offers is general, a field
of a tag set or a node of a model scaled, so that a player can make a mutator
of their own with no change to the game. The stock big head is a pose of every
biped's head, which any mutator could make.

## Poses

A mutator's `[[pose]]` tables each scale a node of the models of a group of
objects, and move it:

```toml
[[pose]]
objects = "biped"     # the objects' group (below)
tag = 'characters\cyborg\cyborg'   # one tag of it (else every one)
node = "head"         # a node of their models
scale = 2.5           # 0.25 to 4 (else 1)
offset = [0, 0, -0.1] # world units, in the node's parent's frame, -1 to 1 each (else none)
```

The groups are `object`, `unit`, `biped`, `vehicle`, `item`, `weapon`,
`equipment`, `garbage`, `projectile`, `scenery`, `device`, `machine`,
`control`, `light_fixture`, `sound_scenery` and `placeholder`; a parent group
(`unit`, `item`, `device`, `object`) poses every group under it. The node is
named as the models name it (Halo's bipeds' skeletons name theirs `bip01 head`,
`bip01 l upperarm`, `bip01 r thigh`, `bip01 pelvis` and so on; any case), or
`head`: a biped's own head node, whatever its model names it. A node that no
model of the map has is passed over and logged, as a tag edit of a tag that it
has not is. Two poses of one node are both made: their scales multiplied,
their offsets added.

The game poses a node as it makes the object's node orientations, after its
animations and before the matrices of its nodes. The model is drawn with those
matrices, and the collision model is tested with them, so a shot hits a node
where it is drawn, on every machine. What hangs from the node moves and scales
with it: the nodes under it in the skeleton, and its markers. The spheres that
the game looks for an object in (shots, melee, explosions, the host's check of
a client's hit) and that it culls an object by grow to hold the posed nodes
and those under them.

So a longer arm is its upper arm scaled, with its hand scaled back down to its
own size, and shorter legs are the thighs scaled down, with the pelvis moved
down for the feet to reach the ground. A pose changes no physics: a biped's
movement, its standing height and the camera are its own.

A mutator with poses is made for `format = 6`. Limits: 64 kinds of object
posed in a game, and 16 nodes of each.

### Big head

```toml
[[pose]]
objects = "biped"
node = "head"
scale = 2.5
```

Every biped's head, the players' and the AI's, is drawn and hit at two and a
half times its size.

## Tag edits

A mutator's `[[tags]]` tables each name a tag of the map, by its group and
its name, and set some of its fields:

```toml
[[tags]]
group = "weapon"                              # the tag's group (below)
tag = 'weapons\assault rifle\assault rifle'    # its name (a literal string: no escapes)
zoom_level_count = 1                          # a field: a number
zoom_in_sound = 'sound\sfx\weapons\sniper rifle\sniper_2x_zoom'   # a tag's name ('' for none)

[tags.triggers.0]                             # an element of a block: its index
projectile_error_angle_upper_bound = 3        # (angles in degrees)

[[tags]]
group = "projectile"
tag = 'weapons\assault rifle\bullet'
[tags.material_responses.stone]               # ... or its name
potential_response = "reflect"                # one of the field's names

[[tags]]
group = "weapon_hud_interface"
tag = 'weapons\assault rifle\assault rifle'
valid_crosshair_types = ["aim", "zoom"]       # some of the field's names
crosshairs = { from = 'weapons\pistol\pistol', elements = [1, 2], append = true }
```

A copy (`from`) puts the elements of the same block of another tag of the
group in place of the block's own: all of them, or those `elements` lists,
and after the block's own with `append = true`.

The edits are made when the map's tags have loaded, before any of its
objects: by the host, and by a game of one machine, as each game starts, and
by a client when the host's come. A tag, an element or a tag named that the
map does not have is passed over and logged (`debug.txt`), and the console
says how many were; the other edits are made. Every machine of a game plays
one map, so each passes over the same. The next game puts back the map's own
values before it makes its own edits. A tag that an edit only names (its `from`,
or a sound or an effect) and that a Custom Edition map lacks is first looked for
in the stock maps (below). The tag an edit changes is not: a map without a
weapon is not given one to change.

The fields that a mutator can set:

| Group | Field | Value |
| --- | --- | --- |
| `weapon` | `heat_recovery_threshold` | 0 to 1 |
| | `heat_loss_per_second` | 0 to 30 |
| | `zoom_level_count` | 0 to 8 |
| | `zoom_magnification_minimum`, `zoom_magnification_maximum` | 1 to 32 |
| | `zoom_in_sound`, `zoom_out_sound` | a sound's name |
| | `age_heat_recovery_penalty`, `age_misfire_start`, `age_misfire_chance` | 0 to 1 |
| | `age_rate_of_fire_penalty` | 0 to 10 |
| | `active_camo_ding`, `active_camo_regrowth_rate` | 0 to 1 |
| | `magazines.<index>.rounds_total_initial`, `.rounds_total_maximum`, `.rounds_loaded_maximum` | 0 to 32767 rounds |
| | `magazines.<index>.rounds_reloaded`, `.rounds_recharged_per_second` | 0 to 32767 rounds |
| | `magazines.<index>.chamber_time` | 0 to 60 seconds |
| | `triggers.<index>.initial_rate_of_fire`, `.final_rate_of_fire` | 0 to 300 a second |
| | `triggers.<index>.rate_of_fire_acceleration_time`, `..._deceleration_time` | 0 to 60 seconds |
| | `triggers.<index>.error_acceleration_time`, `..._deceleration_time` | 0 to 60 seconds |
| | `triggers.<index>.projectile_error_angle_lower_bound`, `..._upper_bound` | 0 to 180 degrees |
| | `triggers.<index>.heat_generated_per_round`, `.age_generated_per_round` | 0 to 1 |
| | `triggers.<index>.rounds_between_tracers` | 0 to 32767 |
| `projectile` | `model` | a model's name (what it is drawn as) |
| | `maximum_range` | 0 to 10000 world units |
| | `air_gravity_scale`, `water_gravity_scale` | 0 to 10 (1 is the world's gravity) |
| | `air_damage_range_lower_bound`, `..._upper_bound`, `water_damage_range_lower_bound`, `..._upper_bound` | 0 to 10000 world units |
| | `initial_velocity`, `final_velocity` | 0 to 3000 world units a second |
| | `material_responses.<material>.default_response`, `.potential_response` | `disappear`, `detonate`, `reflect`, `overpenetrate`, `attach` |
| | `material_responses.<material>.default_effect`, `.potential_effect` | an effect's name |
| | `material_responses.<material>.potential_flags` | `only_against_units` |
| | `material_responses.<material>.potential_skip_fraction` | 0 to 1 |
| | `material_responses.<material>.potential_angle_lower_bound`, `..._upper_bound` | -90 to 90 degrees |
| `weapon_hud_interface` | `valid_crosshair_types` | `aim`, `zoom`, `charge`, `flash_ammo`, ... (`mutator_tags.c` lists them) |
| | `crosshairs` | copied, 19 elements at most |
| | `screen_effects` | copied, 1 element at most |
| `damage_effect` | `damage_minimum`, `damage_lower_bound`, `damage_upper_bound` | 0 to 10000 |
| | `active_camouflage_damage` | 0 to 1 |
| | `material_modifiers.<material>` | 0 to 10 |
| `decal` | `type` | `scratch`, `splatter`, `burn` or `painted_sign` |
| | `flags` | `color_interpolate_in_hsv`, `color_interpolate_along_farthest_hue_path`, `no_random_rotation`, ... (`mutator_tags.c` lists them): the decal's flags, all of them (those not named are cleared) |
| | `radius_lower_bound`, `radius_upper_bound` | 0 to 10 world units |
| | `intensity_lower_bound`, `intensity_upper_bound` | 0 to 1 |
| | `color_lower_bound.<red, green or blue>`, `color_upper_bound.<...>` | 0 to 1 (each decal is a colour between them, through hue with the HSV flags) |
| | `framebuffer_blend_function` | `alpha_blend`, `multiply`, `double_multiply`, `add`, `subtract`, `component_min`, `component_max`, `alpha_multiply_add` |
| | `map` | a bitmap's name (the splat drawn) |

A block whose elements have names (a projectile's `material_responses`) can
be edited in all of them at once, by the element `all`:
`[tags.material_responses.all]` sets the field in each element the tag has.

`tag = '*'` edits every tag of the group the map has, the add-ons' too, so a
mutator can change any weapon without naming it; a later edit of one tag sets
it again (paintball's stock weapons' speeds, after every projectile's). With
`where`, only those tags whose enum field has one of the values:

```toml
[[tags]]
group = "decal"
tag = '*'
where = { type = ["scratch", "splatter", "burn"] }   # not the map's painted signs
framebuffer_blend_function = "add"
```

Of projectiles, `where = { fired = true }` edits only those a weapon's trigger
fires (an add-on weapon's or vehicle's too): not a grenade, which a unit
throws, nor one a vehicle or an effect throws out by itself (the Falcon's
rotor wash).

A magazine's `rounds_total_initial` is the rounds a weapon is made with, loaded
and in reserve (`rounds_loaded_maximum` of them loaded), and
`rounds_total_maximum` the most it carries in reserve. A weapon placed in a
map keeps the rounds the map gives it, up to the new most. Its reload takes
as long as its animation does: the tag's reload time is not read, and
cannot be set.

A trigger's times are set with the step a tick that the game takes them by
(`1 / (seconds x 30)`, or 1 for 0), as the editing kit's tools work it out.

A material is one of `dirt`, `sand`, `stone`, `snow`, `wood`, `hollow_metal`,
`thin_metal`, `thick_metal`, `rubber`, `glass`, `force_field`, `grunt`,
`hunter_armor`, `hunter_skin`, `elite`, `jackal`, `jackal_energy_shield`,
`engineer`, `engineer_force_field`, `flood_combat_form`,
`flood_carrier_form`, `cyborg`, `cyborg_energy_shield`, `armored_human`,
`human`, `sentinel`, `monitor`, `plastic`, `water`, `leaves`,
`elite_energy_shield`, `ice` or `hunter_shield`.

A mutator with tag edits is made for `format = 2`; one with decal edits, an
`all` element, `tag = '*'`, options, a `[death]` or an `[explosion]`, for
`format = 7`. Limits: 32 KB of edits
in a mutator and in a game (an edit is its names and a few bytes), and 2048
edits made in a game.

### Accurized MA5B

The assault rifle zooms, with the pistol's 2x scope (its crosshairs and its
mask, so a map with the pistol needs nothing more), and the sniper rifle's
zoom sounds. Its bullets spread from 0.05 to 3 degrees (the stock rifle's 2
to 6.5), build up in the stock rifle's 0.6 seconds and settle in 0.1 (the
stock rifle's 1), and a tracer is every other round, not every third. They carry 300 world
units (the stock rifle's 40), at the stock speed and with its gravity, and
ricochet as Combat Revolved's do: off stone, metal and ice at up to 60
degrees from the surface, one time in ten, and always off energy shields,
force fields and a hunter's armor, which still take the bullet's damage.
They pass through flesh as the stock rifle's do. The rest of the rifle (its
damage, rate of fire, magazine, flashlight) is stock.

On a map without the pistol (some campaign levels), the rifle zooms without
the scope's crosshairs and mask; on one without the sniper rifle's zoom
sounds, silently.

### Revolved plasma rifle

The plasma rifle as Combat Revolved has it, without its 2x zoom: it does not
zoom, as the stock rifle does not. It fires 6 rounds a second, rising to 9 in
0.8 seconds (the stock rifle's 7 to 10 in 1.8) and falling back in 0.6; its
bolts spread up to 0.75 degrees (the stock 2.5), building in 0.8 seconds and
settling in 0.6 (the stock 2.5, and at once). Each round heats it 0.0861
(the stock 0.08) and ages it 0.003 (0.005); it cools 0.36 a second (0.3) and
is ready again at 0.15 heat (0.25); its age no longer slows its cooling or
its rate of fire, and it misfires from 0.85 age, a quarter as often at worst
(the stock 0.9, half). Firing costs an active camouflage less and it comes
back faster (0.25 and 0.075, the stock 0.3 and 0.05).

Its bolts fly at 27 world units a second (the stock 50, slowing to 25) and
last 45 world units, doing their damage within 15 to 45 (the stock 20 to
50). Each does 13 damage (the stock 10 to 14), all of it to an active
camouflage (the stock 0.12), with Combat Revolved's modifiers: twice as much
to a jackal's shield (the stock all of it) and an engineer's force field (the
stock none), a quarter to a hunter's armor (the stock half), all of it to a
hunter's skin, cyborg armor and rubber, and half to hollow metal. A hunter's
shield no longer reflects them.

Its melee (Combat Revolved's is 91), the flash of its firing and its HUD are
stock.

### Paintball

GoldenEye's paintball mode, from the maps' own tags. Every weapon's
projectile, an add-on weapon's too (`tag = '*'`, `where = { fired = true }`: 450
feet a second), is drawn
as a ball (the plasma grenade's model) and flies at
350 to 600 feet a second (at 10 feet a world unit: the
shotgun's pellets, rockets, the charged plasma bolt and the tank's shell 350,
needles and the plasma pistol's bolts 400, the assault rifle's bullets, the
plasma rifle's and the Ghost's bolts 450, the pistol's, the Warthog's and the
Scorpion's guns' 500, the sniper rifle's 600), with one and a half times the
world's gravity, until it hits something (a range of 1000 world units: one
with no range and no timer, such as the Warthog's gun's, would vanish once
past its damage's reach, as it no longer slows). Its hit on any material plays its weapon's hit on a grunt,
whose splat (the grunt's blood decal) is drawn with the hunter's glowing
splat, added to what is behind it, in a colour of any hue: a bright splotch
in place of a bullet hole and its flash. Every other mark (bullet holes,
blood, scorches; not the map's painted signs) is painted so too, and an
explosion leaves a splotch as big as its blast (`[explosion]`). A biped that dies plays the co-op
teleport and its body vanishes; its weapons drop, and its player respawns,
as in the stock game.

Its options, in MUTATOR SETTINGS: PAINT SPEED (25 to 200%) and PAINT DROP (0
to 300%), each a percent of the speeds and the drop above.

## Options

A mutator's `[[option]]`s are percents the host sets, in Server Setup's
MUTATOR OPTIONS > SETTINGS (a row each, for the mutators on: left and right
step it), kept in `game.mutator_options`. A tag edit's number names one
instead of being a number, and is that number at 100%, scaled:

```toml
[[option]]
id = "speed"            # what its numbers name it by (no . = or ,)
name = "PAINT SPEED"    # its row's label
default = 100           # whole percents, from minimum to maximum by step
minimum = 25
maximum = 200
step = 25

[[tags]]
group = "projectile"
tag = 'weapons\pistol\bullet'
initial_velocity = { value = 50, option = "speed" }   # 50 world units a second at 100%
```

The number is checked at the option's least and most, and made as each game
starts at the host's percent: a client plays the host's edits, already
scaled. Limits: 4 options a mutator, and 128 numbers they scale.

## Explosions

A mutator's `[explosion]` names a decal every explosion leaves, on every
machine, as big as its damage reaches (its damage effect's cutoff radius),
whatever made it:

```toml
[explosion]
decal = 'effects\decals\blood splats\blood grunt splat'
```

It is placed on what the explosion hit, and on the ground below one that
does not face up (off a wall, or in the air). A shock wave's push, which does
no damage, leaves none.

## Deaths

A mutator's `[death]` changes what a unit does as it dies, on every machine:

```toml
[death]
objects = "biped"                # unit, biped or vehicle (a pose's groups)
effect = 'effects\coop teleport' # played at it (an effect's name), else none
body = "vanish"                  # "vanish": no longer drawn; "stays", the default
```

A body that vanishes is still there for what follows a death (the dead
player's camera, the weapons dropped, the cleanup that takes it away later),
only not drawn. It comes as a tag edit of its own kind, so that a client plays
the host's.

## Scripts

A mutator is still its `mutator.toml`. Its `[script]` table can call a Lua script
beside it, and gives the script its values (`format = 5`):

```toml
name = "Third person"
id = "third_person"
format = 5

[script]
file = "main.lua"
distance = 2.5
height = 0.25
keyboard = "F5"
controller = "DPad Up"
```

A script's control can have a row on Controls Setup, while the mutator is on. Each
key of `[script.menu]` names a text value of `[script]`, and gives its row's label;
up to 6 rows, after the ACTION group's own and then the last group's:

```toml
[script.menu]
keyboard = "THIRD PERSON"
```

A row takes keys and mouse buttons, two at most (`"F5, G"`, which `input.held`
takes as either). It writes to `settings.toml` beside the script, the player's own,
and a running script's `settings` change at once. DEFAULTS shows `[script]`'s.

The script runs on each machine that has the mutator, once as its game's mutators
are set. Its global `frame()` is called once a frame for the first local player, and
its global `tick()` once a game tick. The game's code is
`port/linux/game/mutator_scripts.c`.

`frame()` is what a machine does for itself. `tick()` moves the game: it runs on
every machine, for every player, from the buttons `input.button` sets, which go with
a player's input to the host and on to the others. So a client's player moves at
once, as when it runs, and the host's word corrects it as it corrects any prediction.
Only the host moves what is not a player. What a tick sends a client's player, the
host lets that client say it did, as it does for an explosion's throw, up to the
netcode's most a unit goes (2 world units a tick). A script's state is not in a saved
game, so a script that keeps any should start again if `game.time()` goes back.

| Name | What it does |
| --- | --- |
| `input.held(name)` | Whether a key or mouse button (the names of `controls.*`), or `"DPad Up"`, `"DPad Down"`, `"DPad Left"`, `"DPad Right"`, is held in the game |
| `input.jump()`, `input.crouch()` | Whether the first local player holds jump or crouch, whatever they are bound to, as their unit last took them |
| `input.claim(name)` | Claims a control, by its name (`"DPad Up"`), this frame: the game's own uses of it leave it to the script (D-pad Up opens no text chat) |
| `player.alive()`, `player.in_vehicle()` | The first local player's |
| `camera.free()` | Whether no cinematic or script of the map's has the camera |
| `camera.view{ distance, height, backward, crosshair }` | This frame's view, from the height above the eyes and the distance behind them (or with `backward`, in front, looking back at the player); `crosshair` false draws none |
| `camera.first_person()` | Whether the first local player's view is their own eyes |
| `input.button(n, down)` | Sets the first local player's button `n` (0 to 7), for `player.button` on every machine. Each mutator should use its own |
| `game.time()`, `game.host()`, `game.players()` | The game's tick, whether this machine decides (a local game's does), and the index of each player |
| `player.local_index()` | The first local player's index, or nil |
| `player.button(index, n)` | Whether that player holds button `n` |
| `player.unit(index)` | Their unit, or nil with none or dead |
| `player.aim(index)`, `player.crouch(index)` | Their aiming vector (x, y, z), and how far crouched (0 to 1) |
| `object.position(o)`, `object.velocity(o)` | x, y, z, in world units and world units a tick; nothing if it is gone |
| `object.parent(o)`, `object.kind(o)`, `object.tag(o)`, `object.alive(o)`, `object.acceleration_scale(o)` | Its parent or nil, `"biped"`, `"vehicle"`, `"weapon"` and so on, its tag's name, whether it is undead, and its tag's acceleration scale |
| `object.set_velocity(o, x, y, z)` | In a `tick()`: its velocity, at most 10 on each axis. On a client, only a player's unit |
| `object.turn_around(o)` | In a host's `tick()`: faces it the other way |
| `world.ray(x, y, z, dx, dy, dz, ignore)` | Where that vector (at most 1000 long) first hits the map or an object as a projectile does, not stopping at water: x, y, z and the object or nil; nothing with no hit. `ignore` is an object it passes through |
| `draw.line(x, y, z, x2, y2, z2, r, g, b)` | A line this frame, of 64 at most; in `frame()` |
| `draw.line_from_view(right, down, forward, x, y, z, r, g, b)` | The same, from a place by the camera as this frame draws it |
| `model.create(tag, x, y, z, owner)` | A model of a weapon's, a piece of equipment's, garbage's, a projectile's or scenery's tag (the map's or an add-on's) at the point, made on this machine alone, 16 at most: its handle, or nil. Its creation effect plays and its attachments (lights, looping sounds) are made. `owner` is a unit, whose it is |
| `model.place(m, x, y, z, fx, fy, fz, ux, uy, uz)` | Puts it there, facing forward and up if given, its nodes as its model has them |
| `model.node(m, node, x, y, z)` | Moves one of its nodes there until it is placed again, and that node's vertices with it: a rope between two nodes |
| `model.node_from_view(m, node, right, down, forward)` | The same, this frame, from a place by the camera as it is drawn |
| `model.face_view(m)` | Turns its nodes as the camera is, this frame |
| `model.exists(m)`, `model.delete(m)` | Whether it is still there, and takes it away |
| `settings` | The values of `[script]`, then those of a `settings.toml` beside the script, if there is one, over them: the player's own, in no checksum |

The camera used for shooting stays the first-person one, whatever view is drawn. A
scope does not zoom in a script's view.

A model is only what its machine draws: the object is never updated, sent to
another machine, picked up or hit, and moves only as its script puts it. One is
made whatever the game type's weapons are. It is deleted before a game is saved
and when its script stops, so a script whose model is gone (`model.exists`) makes it
again if it still wants it. A grappling hook mutator draws its hook and rope with
them, from its add-on (taken with `--also`: [addons.md](addons.md)).

A client plays the host's mutators: the host sends its files (below, "Network
games"), and a key naming the script by the first digits of the mutator's SHA-256,
which the client runs. One without it plays without it, and `debug.txt` says so.

A script is untrusted input, as a map is. It has Lua's base, string, table and math
libraries, without what loads code (`load`, `dofile`, `loadfile`, `string.dump`) or
reaches files, the system or the network. It has 4 MB of memory and two million
instructions a call, and its file is at most 256 KB. One that fails or passes those
is said in `debug.txt` and stopped until the game is run again.

## Stock tags

A mutator ships no retail data. An add-on names the stock tags it uses by their
groups and names, and a tag edit names those it copies from or refers to. Every
stock map has what its mutators' name, but a Custom Edition map may not, so
such a map is given what it lacks from the player's own stock maps
(`port/linux/game/stock_tags.c`). Nothing of this runs on a stock map, when no
mutator is on, or when the map has every tag asked for: each entry to it returns
at once then.

1. The multiplayer maps are looked in, Blood Gulch first, each read only if the
   ones before it lack the tag. A map's file is inflated, and its tags are read
   to the Xbox tag cache, which a Custom Edition map's tags leave free.
2. The validator checks them all, as when that map is played. A map it refuses,
   or that cannot be read, is passed over for the next. A PAL map's tags are put
   back to the NTSC maps' values, as when it is played.
3. The tag, and each tag it uses that the playing map has not, are copied out,
   by their groups' schemas: a tag the map has, by its group and name, stays the
   map's. Their bitmaps' pixels and sounds' samples are kept in memory, and their
   models given buffers of their own. Either all of them are added, or none is
   (and `debug.txt` says why). A scenario or a structure bsp is never added.
4. Their bitmaps are drawn as the Xbox's are, on a map whose others are Halo
   PC's. All of it goes with the map's tags.

What `debug.txt` says of it, each line starting `stock tags:`:

```
stock tags: the foot 'vehicles\vehicle' from bloodgulch, with 86 tags in all, at 2329
stock tags: no multiplayer map has the snd! 'sound\my\own'
stock tags: 1 tags and 66048 bytes of pixels and samples let go of, read 3 times
```

An add-on's imports are added as the map's tags load, before the add-ons' tags,
so their indices depend on the map and the add-ons on. A tag edit's are added as
the edits are made, 8192 places after the tags before the menus' (whose number
can be one machine's alone), so they too are at the same index on every machine
playing the same edits. On a client, that is as the host's edits come: the game
pauses for as long as a stock map takes to inflate.

A tag named by no multiplayer map is passed over, as an edit's has always been,
or refuses its add-on. A tag only in campaign maps is not looked for. A PAL
map's first-person animations are not paced as they are when it is played.

## The mutators

The mutators are in two folders of the data root (the folder with `maps/`),
each mutator a folder with a `mutator.toml`:

```
stock_mutators/           the game's own: written again at each start
  README.txt
  accurized_ma5b/
    mutator.toml
  big_head/
    mutator.toml
  paintball/
    mutator.toml
  revolved_plasma_rifle/
    mutator.toml
mutators/                 yours: the game never writes here
  huge_heads/
    mutator.toml
```

The stock mutators are embedded in the game (from `port/assets/mutators`,
by `tools/embed_assets.py`). Each time the game looks for the mutators (as
it starts, and as the Mods menu opens), it writes them
into `stock_mutators`, in place of any file there that differs, and plays
them as it has them: a change made in `stock_mutators` is lost, and a folder
added there is not read. So two machines of one build always play the same
stock mutators. Where `stock_mutators` cannot be written (a data root that
cannot be written to), the game plays them all the same, and `debug.txt`
says so.

To make a mutator from a stock one, copy its folder into `mutators` and
change it. It can keep the stock one's `id`: see "Two mutators with one id",
below.

```toml
name = "Huge heads"     # what the Mods menu shows (else the folder's name)
id = "huge_heads"       # what game.mutators names it by (else the folder's name)
version = "1.0"
authors = ["Someone"]   # or one string
description = "Heads at four times their size."
format = 6              # the mutators' format it is made for

[[pose]]                # a node it poses
objects = "biped"
node = "head"
scale = 4
```

Every key but `tags`, `pose`, `addon` or `[script]` is optional. `addon` and
`[[place]]` add a vehicle and place it ([addons.md](addons.md)). A mutator
cannot be played, and the Mods menu says why, if:

- `mutator.toml` is not TOML, or is larger than 64 KB;
- it has a key that the game does not know (at the top, or in a pose);
- it edits no tag, poses nothing and adds nothing;
- it has `[effects]`: the game has no effects built in (a big head of a build
  before 47 is a pose now);
- it poses a group the game does not pose, or with a scale or an offset out of
  its range;
- its add-on or places are not as [addons.md](addons.md) says;
- it edits a group, a field or an element (by its name) that the game does
  not edit, with a value that is not of the field's kind or is out of its
  range;
- it is made for a later `format`;
- its `id` has a comma, a quote, a backslash or an `@`.

Limits: 64 mutators, and folder names and ids of 63 bytes at most.

### Two mutators with one id

The mutators are found in order: the stock mutators, then the folders of
`mutators` by name. A mutator whose `id` (not case-sensitive) one found
before it has is played as another, and its files are not changed:

1. **The same files** (its `mutator.toml` and add-on, by their SHA-256) as a
   mutator with its id: it is that mutator again, played once.
2. **A folder of another name**: its id and its folder's name,
   `big_head@my_big_head`.
3. **A folder of the same name** (`mutators/big_head`, beside
   `stock_mutators/big_head`), or where step 2's is taken: its id and the
   first six hex digits of its files' SHA-256 (the SHA-256 of their SHA-256s,
   one after the other, as a repository's index has it), `big_head@3fa2c1`.

Its folder and files keep the id it is played by, and `game.mutators` names
it by that. The log says each, and the Mods menu's details say what it is
played as and why.

```
mutators: mutators/big_head_copy is the same as stock_mutators/big_head: it is played once, as big_head
mutators: mutators/my_big_head has the id big_head, as stock_mutators/big_head has: it is played as big_head@my_big_head
mutators: mutators/accurized_ma5b has the id accurized_ma5b, as stock_mutators/accurized_ma5b has: it is played as accurized_ma5b@9cc5d1
```

Should the hex digits be taken too, it cannot be played.

On the web, only the stock mutators are found: the page cannot add a
mutators folder yet.

## Mutator repositories

A mutator repository is a folder of mutators, laid out as `mutators` is,
with a `mutator-index.toml` beside them, published as plain files on any
web server (GitHub Pages, say). The game does not read repositories yet:
the plan is that its link is given to the game, which lists the mutators
in its index and installs those chosen into `mutators`.

```
mutator-index.toml        the index, written by tools/mutator_index.py
falcon/
  mutator.toml
  falcon.cevehicle
```

The same folder can be the data root's `mutators` folder (a link to it):
the game reads its folders, and the index beside them is not one.

`tools/mutator_index.py <repository>` writes the index from each folder's
`mutator.toml`: it lists the files the game plays the mutator from (its
`mutator.toml` and its add-on, nothing else of its folder), with their sizes
and SHA-256 checksums, which a download must match, and the mutator's
SHA-256 (of those, one after the other: the game's, as above).
`--check` fails if the index is not up to date, for a commit hook or CI.

```toml
format = 1                  # the index's format
name = "Someone's mutators"
description = "..."

[[mutator]]
folder = "falcon"           # the folder it is in, and is installed into
id = "falcon"               # the rest as its mutator.toml has them
name = "Falcon"
version = "1"
authors = []
description = "..."
format = 3                  # the mutators' format it is made for
size = 10293512             # its files' bytes
sha256 = "7323c317..."      # its files' SHA-256s' SHA-256

[[mutator.file]]
path = "mutator.toml"       # in its folder
size = 1196
sha256 = "5a2f9dbe..."
```

The tool checks what it can without the game: the folders' and ids' names,
the files' sizes (a `mutator.toml` of 64 KB and an add-on of
64 MB at most) and the format; it says where two mutators have one id, which the game
plays as above. A repository and what it sends are untrusted
input, as a map is: the game checks it all again.

## Turning them on

Server Setup (Create Game, before **START GAME**) has **MUTATOR OPTIONS**,
below **MAP SHARING**: a screen of the mutators (the stock mutators first,
then the folders of `mutators`, each by name), a row each, ON or OFF. A turns
the chosen mutator on or off; the details say what it sets and why it cannot
be played, if it cannot. **OK** keeps them, and the game started next plays
them; **Cancel** forgets them.

The main menu's **Mods**, then **MUTATORS**, shows the same rows in place of
the mods, for games that have no Server Setup (the campaign). **MODS** shows
the mods again. **OK** keeps the mods and the mutators, **Cancel** forgets
both.

The mutators turned on are `game.mutators` in `config.toml` (their ids,
separated by commas; `HALO_MUTATORS` for one run). Empty, the default, plays
none: the stock game.

Where two mutators that are on set one parameter, the one found later sets
it.

## Network games

- Network version 49 has the mutators (`port/linux/NETCODE.md`): a machine of
  another version does not join, whatever mutators are on.
- The host, and a game of one machine, plays the mutators that are on when
  the map loads. Changes take effect in the next game.
- A client plays the host's mutators, never its own. As each client is ready
  for the host's game, before the host's objects, the host sends it each
  key, its scripts' (distributed message 112).
- A client takes them only from the host, reliably, and only whole: a count
  that the size does not match, a key that is not a key's text, or a value
  that is not a number leaves the message unplayed.
- A key that the client's build does not have (a fork's own) is passed
  over, and the console says so: that game cannot play as the host's does.
- The host's tag edits go before the keys (distributed message 113): their
  bytes in pieces of 4000, each its own reliable message. A client takes a
  piece only from the host, and only where the last ended, and makes the
  edits when it has them all. The poses go with them, as edits of their own
  kind. Edits of a group, field or value that its build does not take leave
  none made, and the log says so.
- Each machine's console and `debug.txt` name the mutators of the game.
- The host sends the files of its mutators that have an add-on or a script: each
  `mutator.toml`, `.lua`, `.cevehicle` and `.ceweapon`
  (`port/linux/game/mutator_share.c`). As a client joins, and for each game, the
  host lists them by their SHA-256s, in the order it plays them. The client plays
  those, in that order, in place of its own add-ons: those it has, by their SHA-256,
  and those it lacks it fetches from the host, over a connection of map sharing's
  (`map_share.c`) at `network.map_share_upload` at most, and its game does not load
  until it is done. They go in `mutator_downloads/<first digits of the SHA-256>` in
  the data root, are kept for the next time, and are played only in that host's game.
- A host is not trusted: names, kinds and sizes are checked before a file is asked
  for, each file against its SHA-256 and the files against the mutator's, and the
  mutator is then read as any is. An add-on goes through the validator, and a script
  runs in its sandbox.
- `network.mutator_download` false fetches none: the host's add-ons this machine
  lacks are not played, and what it sees of them is not the host's.
- The first time a host's script would be fetched, the player is asked: A or Enter
  for yes, B or Esc for no (on a touchscreen, a tap on the left half of the screen for
  yes, on the right for no), and no after 30 seconds, with the game held for it (a press
  or tap in its first second and a half is no answer). A yes
  is kept for that script, by its SHA-256. A script in the player's own folders is not
  asked about. One refused is not fetched, if its mutator is only that; a mutator with
  an add-on too needs the script to be checked, so it is fetched and not run.
  `network.mutator_host_scripts` is `"ask"`, `"always"` or `"never"`.
- They play on Custom Edition maps too, with the stock tags
  those lack ("Stock tags", above), whose indices every machine's tags must
  agree on.
