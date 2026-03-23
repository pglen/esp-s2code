# Make for AP

all:
	./convpages.sh && ./genpages.sh
	idf.py build flash monitor ${ARGS}

target:
	idf.py set-target esp32s2

convert:
	./convpages.sh && ./genpages.sh

menuconfig:
	idf.py menuconfig

monitor:
	idf.py monitor

clean:
	idf.py clean

# EOF