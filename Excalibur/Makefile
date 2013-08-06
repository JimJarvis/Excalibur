CC = g++
CXX = g++
CFLAGS = -std=c++11
LDFLAGS = -pthread
CXXFLAGS = -std=c++11

Excalibur: utils.o board.o position.o perft.o move.o movegen.o\
	zobrist.o ttable.o endgame.o material.o pawnstruct.o\
	kpkbase.o eval.o search.o uci.o thread.o

Excalibur.o: search.h uci.h

utils.o: utils.h

board.o: board.h

position.o: position.h

perft.o: search.h

move.o: move.h

movegen.o: position.h

zobrist.o: zobrist.h

ttable.o: ttable.h

endgame.o: endgame.h

material.o: material.h

pawnstruct.o: pawnstruct.h

kpkbase.o: position.h

eval.o: eval.h

search.o: search.h

uci.o: uci.h

thread.o: thread.h

.PHONY: clean
clean:
	@rm -rf *~ *.o Excalibur 

.PHONY: all
all: clean Excalibur
	@echo "Forged the sacred sword"