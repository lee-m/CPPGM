all: pptoken

# build pptoken application
pptoken: pptoken.cpp utf8.cpp utf8.h pptoken_exception.h IPPTokenStream.h DebugPPTokenStream.h
	g++ -g -std=gnu++11 -Wall -o pptoken pptoken.cpp utf8.cpp

# test pptoken application
test: all
	scripts/run_all_tests.pl pptoken my
	scripts/compare_results.pl ref my

# regenerate reference test output
ref-test:
	scripts/run_all_tests.pl pptoken-ref ref

