SOURCES += $(wildcard src/*.c)

build:
	clang -pie -lSDL2 -o wolfbox -g -O3 $(SOURCES)

run:
	./wolfbox