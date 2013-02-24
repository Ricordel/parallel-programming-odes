CC = gcc -c
MPICC = mpicc -c
CFLAGS = -std=c99 -Wall -Wextra -O2 -DNDEBUG
LD = gcc
MPILD = mpicc
LDFLAGS = -lm

COMMON_HEADERS = src/dbg.h src/option_parser.h src/solve_ode_mpi.h

all: solve_ode_mpi

dev: CFLAGS = -g -std=c99 -Wall -Wextra -O0
dev: all

solve_ode_mpi: bin/solve_ode_mpi.o bin/option_parser.o
	$(MPILD) $^ -o $@ $(LDFLAGS) 

bin/%_mpi.o: src/%_mpi.c $(COMMON_HEADERS)
	$(MPICC) $(CFLAGS) $< -o $@

bin/%.o: src/%.c $(COMMON_HEADERS)
	$(CC) $(CFLAGS) $< -o $@

#dist:
	#rm -rf ./dist
	#mkdir -p ./dist/parpro1-13-ricordel-hw1
	#cp -r bin src *.py *.sh Makefile README dist/parpro1-13-ricordel-hw1
	#cd dist && tar cvzf parpro1-13-ricordel-hw1.tar.gz ./parpro1-13-ricordel-hw1
	#rm -rf ./dist/parpro1-13-ricordel-hw1
	#a2ps --pretty-print=C --output dist/hw1-code-ricordel.ps \
		#src/mandelbrot.h src/mandelbrot.c src/option_parser.h \
		#src/option_parser.c src/mandelbrot_mpi.c src/mandelbrot_serial.c src/dbg.h
	#ps2pdf dist/hw1-code-ricordel.ps dist/hw1-code-ricordel.pdf



clean:
	rm -f solve_ode_mpi
	rm -f bin/*
	rm -rf ./dist/

run:
	mpirun -np 1 ./solve_ode_mpi

multi_run:
	mpirun -np 4 ./solve_ode_mpi


.PHONY: clean dist run multi_run
