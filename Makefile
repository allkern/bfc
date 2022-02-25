.PHONY: bfc

bfc: main.cpp
	c++ main.cpp -o bfc -O3

install:
	sudo cp -rf bfc /usr/bin/bfc