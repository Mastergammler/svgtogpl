

default: build

build: build.ninja
	@ninja

run: build
	@.build/svgtogpl ~/bj-assets/colors/beatjutsu_v2_test.svg
