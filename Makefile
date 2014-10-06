
ifeq ($(OS),Windows_NT)
	ObjSuffix = .obj
	ExeSuffix = .exe
else
	ObjSuffix = .o
	ExeSuffix =
endif

BuildDir = build
Sources = $(wildcard src/*.c)
Objects = $(addprefix $(BuildDir)/,$(Sources:.c=$(ObjSuffix)))
Dirs = $(sort $(dir $(Objects)))
Target = $(BuildDir)/dumpfilter$(ExeSuffix)

all: $(Target)

$(Dirs):
	mkdir -p $(Dirs)

$(Target): $(Dirs) $(Objects)
	$(CC) $(Objects) -o $@

$(BuildDir)/%$(ObjSuffix): %.c
	$(CC) -c $< -o $@
