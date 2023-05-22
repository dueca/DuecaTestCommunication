## -*-python-*-
## this is an example dueca_mod.py file, for you to start out with and adapt
## according to your needs. Note that you need a dueca_mod.py file only for the
## node with number 0

## in general, it is a good idea to clearly document your set up
## this is an excellent place.

## node set-up
ecs_node = 0   # dutmms1, send order 3

## priority set-up
# normal nodes: 0 administration
#               1 hdf5 logging
#               2 simulation, unpackers
#               3 communication
#               4 ticker

# administration priority. Run the interface and logging here
admin_priority = dueca.PrioritySpec(0, 0)

# logging prio. Keep this free from time-critical other processes
log_priority = dueca.PrioritySpec(1, 0)

# priority of simulation, just above log
sim_priority = dueca.PrioritySpec(2, 0)

# nodes with a different priority scheme
# control loading node has 0, 1, 2 and 3 as above and furthermore
#               4 stick priority
#               5 ticker priority
# priority of the stick. Higher than prio of communication
# stick_priority = dueca.PrioritySpec(4, 0)

# timing set-up
# timing of the stick calculations. Assuming 100 usec ticks, this gives 2500 Hz
# stick_timing = dueca.TimeSpec(0, 4)

# this is normally 100, giving 100 Hz timing
sim_timing = dueca.TimeSpec(0, 100)

## for now, display on 50 Hz
display_timing = dueca.TimeSpec(0, 200)

## log a bit more economical, 25 Hz
log_timing = dueca.TimeSpec(0, 400)

## ---------------------------------------------------------------------
### the modules needed for dueca itself
if this_node_id == ecs_node:

    # create a list of modules:
    DUECA_mods = []
    DUECA_mods.append(dueca.Module("dusime", "", admin_priority))
    DUECA_mods.append(dueca.Module("dueca-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("activity-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("timing-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("log-view", "", admin_priority))
    DUECA_mods.append(dueca.Module("channel-view", "", admin_priority))
    # uncomment for web-based graph, see DUECA documentation
    # DUECA_mods.append(dueca.Module("config-storage", "", admin_priority))

    for e in ("PHLAB",):
        DUECA_mods.append(
            dueca.Module("initials-inventory", e, admin_priority).param(
                # reference_file=f"initials-{e}.toml",
                store_file=f"initials-{e}-%Y%m%d_%H%M.toml"))
        DUECA_mods.append(
            dueca.Module("replay-master", e, admin_priority).param(
                # reference_files=f"recordings-{e}.ddff",
                store_files=f"recordings-{e}-%Y%m%d_%H%M%S.ddff"))

    # create the entity with that list
    DUECA_entity = dueca.Entity("dueca", DUECA_mods)

## ---------------------------------------------------------------------
# modules for your project (example)
mymods = []

# test creating a blip from script
blp = dueca.MyBlip().param(identification = "ident",
                           x = 0.4,
                           y = 2,
                           dx = 0.0,
                           dy = 1.3,
                           mode = "On").complete()

if this_node_id == ecs_node:
    mymods.append(
        dueca.Module(
            "write-unified", "1", admin_priority).param(

                set_timing = sim_timing,
                check_timing = (10000, 20000),

                add_blip = "first blip",
                place_blip = [ 20.0, 20.0, -0.1, -0.1, ]).param(

                add_blip = "second blip",
                place_blip = [ 2, 2 ]).param(

                add_flasher_blip = "flashing",
                place_flasher_blip = [ 300, 0.4, 4.0 ]))

    mymods.append(
        dueca.Module(
            "read-unified", "1", sim_priority).param(
                set_timing = sim_timing,
                check_timing = (10000, 20000)
            ))
    mymods.append(
        dueca.Module("hdf5-logger", "", log_priority).param(
            ('set_timing', log_timing),
            ('chunksize', 3000),
            ('log_entry', ("MyBlip://PHLAB/1",
                           "MyBlip", "second blip", "/entry/second")),
            ('watch-channel', ("MyBlip://PHLAB/1", "/watched/myblip"))
        ))

    filer = dueca.ReplayFiler("PHLAB")


# etc, each node can have modules in its mymods list

# then combine in an entity (one "copy" per node)
if mymods:
    myentity = dueca.Entity("PHLAB", mymods)
