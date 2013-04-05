LIB_PATH = -L./compiler
INC_PATH = -I./compiler/preprocessor/include
CFLAGS   = -g3 -std=gnu++11 -Wall $(LIB_PATH) $(INC_PATH) -o pptoken
OBJLIBS	 = libpreprocessor.a

all: pptoken

# build pptoken application
pptoken: main.cpp $(OBJLIBS)
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

