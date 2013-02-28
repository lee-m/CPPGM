all: posttoken

# build posttoken application
# posttoken: posttoken.cpp IPPTokenStream.h DebugPPTokenStream.h
#	g++ -g -std=gnu++11 -Wall -o pptoken pptoken.cpp

# test posttoken application
test: build
	scripts/run_all_tests.pl posttoken my
	scripts/compare_results.pl ref my

# regenerate reference test output
ref-test:
	scripts/run_all_tests.pl posttoken-ref ref

