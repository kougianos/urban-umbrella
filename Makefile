all: jms_console jms_coord

jms_console: jms_console.o
	g++ jms_console.o -o jms_console

jms_coord: jms_coord.o
	g++ jms_coord.o -o jms_coord

jms_coord.o: jms_coord.cpp domes.h
	g++ -c jms_coord.cpp

jms_console.o: jms_console.cpp
	g++ -c jms_console.cpp

