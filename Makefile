# Retcon's Mod dedicated server: the game's own engine (game/psp/src, the retcons-mod repo as a submodule)
# on a small PC platform layer (src/) with stand-ins for the PSP SDK's headers (compat/).
#
#   make                 build ./retcons-mod-server (with Lua when pkg-config finds Lua 5.4: liblua5.4-dev)
#   make LUA=0           without Lua: no addons or gamemodes
#   make ENGINE=path     the engine from somewhere else (a retcons-mod checkout's psp/src)

ENGINE ?= game/psp/src
BIN = retcons-mod-server

# The engine files the server runs; gfx, audio, input, voice, the keyboard, memory and the network
# are the platform's (src/platform.c, src/net.c)
ENGINE_SRC = game physics collision map ents npc player proplib propx pak vehicle vmodel camera sky ui settings \
             menu achieve workshop builds nav
SRC = main platform net listserver

CC ?= gcc
CFLAGS ?= -O2 -g
CFLAGS += -std=gnu11 -ffast-math -Wall -Wno-unused-parameter -Wno-unused-function -MMD -MP
# -iquote: the engine's "input.h" and the rest win over the system's headers of the same name
CPPFLAGS += -iquote $(ENGINE) -Icompat -Isrc -I$(ENGINE) -D_GNU_SOURCE -DPSP_SANDBOX_SERVER
LDLIBS += -lm

# Lua (addons and gamemodes) when the system has Lua 5.4's headers
LUA ?= $(shell pkg-config --exists lua5.4 && echo 1 || echo 0)
ifeq ($(LUA),1)
ENGINE_SRC += script
CPPFLAGS += $(shell pkg-config --cflags lua5.4)
LDLIBS += $(shell pkg-config --libs lua5.4)
else
SRC += noscript
endif

OBJS = $(ENGINE_SRC:%=build/engine/%.o) $(SRC:%=build/src/%.o)

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

build/engine/%.o: $(ENGINE)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(ENGINE)/game.c:
	@echo "No engine at $(ENGINE): run git submodule update --init (or make ENGINE=path/to/retcons-mod/psp/src)"
	@false

clean:
	rm -rf build $(BIN)

.PHONY: all clean

-include $(OBJS:.o=.d)
