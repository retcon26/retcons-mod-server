# PSP Sandbox dedicated server: the game's own engine (game/psp/src, the psp-sandbox repo as a submodule)
# on a small PC platform layer (src/) with stand-ins for the PSP SDK's headers (compat/).
#
#   make                 build ./psp-sandbox-server
#   make LUA=1           with Lua addons (needs Lua 5.4's headers: liblua5.4-dev)
#   make ENGINE=path     the engine from somewhere else (a psp-sandbox checkout's psp/src)

ENGINE ?= game/psp/src
BIN = psp-sandbox-server

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
	@echo "No engine at $(ENGINE): run git submodule update --init (or make ENGINE=path/to/psp-sandbox/psp/src)"
	@false

clean:
	rm -rf build $(BIN)

.PHONY: all clean

-include $(OBJS:.o=.d)
