CFLAGS   = -g3 -std=gnu++11 -Wall -L./compiler -I./compiler/include -o posttoken
OBJLIBS	 = libcompiler.a

all: posttoken

posttoken: posttoken.cpp $(OBJLIBS)
	g++ $(CFLAGS) main.cpp -lcompiler

libcompiler.a: force_look
	cd ./compiler; $(MAKE)

force_look:
	true

# test pptoken application
test: all
	scripts/run_all_tests.pl posttoken my
	scripts/compare_results.pl ref my

# regenerate reference test output
ref-test:
	scripts/run_all_tests.pl posttoken-ref ref

