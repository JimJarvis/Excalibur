CXX = g++
CC = $(CXX)

# =======================================================================
# http://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
# http://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html
#
# -O3 highest level standard-compliant optimization
#  we can also try -Ofast, but no detectable performance gain
#
# -fno-rtti
#  Disable generation of information about every class with virtual functions for use 
#  by the C++ run-time type identification features (‘dynamic_cast’ and ‘typeid’). 
#  If you don't use those parts of the language, you can save some space by using 
#  this flag. Note that exception handling uses the same information, 
#  but G++ generates it as needed.)
#
# -march=native
#  For code intended to run on your system you should choose -march=native, 
#  which will select what is available on your processor. 
#  For instance, if you have a Sandy Bridge, this will enable:
#  -msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2 -mavx.
#
# -flto
#  This option runs the standard link-time optimizer. 
#  When invoked with source code, it generates GIMPLE (one of GCC's internal 
#  representations) and writes it to special ELF sections in the object file. 
#  When the object files are linked together, all the function bodies 
#  are read from these ELF sections and instantiated as if they had been 
#  part of the same translation unit.)
#  The only important thing to keep in mind is that to enable link-time 
#  optimizations the -flto flag needs to be passed to both the compile 
#  and the link commands.
#
# -fwhole-program
#  To make -flto effective, it is necessary to make certain whole program assumptions. 
#  The compiler needs to know what functions and variables can be accessed by libraries 
#  and runtime outside of the link-time optimized unit. When supported by the linker, 
#  the linker plugin (see -fuse-linker-plugin) passes information to the compiler 
#  about used and externally visible symbols. 
#  When the linker plugin is not available, this flag should be used to allow 
#  the compiler to make these assumptions, which leads to more aggressive optimization)
# =======================================================================

CXXFLAGS = -std=c++11 -O3 -fno-rtti -march=native -flto -fwhole-program
LDFLAGS = -pthread $(CXXFLAGS)
CFLAGS = $(CXXFLAGS)

Excalibur: utils.o board.o position.o perft.o movegen.o\
	zobrist.o ttable.o endgame.o material.o pawnshield.o\
	kpkbase.o eval.o search.o uci.o thread.o

Excalibur.o: search.h uci.h

utils.o: utils.h

board.o: board.h

position.o: position.h

perft.o: search.h

movegen.o: position.h

moveorder.o: moveorder.h

zobrist.o: zobrist.h

ttable.o: ttable.h

endgame.o: endgame.h

material.o: material.h

pawnshield.o: pawnshield.h

kpkbase.o: endgame.h

eval.o: eval.h material.h pawnshield.h search.h uci.h

search.o: search.h

uci.o: uci.h search.h thread.h

thread.o: thread.h

.PHONY: clean
clean:
	@rm -rf *~ *.o Excalibur 

.PHONY: all
all: clean Excalibur
	@echo "Forged the sacred sword"
