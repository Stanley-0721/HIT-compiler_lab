all:
	rm -f main lex.yy.c syntax.tab.c syntax.tab.h parser
	bison -d syntax.y
	flex lexical.l
	gcc syntax.tab.c lex.yy.c main.c -o parser -lfl
clean:
	rm -f main lex.yy.c syntax.tab.c syntax.tab.h parser
test:
	./parser 1.cmm
	./parser 2.cmm
	./parser 3.cmm
	./parser 4.cmm
