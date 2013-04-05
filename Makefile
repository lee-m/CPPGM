all: pptoken

# build pptoken application
pptoken: preprocessor_lexer.cpp preprocessor_lexer.h utf8.cpp utf8.h
	g++ -g3 -std=gnu++11 -Wall -o pptoken preprocessor_lexer.cpp utf8.cpp main.cpp

# test pptoken application
test: all
	scripts/run_all_tests.pl pptoken my
	scripts/compare_results.pl ref my

# regenerate reference test output
ref-test:
	scripts/run_all_tests.pl pptoken-ref ref

