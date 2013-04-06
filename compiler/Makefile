CFLAGS     = -c -g -std=gnu++11 -Wall -I./include
PP_OBJS    = preprocessor_lexer.o  preprocessor.o
LEXER_OBJS = lexer.o
UTIL_OBJS  = utf8.o
OBJS       = $(PP_OBJS) $(LEXER_OBJS) $(UTIL_OBJS)
LIB        = libcompiler.a

all: $(LIB)

libcompiler.a: $(OBJS)
	ar rcs $(LIB) $(OBJS)
	rm $(OBJS)

#Preprocessor
preprocessor_lexer.o: ./src/preprocessor/preprocessor_lexer.cpp ./include/preprocessor/preprocessor_lexer.h ./include/util/utf8.h
	g++ $(CFLAGS) -o preprocessor_lexer.o ./src/preprocessor/preprocessor_lexer.cpp

preprocessor.o: ./src/preprocessor/preprocessor.cpp ./include/preprocessor/preprocessor.h
	g++ $(CFLAGS) -o preprocessor.o ./src/preprocessor/preprocessor.cpp

#Lexer
lexer.o: ./src/lexer/lexer.cpp ./include/lexer/lexer.h
	g++ $(CFLAGS) -o lexer.o ./src/lexer/lexer.cpp

#Utils
utf8.o: ./src/util/utf8.cpp ./include/util/utf8.h
	g++ $(CFLAGS) ./src/util/utf8.cpp




