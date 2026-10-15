#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

# Normalize paths for Windows/MSYS2 compatibility
export DEVKITPRO := $(subst \,/,$(DEVKITPRO))
export DEVKITARM := $(subst \,/,$(DEVKITARM))

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# Target configuration
#---------------------------------------------------------------------------------
TARGET          := PixelDungeon
BUILD           := build
SOURCES         := source source/noosa source/pd source/pd/ui source/pd/windows
DATA            := data
INCLUDES        := include include/noosa include/pd include/pd/ui include/pd/scenes include/pd/windows include/pd/effects
GRAPHICS        := gfx
ROMFS           := romfs

# SMDH metadata
APP_TITLE       := Pixel Dungeon
APP_DESCRIPTION := Native Nintendo 3DS Port
APP_AUTHOR      := Oleg Dolya / 3DS Homebrew Port
ICON            := icon.png

#---------------------------------------------------------------------------------
# Compiler flags
#---------------------------------------------------------------------------------
ARCH            := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS          := -g -Wall -Wextra -O2 -mword-relocations \
                   -ffunction-sections -fdata-sections \
                   $(ARCH)

CFLAGS          += $(INCLUDE) -D__3DS__

CXXFLAGS        := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17

ASFLAGS         := -g $(ARCH)
LDFLAGS         := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map) -Wl,--gc-sections

LIBS            := -lcitro2d -lcitro3d -lctru -lpng -lz -lm
LIBDIRS         := $(PORTLIBS) $(CTRULIB)

#---------------------------------------------------------------------------------
# Directory parsing and build rules
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT   := $(CURDIR)/$(TARGET)
export TOPDIR   := $(CURDIR)

export VPATH    := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                   $(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir)) \
                   $(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR  := $(CURDIR)/$(BUILD)

CFILES          := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES        := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES          := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES   := $(OFILES_SOURCES)

export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                   -I$(CURDIR)/$(BUILD)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export APP_ICON := $(TOPDIR)/$(ICON)
export _3DSXDEPS := $(OUTPUT).smdh
export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh

ifneq ($(wildcard $(TOPDIR)/$(ROMFS)),)
export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: all clean

all:
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD) $(TARGET).3dsx $(OUTPUT).smdh $(TARGET).elf $(TARGET).smdh $(TARGET).map

else

#---------------------------------------------------------------------------------
# Main targets executed within $(BUILD)
#---------------------------------------------------------------------------------
$(OUTPUT).3dsx  : $(OUTPUT).elf $(_3DSXDEPS)

$(OFILES_SOURCES) : $(DEPSDIR)

$(OUTPUT).elf   : $(OFILES)
	@echo linking $(notdir $@)
	@$(CXX) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	@arm-none-eabi-nm -CSn $@ > $(notdir $*.lst)

-include $(wildcard $(DEPSDIR)/*.d)

endif

