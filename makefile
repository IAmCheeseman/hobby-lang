CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror
CFLAGS += -Isrc
LDFLAGS = -lm

ifndef OS
	OS=linux
endif

RM = rm
RMDIR = rm -r
MKDIR = mkdir -p

ifndef PROFILE
	PROFILE = debug
endif

ifeq ($(PROFILE), debug)
	CFLAGS += -O3 -g
	ifneq ($(OS), windows)
		CFLAGS += -fsanitize=address
	endif
endif

ifeq ($(PROFILE), release)
	CFLAGS += -O3
endif

BUILD = bin

SRC = src/main.c src/memory.c src/debug.c src/value.c src/vm.c \
			src/compiler.c src/tokenizer.c src/object.c src/table.c \
			src/state.c src/tostring.c src/core.c

OBJ = $(SRC:%.c=$(BUILD)/%_$(PROFILE).o)

DEPENDS = $(OBJ:.o=.d)
EXE = $(BUILD)/hs_$(PROFILE)

.PHONY: clean compile_flags

$(EXE): $(OBJ)
	@$(MKDIR) $(BUILD)
	@echo "Compiling $(EXE)..."
	@$(CC) -o $(EXE) $(OBJ) $(CFLAGS) $(LDFLAGS)
	@echo "Compile args: $(CC) $(CFLAGS)"

$(BUILD)/%_$(PROFILE).o: %.c
	@$(MKDIR) $(@D)
	@echo "Compiling $< -> $@..."
	@$(CC) -o $@ -c $< $(CFLAGS) -MMD -MP

clean:
	$(RMDIR) $(BUILD)

compile_flags:
	@echo "" > compile_flags.txt
	@$(foreach flag,$(CFLAGS),echo $(flag) >> compile_flags.txt;)

-include $(DEPENDS)
