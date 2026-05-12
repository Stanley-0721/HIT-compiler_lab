all:
	rm -f main lex.yy.c syntax.tab.c syntax.tab.h parser
	bison -d syntax.y
	flex lexical.l
	gcc syntax.tab.c lex.yy.c main.c symbol_table.c semantic.c -o parser -lfl
clean:
	rm -f main lex.yy.c syntax.tab.c syntax.tab.h parser
test:
	./parser 1.cmm
	./parser 2.cmm
	./parser 3.cmm
	./parser 4.cmm
	./parser 5.cmm
	./parser 6.cmm
	./parser 7.cmm
	./parser 8.cmm
	./parser 9.cmm
	./parser 10.cmm
	./parser 11.cmm
	./parser 12.cmm
	./parser 13.cmm
	./parser 14.cmm
	./parser 15.cmm
	./parser 16.cmm
	./parser 17.cmm
