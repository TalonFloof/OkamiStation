SOURCES += $(wildcard src/*.c)

all: run

build:
	clang -pie -o wolfbox -Ofast $(SOURCES) -lSDL2
	rm icewolf-as- | true
	cd icewolf-as && cargo build -Z unstable-options --release --out-dir=. && mv icewolf-as ../icewolf-as-

run: build
	./wolfbox