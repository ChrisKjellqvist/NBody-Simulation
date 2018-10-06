# other compilers work but this is the once that I've used before
CC=pgc++
# uncomment the stuff on the next line to have a limited number of iterations
# instead of an infinite loop
OPTS=-Minfo=accel -O3 #-DITERATIONS=100
SRC=main.cpp SolarSystem.cpp

all: 
	@$(CC) $(OPTS) -acc $(SRC) -o ss_gpu
serial:
	@$(CC) $(OPTS) $(SRC) -o ss_serial
clean:
	@rm -f ss_gpu ss_serial *.o
	@echo "Cleaning..."
