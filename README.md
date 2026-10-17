# Retcon's Mod Server

A dedicated server for [Retcon's Mod](https://github.com/retcon26/retcons-mod) (formerly PSP Sandbox), the physics
sandbox for the PSP. It runs the game's own engine on a Linux PC or a rented server, with nobody playing on it: the
physics, NPCs, ragdolls, doors and saved builds all run on the server, and PSPs join it over Wi-Fi or the internet.

- Up to **7 players** (`max_players`), with a **password** if you like.
- A **map rotation**: every player's PSP follows the server to the next map by itself.
- The players' build **saved every few minutes**, and loaded again when its map comes back round.
- PSPs find it in **Multiplayer > Online Servers**: by itself on the same network, through a **server list**, or
  by its address.
- A **console** for status, chat, kicking, changing maps and loading saved builds.
- The same program is also a **server list** (`--mode list`): game servers sign up to it, and PSPs see them all.
- Server 1.2.1 needs the game **1.11.0** on the PSPs (the server and the PSPs must speak the same network
  protocol; see the game's [changelog](https://github.com/retcon26/retcons-mod/blob/main/docs/CHANGELOG.md)).

It was called psp-sandbox-server before 1.2.0; GitHub forwards the old links.

## Building

The engine comes from the game's repo, as a git submodule in `game/`. On Linux, or WSL on Windows:

```bash
git clone --recursive https://github.com/retcon26/retcons-mod-server.git
cd retcons-mod-server
make
```

That needs `gcc` and `make` (`sudo apt install build-essential`). Already cloned without `--recursive`? Run
`git submodule update --init`. To build against another copy of the game's source, use
`make ENGINE=path/to/retcons-mod/psp/src` (a path with no spaces).

**Lua addons** don't run on the server unless you build it with them: `sudo apt install liblua5.4-dev`, then
`make LUA=1`.

## Game data

The server loads the same converted maps and packs as the PSP: point `game_dir` at the game's folder, the one
with `data/` in it. That's your install's `PSP/GAME/RetconsMod`, or `psp/` in a retcons-mod checkout where you've
converted the data. **No game content is in this repository.** The data comes from your own Valve and GMod
installs (see the game's README), and the PSPs that join need the same map.

Give the server its own copy of the folder if you can. It reads maps and packs, writes saved builds to
`data/saves/` (autosaves are `auto-<map>.build`), and never touches the PSP's settings or achievements. The server's NPCs find their way round walls
with a map's node graph, `data/maps/<map>.pnav`, made by the converter with the map (`tools/bspconv.sh nav` makes
them for maps converted before 1.8.2); without it they head straight for where they're going.

## Running

```bash
cp server.cfg.example server.cfg   # then set game_dir, map, name...
./retcons-mod-server
```

Or with no config file: `./retcons-mod-server --game ~/RetconsMod --map gm_flatgrass --name "My Server"`. Every
setting in [server.cfg.example](server.cfg.example) works as `--setting value`.

| Setting | Default | What it does |
|---|---|---|
| `game_dir` | `.` | The game's folder (with `data/`) |
| `map` | `gm_construct` | The map to host |
| `build` | | A saved build (`data/saves/NAME.build`) to load where it was built |
| `name` | `Retcon's Server` | Shown in Online Servers (15 characters at most) |
| `port` | `18256` | UDP port |
| `lan` | `1` | Beacons on the local network too, so PSPs there find the server without its address |
| `tick` | `60` | World updates a second |
| `max_players` | `7` | How many can play at once (1 to 7) |
| `password` | | Players type it in to join (Online Servers marks the server "locked"); none if it's not set |
| `maps` | | A map rotation: map names with spaces between, e.g. `gm_construct gm_flatgrass gm_bigcity` |
| `map_minutes` | `30` | Minutes on each map of the rotation, with a minute's warning (0: no rotation) |
| `autosave_minutes` | `5` | The players' build is saved this often, and on a map change or shutdown (0: never) |
| `autosave_resume` | `1` | A map starts with its last autosave (`data/saves/auto-<map>.build`) |
| `list` | | A server list to appear on (`host` or `host:port`; its port is 18257 unless you say) |
| `public_address` | | The address the list gives players, when it's not the one the list sees (a LAN-only list) |
| `mode` | `game` | `game` hosts a map; `list` runs a server list instead (see below) |
| `list_port` | `18257` | `mode list`: its UDP port |
| `god_mode`, `npc_think`, `npc_ignore_players`, `unlimited_ammo`, `physics` | the game's | The game's options, as the host's are in a PSP-hosted game |

### Console

Type these while it runs:

| Command | |
|---|---|
| `status` | The map, who's in each slot, and the next map |
| `say TEXT` | A chat line on everyone's screen |
| `kick SLOT` | Sends a player away (slots are in `status`) |
| `map NAME` | Changes map (after an autosave): every player's PSP joins the new map by itself |
| `next` | The next map in the rotation, now |
| `save NAME` / `load NAME` | Saves everything spawned to `data/saves/NAME.build` / loads a build where it was built |
| `cleanup` | Removes everything spawned |
| `quit` | Stops the server (Ctrl+C works too) |

Joins, leaves, chat and autosaves go to the log. `PSB_DEBUG=1 ./retcons-mod-server` also logs every packet
that arrives.

## Server lists

A server list saves players typing addresses: every dedicated server that signs up shows in their Online Servers.

```bash
./retcons-mod-server --mode list            # on a machine everyone can reach (UDP port 18257)
```

It needs no game data and runs nothing but the list (`status` lists its servers). Each game server signs up with
`list` in its `server.cfg`:

```
list lists.example.org          # or an IP, with :port if the list isn't on 18257
```

It tells the list it's up every 30 seconds and drops off after 90 seconds without. On the PSP, Online Servers'
**R** sets the list's address. The list hands a PSP the servers of its own game version, and the PSP asks each of
them about its game itself, so a stopped server never shows. For players outside your network, forward UDP 18257
(the list) as well as 18256 (the game).

## Joining from a PSP

1. The PSP needs a Wi-Fi connection set up in **System Settings > Network Settings** (it uses the first one).
   In PPSSPP, turn networking (WLAN) on in its settings.
2. **Multiplayer > Online Servers.** A server on the same network shows up by itself. Otherwise press
   **Triangle** and type its address, a name or `1.2.3.4`, with `:port` if it isn't 18256. Or press **R** and
   type a server list's address to see every server on it. Both are kept in `data/servers.cfg`.
3. Pick it and press **X**. A "locked" server asks for its password.

For players outside your network, forward **UDP port 18256** on your router to the PC that runs the server, and
give them your public address.

**WSL on Windows:** WSL 2 sits behind its own network address, which PSPs (and PPSSPP) on your network can't
reach. Either turn on WSL's mirrored networking (`networkingMode=mirrored` in `%UserProfile%\.wslconfig`, then
`wsl --shutdown`), or run the server on a Linux machine or VPS.

## How it works

The server is the game itself. It builds the game's engine (`game/psp/src`) for a PC, on a small platform layer
in `src/`:

- `compat/`: stand-ins for the PSP SDK's headers.
- `src/platform.c`: files and time on POSIX; graphics, sound, input, voice and the on-screen keyboard do nothing.
- `src/net.c`: the game's network layer on UDP sockets, with the same packets the PSPs send over Wi-Fi.
- `src/main.c`: the config, the console and a fixed-rate loop.

The game hosts as it would on a PSP, with `game_host_dedicated()` keeping the host's own player out of the game, so
the server and the PSP versions can't drift apart. Update the engine with
`git submodule update --remote game` after a new game release.
