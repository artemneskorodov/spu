FLAGS:=-I ../include -I ../spu -Wshadow -Winit-self -Wredundant-decls -Wcast-align -Wundef -Wfloat-equal -Winline -Wunreachable-code -Wmissing-declarations -Wmissing-include-dirs -Wswitch-enum -Wswitch-default -Weffc++ -Wmain -Wextra -Wall -g -pipe -fexceptions -Wcast-qual -Wconversion -Wctor-dtor-privacy -Wempty-body -Wformat-security -Wformat=2 -Wignored-qualifiers -Wlogical-op -Wno-missing-field-initializers -Wnon-virtual-dtor -Woverloaded-virtual -Wpointer-arith -Wsign-promo -Wstack-usage=8192 -Wstrict-aliasing -Wstrict-null-sentinel -Wtype-limits -Wwrite-strings -Werror=vla -D_DEBUG -D_EJUDGE_CLIENT_SIDE
BINDIR:=..\bin
SOURCE:=$(wildcard *.cpp)
OBJECTS:=$(addsuffix .o,$(addprefix ${BINDIR}\,$(basename ${SOURCE})))

all: ${OBJECTS}

${OBJECTS}:${SOURCE}
	$(foreach SRC,${SOURCE}, $(shell g++ -c ${FLAGS} ${SRC} -o $(addsuffix .o,$(addprefix ${BINDIR}/,$(basename ${SRC})))))
clean:
	$(foreach OBJ,${OBJECTS}, $(shell del ${OBJ}))
