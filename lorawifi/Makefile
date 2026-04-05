# Make for AP

COMM = idf.py ${ARGS}

all:
	./convpages.sh && ./genpages.sh
	${COMM} build flash monitor

target:
	${COMM}  set-target esp32s2

convert:
	./convpages.sh && ./genpages.sh

build:
	${COMM} build

menuconfig:
	${COMM} menuconfig

monitor:
	${COMM}  monitor

clean:
	idf.py clean

fullclean: clean
	rm -rf build

# EOF