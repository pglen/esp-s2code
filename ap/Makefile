# Make for AP

all:
	./convpages.sh && ./genpages.sh
	idf.py build flash monitor

convert:
	./convpages.sh && ./genpages.sh

menuconfig:
	idf.py menuconfig

monitor:
	idf.py monitor

clean:
	idf.py clean

# EOF