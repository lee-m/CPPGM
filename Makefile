LIB_PATH = -L./compiler
INC_PATH = -I./compiler/preprocessor/include
CFLAGS   = -g3 -std=gnu++11 -Wall $(LIB_PATH) $(INC_PATH) -o posttoken
OBJLIBS	 = libpreprocessor.a

all: posttoken

posttoken: posttoken.cpp $(OBJLIBS)
	g++ $(CFLAGS) main.cpp -lpreprocessor

libpreprocessor.a: force_look
	cd ./compiler/preprocessor; $(MAKE)

force_look:
	true

# test pptoken application
test: all
	scripts/run_all_tests.pl pptoken my
	scripts/compare_results.pl ref my

# regenerate reference test output
ref-test:
	scripts/run_all_tests.pl pptoken-ref ref

