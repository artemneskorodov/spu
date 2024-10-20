FLAGS:=-I ../include -I./include -Wshadow -Winit-self -Wredundant-decls -Wcast-align -Wundef -Wfloat-equal -Winline -Wunreachable-code -Wmissing-declarations -Wmissing-include-dirs -Wswitch-enum -Wswitch-default -Weffc++ -Wmain -Wextra -Wall -g -pipe -fexceptions -Wcast-qual -Wconversion -Wctor-dtor-privacy -Wempty-body -Wformat-security -Wformat=2 -Wignored-qualifiers -Wlogical-op -Wno-missing-field-initializers -Wnon-virtual-dtor -Woverloaded-virtual -Wpointer-arith -Wsign-promo -Wstack-usage=8192 -Wstrict-aliasing -Wstrict-null-sentinel -Wtype-limits -Wwrite-strings -Werror=vla -D_DEBUG -D_EJUDGE_CLIENT_SIDE
BINDIR:=bin
OUTPUT:=asm.exe
OBJDIR:=..\bin
SRCDIR:=src
SOURCE:=$(wildcard ${SRCDIR}/*.cpp)
OBJECTS:=$(addsuffix .o,$(addprefix ${BINDIR}\,$(notdir $(basename ${SOURCE}))))
LINKED:=$(wildcard ${OBJDIR}/*.o)

all: ${OUTPUT}

${OUTPUT}:${OBJECTS}
	g++ ${FLAGS} ${OBJECTS} ${LINKED} -o ../${OUTPUT}
${OBJECTS}: ${SOURCE}
	$(foreach SRC,${SOURCE},$(shell g++ -c ${SRC} ${LINKED} ${FLAGS} -o $(addsuffix .o,$(addprefix ${BINDIR}\,$(notdir $(basename ${SRC}))))))
clean:
	$(foreach OBJ,${OBJECTS}, $(shell del ${OBJ}))
	del ..\${OUTPUT}
${SOURCE}:
