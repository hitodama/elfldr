Compiler := clang
Assember := clang
Archiver := ar
ObjectCopy := objcopy

###################################

Sources := source
Includes := -I. -Iinclude
IncludesPS4 := -I$(LIBPS4)/include
Libs := -L. -Llib
LibsPS4 := -L$(LIBPS4)/lib
#Builds := build
Outs := bin
OutsLocal := $(Outs)/local
OutsPS4 := $(Outs)/ps4
OutsExample := $(Outs)/example

Libraries := -lps4

###################################

ifndef Libps4
ifndef LIBPS4
$(error Neither LIBPS4 nor Libps4 set)
endif
endif

ifdef LIBPS4
Libps4 := $(LIBPS4)
endif

ifdef DEBUG
Debug := $(DEBUG)
endif

ifdef Debug
Debug := -D"Debug"
endif

###################################

Text ?= 0x926200000
Data ?= 0x926300000
Payload ?= 0x926400000 # 3x
# Binary loader mem layout.
# text#[data#data-payl-area]
# ^     ^   #paycode#paydata
# |-----|-------x      x
#       |--------------|

###################################

CompilerFlags := -O3 -std=c11 -Wall -pedantic -m64 -mcmodel=large $(Includes) $(Debug)
CompilerFlagsPS4 := $(CompilerFlags) -D__PS4__ -Wl,--build-id=none -target x86_64-scei-ps4-elf -ffreestanding -nostdlib -nostdinc -fno-builtin $(IncludesPS4)
CompilerFlagsPS4Binary := $(CompilerFlagsPS4) -DPayload=$(Payload)
CompilerFlagsLocal := $(CompilerFlags) -fPIE -pie -lrt

AssemberFlags := -m64
AssemberFlagsPS4 := $(AssemberFlags) -D__PS4__ -target x86_64-scei-ps4-elf

LinkerFlags := $(Libs)
LinkerFlagsPS4 := $(LinkerFlags) $(LibsPS4) $(Libraries) -nostartfiles
LinkerFlagsPS4Binary := $(LinkerFlagsPS4) -Ttext $(Text) -Tdata $(Data)

CompilerFiles := $(wildcard $(Sources)/*.c)

LdrGenCompilerFiles := ldrgen/main.c source/util.c

###################################

ElfLdrPS4Target := $(OutsPS4)/elfldr
BinLdrPS4Target := $(OutsPS4)/binldr
ElfLdrFileTarget := $(OutsLocal)/elfldr
ElfLdrServerTarget := $(OutsLocal)/elfldr-srv
ElfLdrFileProtectedMemoryTarget := $(OutsLocal)/elfldr-protmem
ElfLdrServerProtectedMemoryTarget := $(OutsLocal)/elfldr-protmem-srv

LdrGenTarget := $(Outs)/ldrgen

###################################

$(ElfLdrPS4Target): $(CompilerFiles)
	$(Compiler) $(Libps4)/crt0.s $? -o $@.elf $(CompilerFlagsPS4Binary) $(LinkerFlagsPS4Binary) #-DElfLdr
	$(ObjectCopy) -O binary $@.elf $@.bin
	$(LdrGenTarget) ElfLdr $@.bin ../local/ldr/elf.js

$(BinLdrPS4Target): $(CompilerFiles)
	$(Compiler) $(Libps4)/crt0.s  $? -o $@.elf $(CompilerFlagsPS4Binary) $(LinkerFlagsPS4Binary) -DBinLdr
	$(ObjectCopy) -O binary $@.elf $@.bin
	$(LdrGenTarget) BinLdr $@.bin ../local/ldr/bin.js

$(ElfLdrFileTarget): $(CompilerFiles)
	$(Compiler) $? -o $@ $(CompilerFlagsLocal) $(LinkerFlags)

$(ElfLdrServerTarget): $(CompilerFiles)
	$(Compiler) $? -o $@ $(CompilerFlagsLocal) $(LinkerFlags) -DElfLdrServer

$(ElfLdrFileProtectedMemoryTarget): $(CompilerFiles)
	$(Compiler) $? -o $@ $(CompilerFlagsLocal) $(LinkerFlags) -DElfLdrEmulatePS4Memory

$(ElfLdrServerProtectedMemoryTarget): $(CompilerFiles)
	$(Compiler) $? -o $@ $(CompilerFlagsLocal) $(LinkerFlags) -DElfLdrServer -DElfLdrEmulatePS4Memory

$(LdrGenTarget): $(LdrGenCompilerFiles)
	$(Compiler) $? -o $@ $(CompilerFlagsLocal) $(LinkerFlags)

###################################

$(Outs):
	@mkdir -p $@

$(OutsLocal): $(Outs)
	@mkdir -p $@

$(OutsPS4): $(Outs)
	@mkdir -p $@

$(OutsExample): $(Outs)
	@mkdir -p $@

###################################

.PHONY: clean all example local ps4 ldrgen
.DEFAULT_GOAL := all

ldrgen: $(Outs) $(LdrGenTarget)

ps4: $(OutsPS4) $(LdrGenTarget) $(ElfLdrPS4Target) $(BinLdrPS4Target)

local: clean $(OutsLocal) $(ElfLdrFileTarget) $(ElfLdrServerTarget) $(ElfLdrFileProtectedMemoryTarget)  $(ElfLdrServerProtectedMemoryTarget)

# Adding -target x86_64-scei-ps4-elf will alter (optimize) the
# instruction set and may thus not work locally - it will work on the ps4
example: $(OutsExample)
	$(Compiler) example/null.c -o $(OutsExample)/null $(Libps4)/crt0.s -Wl,--build-id=none -I include/ -fPIE -pie -O3 -Wall -pedantic -std=c11 -Wl,--build-id=none -m64 -mcmodel=large -ffreestanding -nostdlib -nostdinc -fno-builtin
	$(Compiler) example/forawhile.c -o $(OutsExample)/forawhile $(Libps4)/crt0.s -Wl,--build-id=none -I include/ -fPIE -pie -O3 -Wall -pedantic -std=c11 -Wl,--build-id=none -m64 -mcmodel=large -ffreestanding -nostdlib -nostdinc -fno-builtin
	$(Compiler) example/stress.c -o $(OutsExample)/stress $(Libps4)/crt0.s -Wl,--build-id=none -I include/ -fPIE -pie -O3 -Wall -pedantic -std=c11 -Wl,--build-id=none -m64 -mcmodel=large -ffreestanding -nostdlib -nostdinc -fno-builtin

all: clean local ps4 example

clean:
	rm -fR $(Outs) #$(Builds)
