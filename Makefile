CC	        = gcc
CXX	        = g++
OBJCOPY	    = objcopy

CFLAGS	    =  -g -rdynamic -Wall -pthread -MMD -MP
CFLAGS      += -Iinclude -IfreeModbus/include -IfreeModbus/tcp -IfreeModbus/port

CCFLAGS	    =  -g -rdynamic -Wall -Wextra -Iinclude -MMD -MP

LDFLAGS     =  -g -rdynamic -lfltk -lX11 -lpthread

BUILD_DIR   = build

NAME_APP   = appForFaradayCups
BIN_APP    = $(BUILD_DIR)/$(NAME_APP)

CCSRC       = source/main.cpp \
              source/serial_ports.cpp \
              source/gui_widgets.cpp

OBJS_RSTL  = $(addprefix $(BUILD_DIR)/, $(CCSRC:.cpp=.o))
DEPS_RSTL  = $(OBJS_RSTL:.o=.d)

.PHONY: clean all

all: $(BIN_APP)

$(BIN_APP): $(OBJS_RSTL)
	$(CXX) -o $@ $(OBJS_RSTL) $(LDFLAGS)

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

