# turtle-kernel build
#
#   make            build everything (stage 1, stage 2, libk)
#   make disk       build the bootable image build/disk.img
#   make run        build the image and boot it in QEMU
#   make installer  build the host installer bin/turtle-install
#   make V=1        show full commands
#   make clean      remove the build directory
#
# Sources (*.c, *.S, *.s) are discovered automatically under each component's
# source directory. Objects go to $(BUILD)/<path of source>.o, mirroring the
# source tree. Header dependencies are tracked via -MMD/-MP.

# --------------------------------------------------------------------------
#  Toolchain / global flags
# --------------------------------------------------------------------------
BUILD    ?= build

# make predefines CC=cc; prefer gcc unless overridden on the command line/env
ifeq ($(origin CC),default)
CC       := gcc
endif
LD       ?= ld
AR       ?= ar

ARCH_CFLAGS  := -m32 -fno-pic -fno-pie -mno-mmx -mno-sse -mno-sse2 -mno-80387
CFLAGS       ?= -O2 -g
CFLAGS       += $(ARCH_CFLAGS) -std=gnu11 -Wall -Wextra \
                -ffreestanding -fno-builtin -fno-stack-protector \
                -fno-asynchronous-unwind-tables -fno-unwind-tables \
				-fno-delete-null-pointer-checks --param=min-pagesize=0
ASFLAGS      += -m32
CPPFLAGS     +=
DEPFLAGS      = -MMD -MP -MF $(@:.o=.d) -MT $@
LDFLAGS      += -m elf_i386 -nostdlib
ARFLAGS      := rcs

# Quiet output unless V=1
ifeq ($(V),1)
Q :=
msg = @true
else
Q := @
msg = @printf '  %-6s %s\n' $(1) $(2)
endif

# --------------------------------------------------------------------------
#  Helpers
# --------------------------------------------------------------------------
# All C / assembly sources below a directory, sorted for stable builds.
find_srcs = $(sort $(shell find $(1) -type f \( -name '*.c' -o -name '*.S' -o -name '*.s' \) 2>/dev/null))
# src/foo/bar.c -> $(BUILD)/src/foo/bar.o
to_objs   = $(addprefix $(BUILD)/,$(addsuffix .o,$(basename $(1))))
# Put any entry.o first so its code lands at the start of .text.
entry_first = $(filter %/entry.o,$(1)) $(filter-out %/entry.o,$(1))

# --------------------------------------------------------------------------
#  libk  (static library, linked into stage 2)
# --------------------------------------------------------------------------
LIBK_DIR    := libk
LIBK_SRCS   := $(call find_srcs,$(LIBK_DIR)/src)
LIBK_OBJS   := $(call to_objs,$(LIBK_SRCS))
LIBK        := $(BUILD)/libk.a

$(LIBK_OBJS): CPPFLAGS += -I$(LIBK_DIR)/include

# --------------------------------------------------------------------------
#  BIOS stage 1  (MBR)
# --------------------------------------------------------------------------
STAGE1_DIR  := boot/src/bios/stage1
STAGE1_SRCS := $(call find_srcs,$(STAGE1_DIR))
STAGE1_OBJS := $(call to_objs,$(STAGE1_SRCS))
STAGE1_LDS  := $(STAGE1_DIR)/stage1.ld
STAGE1      := $(BUILD)/stage-1.bin

$(STAGE1_OBJS): CPPFLAGS += -Iboot/include -Iboot/include/bios

# --------------------------------------------------------------------------
#  BIOS stage 2
# --------------------------------------------------------------------------
STAGE2_DIR  := boot/src/bios/stage2
STAGE2_SRCS := $(call find_srcs,$(STAGE2_DIR))
STAGE2_OBJS := $(call to_objs,$(STAGE2_SRCS))
STAGE2_LDS  := $(STAGE2_DIR)/stage2.ld
STAGE2      := $(BUILD)/stage-2.bin

$(STAGE2_OBJS): CPPFLAGS += -Iboot/include -Iboot/include/bios -I$(LIBK_DIR)/include

# --------------------------------------------------------------------------
#  Installer  (host program, built with the host compiler and libc)
#  Objects go to $(BUILD) like everything else; the binary goes to $(BIN).
# --------------------------------------------------------------------------
BIN          ?= bin
HOSTCC       ?= cc
HOSTCFLAGS   ?= -O2 -g -std=gnu11 -Wall -Wextra
HOSTLDFLAGS  ?=

INSTALL_DIR  := install
INSTALL_SRCS := $(call find_srcs,$(INSTALL_DIR)/src)
INSTALL_OBJS := $(call to_objs,$(INSTALL_SRCS))
INSTALLER    := $(BIN)/turtle-install

# --------------------------------------------------------------------------
#  Top-level targets
# --------------------------------------------------------------------------
ALL_OBJS := $(LIBK_OBJS) $(STAGE1_OBJS) $(STAGE2_OBJS) $(INSTALL_OBJS)
DEPS     := $(ALL_OBJS:.o=.d)

.PHONY: all libk stage1 stage2 installer disk run clean
.DEFAULT_GOAL := all

all: $(STAGE1) $(STAGE2) $(INSTALLER)
libk: $(LIBK)
stage1: $(STAGE1)
stage2: $(STAGE2)
installer: $(INSTALLER)

$(LIBK): $(LIBK_OBJS)
	$(call msg,AR,$@)
	@mkdir -p $(@D)
	$(Q)rm -f $@
	$(Q)$(AR) $(ARFLAGS) $@ $^

$(STAGE1): $(STAGE1_OBJS) $(STAGE1_LDS)
	$(call msg,LD,$@)
	@mkdir -p $(@D)
	$(Q)$(LD) $(LDFLAGS) -T $(STAGE1_LDS) -Map=$(@:.bin=.map) \
		-o $@ $(call entry_first,$(STAGE1_OBJS))

$(STAGE2): $(STAGE2_OBJS) $(LIBK) $(STAGE2_LDS)
	$(call msg,LD,$@)
	@mkdir -p $(@D)
	$(Q)$(LD) $(LDFLAGS) -T $(STAGE2_LDS) -Map=$(@:.bin=.map) \
		-o $@ $(call entry_first,$(STAGE2_OBJS)) $(LIBK)

$(INSTALLER): $(INSTALL_OBJS)
	$(call msg,LD,$@)
	@mkdir -p $(@D)
	$(Q)$(HOSTCC) $(HOSTLDFLAGS) -o $@ $^

# Installer objects: host compiler/flags. This rule's stem is shorter than the
# generic $(BUILD)/%.o one below, so GNU make picks it for install/ sources.
$(BUILD)/$(INSTALL_DIR)/%.o: $(INSTALL_DIR)/%.c Makefile
	$(call msg,HOSTCC,$<)
	@mkdir -p $(@D)
	$(Q)$(HOSTCC) -I$(INSTALL_DIR)/include $(HOSTCFLAGS) $(DEPFLAGS) -c $< -o $@

# --------------------------------------------------------------------------
#  Pattern rules (output mirrors source tree under $(BUILD))
#  Objects also depend on this Makefile so flag changes trigger a rebuild.
# --------------------------------------------------------------------------
$(BUILD)/%.o: %.c Makefile
	$(call msg,CC,$<)
	@mkdir -p $(@D)
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# .S goes through the C preprocessor, so it gets dependency tracking too
$(BUILD)/%.o: %.S Makefile
	$(call msg,AS,$<)
	@mkdir -p $(@D)
	$(Q)$(CC) $(CPPFLAGS) $(ASFLAGS) $(DEPFLAGS) -c $< -o $@

# .s is plain assembly (no preprocessor, no includes to track)
$(BUILD)/%.o: %.s Makefile
	$(call msg,AS,$<)
	@mkdir -p $(@D)
	$(Q)$(CC) $(ASFLAGS) -c $< -o $@

# --------------------------------------------------------------------------
#  Disk image + QEMU
#    The image is laid out by the installer (bin/turtle-install): MBR,
#    stage 2 and a FAT32 ESP filling the rest of the image. The ESP needs
#    at least ~36 MB, so keep IMG_SIZE_MB above that.
#    Note: the installer reads build/stage-{1,2}.bin, so BUILD must be "build".
# --------------------------------------------------------------------------
IMG          := $(BUILD)/disk.img
IMG_SIZE_MB  ?= 64
QEMU         ?= qemu-system-x86_64
QEMUFLAGS    ?= -no-reboot -d int,cpu_reset -D $(BUILD)/qemu.log -m 4G

disk: $(IMG)

$(IMG): $(STAGE1) $(STAGE2) $(INSTALLER) Makefile
	$(call msg,IMG,$@)
	$(Q)rm -f $@ && truncate -s $(IMG_SIZE_MB)M $@ && \
	./$(INSTALLER) --dev $@ --mbr --yes || { rm -f $@; exit 1; }

run: $(IMG)
	$(QEMU) -drive file=$(IMG),format=raw $(QEMUFLAGS)

clean:
	$(call msg,CLEAN,$(BUILD))
	$(Q)rm -rf $(BUILD)
	$(Q)rm -f $(INSTALLER)

-include $(DEPS)
