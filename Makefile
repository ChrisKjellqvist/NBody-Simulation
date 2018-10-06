# other compilers work but this is the once that I've used before
CC=pgc++
OPTS=-Minfo=accel -O3
SRC=main.cpp SolarSystem.cpp

all: 
	@$(CC) $(OPTS) -acc $(SRC) -o ss_gpu
serial:
	@$(CC) $(OPTS) $(SRC) -o ss_serial
clean:
	@rm -f ss_gpu ss_serial *.o
	@echo "Cleaning..."
