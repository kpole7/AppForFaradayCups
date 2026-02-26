CXX	        = g++
OBJCOPY	    = objcopy

# Base compile flags
CCFLAGS	    =  -g -rdynamic -Wall -Wextra -Wpedantic -Iinclude -I/usr/include/modbus -MMD -MP

# Base link flags
LDFLAGS     =  -g -rdynamic -lfltk -lX11 -lpthread -lmodbus -lfltk_images -lpng -lz

BUILD_DIR   = build

NAME_APP   = appForFaradayCups
BIN_APP    = $(BUILD_DIR)/$(NAME_APP)

NAME_CFG   = PomiarWiązki.cfg

CCSRC       = source/main.cpp \
              source/peripheral_thread.cpp \
              source/shared_data.cpp \
              source/modbus_rtu_master.cpp \
              source/gui_widgets.cpp \
              source/settings_file.cpp

OBJS_RSTL  = $(addprefix $(BUILD_DIR)/, $(CCSRC:.cpp=.o))
DEPS_RSTL  = $(OBJS_RSTL:.o=.d)

# Sanitizer flags (empty by default; targets below append to these)
SANFLAGS   =
ASANFLAGS  = -O1 -fno-omit-frame-pointer -fsanitize=address
UBSANFLAGS = -O1 -fno-omit-frame-pointer -fsanitize=undefined
SANUBFLAGS = -O1 -fno-omit-frame-pointer -fsanitize=address,undefined

.PHONY: clean all \
        asan ubsan sanitize \
        run run-asan run-ubsan run-sanitize

all: $(BIN_APP)

$(BIN_APP): $(OBJS_RSTL)
	$(CXX) -o $@ $(OBJS_RSTL) $(LDFLAGS) $(SANFLAGS)
	cp $(NAME_CFG) $(BUILD_DIR)
	cp doc/*.pdf $(BUILD_DIR)

# ---------------------------------------------------------------------------
# rules for code generation
# ---------------------------------------------------------------------------
$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CCFLAGS) $(SANFLAGS) -o $@ -c $<

# ---------------------------------------------------------------------------
# compiler generated dependencies
# ---------------------------------------------------------------------------
-include $(DEPS_RSTL)

clean:
	rm -rf $(BUILD_DIR)

# -----------------------
# Sanitizer build targets
# -----------------------
asan: SANFLAGS += $(ASANFLAGS)
asan: clean all

ubsan: SANFLAGS += $(UBSANFLAGS)
ubsan: clean all

sanitize: SANFLAGS += $(SANUBFLAGS)
sanitize: clean all

# -----------------------
# Run targets (optional)
# -----------------------
run: all
	./$(BIN_APP)

run-asan: asan
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=0 ./$(BIN_APP)

run-ubsan: ubsan
	UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./$(BIN_APP)

run-sanitize: sanitize
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=0 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./$(BIN_APP)
	