SOURCES += $(wildcard src/*.c)

build:
	clang -pie -o wolfbox -Ofast $(SOURCES) -lSDL2

run: build
	./wolfbox