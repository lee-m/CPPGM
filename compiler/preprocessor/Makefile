CFLAGS = -c -g -std=gnu++11 -Wall -I./include
OBJS   = preprocessor_lexer.o utf8.o
LIB    = ../libpreprocessor.a

all: $(LIB)

../libpreprocessor.a: $(OBJS)
	ar rcs $(LIB) $(OBJS)
	rm $(OBJS)

preprocessor_lexer.o: ./src/preprocessor_lexer.cpp ./include/preprocessor_lexer.h ./include/utf8.h
	g++ $(CFLAGS) -o preprocessor_lexer.o ./src/preprocessor_lexer.cpp
utf8.o: ./src/utf8.cpp ./include/utf8.h	
	g++ $(CFLAGS) ./src/utf8.cpp



