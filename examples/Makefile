# This makefile is capable of creating an example for a hub application, a client application and a packet tester application.
# The rules hub, cli and mock_rtlp are responsable for that, respectively.

PROJECT_DIR	= .
SRC_DIR		= ../src

hub: hub.o server.o rtlp.o
	gcc -o hub hub.o server.o rtlp.o

hub.o: $(PROJECT_DIR)/hub.c
	gcc -g -c $(PROJECT_DIR)/hub.c

server.o: $(SRC_DIR)/server.c
	gcc -g -c $(SRC_DIR)/server.c

cli: cli.o client.o rtlp.o
	gcc -o cli cli.o client.o rtlp.o

cli.o: $(PROJECT_DIR)/cli.c
	gcc -g -c $(PROJECT_DIR)/cli.c

client.o: $(SRC_DIR)/client.c
	gcc -g -c $(SRC_DIR)/client.c

mock_rtlp: mock_rtlp.o rtlp.o
	gcc -o mock_rtlp mock_rtlp.o rtlp.o

mock_rtlp.o: $(PROJECT_DIR)/mock_rtlp.c
	gcc -g -c $(PROJECT_DIR)/mock_rtlp.c

rtlp.o: $(SRC_DIR)/rtlp.c
	gcc -g -c $(SRC_DIR)/rtlp.c

clean:
	rm -f *.o mock_rtlp cli hub
