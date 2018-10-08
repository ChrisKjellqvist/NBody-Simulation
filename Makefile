# other compilers work but this is the once that I've used before
CC=pgc++
#CC=clang++-5.0
# uncomment the stuff on the next line to have a limited number of iterations
# instead of an infinite loop
OPTS= -Minfo=accel -O3 -DITERATIONS=200
SRC=main.cpp SolarSystem.cpp

all: 
	@$(MAKE) gpu
serial:
	@$(CC) $(OPTS) $(SRC) -o ss_serial
var:
	@$(CC) $(OPTS) $(SRC) -DVARIABLE_TS -o ss_serial
gpu:
	@$(CC) $(OPTS) -acc $(SRC) -o ss_gpu
clean:
	@rm -f ss_gpu ss_serial *.o
	@echo "Cleaning..."
