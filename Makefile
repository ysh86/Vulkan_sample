TARGET_EXEC ?= hellovk

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src

SRCS := $(shell find -L $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find -L $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP

CFLAGS += -Wall -Werror --std=c99 -O3
CXXFLAGS += -Wall -Werror --std=c++11 -O3


# libs

# Vulkan
SHADER_SRCS := $(shell find -L $(SRC_DIRS) -name *.vert -or -name *.frag -or -name *.comp)
SHADER_OBJS := $(SHADER_SRCS:%=$(BUILD_DIR)/%.spv)
SHADER_VER := 450

CPPFLAGS += -I$(HOME)/SDKs/Vulkan-Headers/include
ifdef WSL_DISTRO_NAME
	TARGET_EXEC := $(TARGET_EXEC).exe
	AS  := x86_64-w64-mingw32-as
	CC  := x86_64-w64-mingw32-gcc-posix
	CXX := x86_64-w64-mingw32-g++-posix
	#CPPFLAGS += -I$(HOME)/SDKs/mingw-std-threads
	#CXXFLAGS += -O0
	CFLAGS += -mavx2 -mfma
	CXXFLAGS += -mavx2 -mfma
	LDFLAGS += /mnt/c/Windows/System32/vulkan-1.dll -static -lstdc++ -lgcc -lwinpthread
else
	CFLAGS += -march=armv7-a
	CXXFLAGS += -march=armv7-a
	LDFLAGS += -lvulkan
	#LDFLAGS += -framework vulkan
endif

# stb
CPPFLAGS += -I$(HOME)/SDKs/stb
CFLAGS += -Wno-error=sign-compare
CXXFLAGS += -Wno-error=sign-compare

# OpenCV, glm
###CPPFLAGS += `pkg-config --cflags opencv glm`
###LDFLAGS += `pkg-config --libs opencv`

# ipp
###CPPFLAGS += -I$(HOME)/SDKs/ipp/include
###LDFLAGS += -no-pie -L$(HOME)/SDKs/ipp/lib -lippi -lipps -lippcc -lippcore


all: $(BUILD_DIR)/$(TARGET_EXEC) $(SHADER_OBJS)

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Vulkan source
$(SHADER_OBJS): $(SHADER_SRCS)
	$(MKDIR_P) $(dir $@)
	glslangValidator -o $@ -V$(SHADER_VER) $<

.PHONY: all clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p
