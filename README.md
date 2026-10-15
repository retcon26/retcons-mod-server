# PSP Sandbox Server

A dedicated server for [PSP Sandbox](https://github.com/retcon26/psp-sandbox), the Garry's Mod–style sandbox for the
PSP. It runs the game's own engine on a Linux PC or a rented server, with nobody playing on it: the physics, NPCs,
ragdolls, doors and saved builds all run on the server, and PSPs join it over Wi-Fi or the internet. It's like
GMod's `srcds`, for PSPs.

- Up to **3 players** (the game's 4 slots, minus the host's, which the server keeps empty).
- PSPs find it in **Multiplayer > Online Servers**: by itself on the same network, or by its address anywhere else.
- A **console** for status, chat, kicking, changing maps and loading saved builds.
- Needs PSP Sandbox **1.7.0** or newer on the PSPs (the server and the PSPs must be the same version).

## Building

The engine comes from the game's repo, as a git submodule in `game/`. On Linux, or WSL on Windows:

```bash
git clone --recursive https://github.com/retcon26/psp-sandbox-server.git
cd psp-sandbox-server
make
```

That needs `gcc` and `make` (`sudo apt install build-essential`). Already cloned without `--recursive`? Run
`git submodule update --init`. To build against another copy of the game's source, use
`make ENGINE=path/to/psp-sandbox/psp/src` (a path with no spaces).

**Lua addons** don't run on the server unless you build it with them: `sudo apt install liblua5.4-dev`, then
`make LUA=1`.

## Game data

The server loads the same converted maps and packs as the PSP: point `game_dir` at a PSP Sandbox folder, the one
with `data/` in it. That's your install's `PSP/GAME/PSPSandbox`, or `psp/` in a psp-sandbox checkout where you've
converted the data. **No game content is in this repository.** The data comes from your own Valve and GMod
installs (see the game's README), and the PSPs that join need the same map.

Give the server its own copy of the folder if you can. It reads maps and packs, writes saved builds to
`data/saves/`, and never touches the PSP's settings or achievements.

## Running

```bash
cp server.cfg.example server.cfg   # then set game_dir, map, name...
./psp-sandbox-server
```

Or with no config file: `./psp-sandbox-server --game ~/PSPSandbox --map gm_flatgrass --name "My Server"`. Every
setting in [server.cfg.example](server.cfg.example) works as `--setting value`.

| Setting | Default | What it does |
|---|---|---|
| `game_dir` | `.` | The PSP Sandbox folder (with `data/`) |
| `map` | `gm_construct` | The map to host |
| `build` | | A saved build (`data/saves/NAME.build`) to load where it was built |
| `name` | `Sandbox Server` | Shown in Online Servers (15 characters at most) |
| `port` | `18256` | UDP port |
| `lan` | `1` | Beacons on the local network too, so PSPs there find the server without its address |
| `tick` | `60` | World updates a second |
| `god_mode`, `npc_think`, `npc_ignore_players`, `unlimited_ammo`, `physics` | the game's | The game's options, as the host's are in a PSP-hosted game |

### Console

Type these while it runs:

| Command | |
|---|---|
| `status` | The map and who's in each slot |
| `say TEXT` | A chat line on everyone's screen |
| `kick SLOT` | Sends a player away (slots are in `status`) |
| `map NAME` | Changes map: players are told to join again |
| `save NAME` / `load NAME` | Saves everything spawned to `data/saves/NAME.build` / loads a build where it was built |
| `cleanup` | Removes everything spawned |
| `quit` | Stops the server (Ctrl+C works too) |

Joins, leaves and chat go to the log. `PSB_DEBUG=1 ./psp-sandbox-server` also logs every packet that arrives.

## Joining from a PSP

1. The PSP needs a Wi-Fi connection set up in **System Settings > Network Settings** (it uses the first one).
   In PPSSPP, turn networking (WLAN) on in its settings.
2. **Multiplayer > Online Servers.** A server on the same network shows up by itself. Otherwise press
   **Triangle** and type its address, a name or `1.2.3.4`, with `:port` if it isn't 18256. Servers you add are
   kept in `data/servers.cfg`.
3. Pick it and press **X**.

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
