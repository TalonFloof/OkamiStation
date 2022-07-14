SOURCES += $(wildcard src/*.c)

build:
	clang -pie -o wolfbox -g -Ofast $(SOURCES) -lSDL2

run:
	./wolfbox