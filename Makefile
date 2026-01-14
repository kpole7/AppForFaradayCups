CXX	        = g++
OBJCOPY	    = objcopy

CCFLAGS	    =  -g -rdynamic -Wall -Wextra -Iinclude -MMD -MP

LDFLAGS     =  -g -rdynamic -lfltk -lX11 -lpthread

BUILD_DIR   = build

NAME_APP   = appForFaradayCups
BIN_APP    = $(BUILD_DIR)/$(NAME_APP)

NAME_CFG   = PomiarWiązki.cfg

CCSRC       = source/main.cpp \
              source/serial_communication.cpp \
              source/shared_data.cpp \
              source/uart_ports.cpp \
              source/gui_widgets.cpp \
              source/settings_file.cpp

OBJS_RSTL  = $(addprefix $(BUILD_DIR)/, $(CCSRC:.cpp=.o))
DEPS_RSTL  = $(OBJS_RSTL:.o=.d)

.PHONY: clean all

all: $(BIN_APP)

$(BIN_APP): $(OBJS_RSTL)
	$(CXX) -o $@ $(OBJS_RSTL) $(LDFLAGS)
	cp $(NAME_CFG) $(BUILD_DIR)

# ---------------------------------------------------------------------------
# rules for code generation
# ---------------------------------------------------------------------------
$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CCFLAGS) -o $@ -c $<

# ---------------------------------------------------------------------------
#  # compiler generated dependencies
# ---------------------------------------------------------------------------
-include $(DEPS_RSTL)

clean:
	rm -rf $(BUILD_DIR)

