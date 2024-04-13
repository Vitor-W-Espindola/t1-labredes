PROJECT_DIR	= .
SRC_DIR		= ../src

all: mock_rtlp cli

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
	rm *.o mock_rtlp cli
