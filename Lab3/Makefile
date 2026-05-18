all:
	rm -f main lex.yy.c syntax.tab.c syntax.tab.h parser out1.ir out2.ir
	bison -d syntax.y
	flex lexical.l
	gcc syntax.tab.c lex.yy.c main.c symbol_table.c semantic.c irgen.c -o parser -lfl
clean:
	rm -f main lex.yy.c syntax.tab.c syntax.tab.h parser out1.ir out2.ir
test:
	./parser 1.cmm out1.ir
	./parser 2.cmm out2.ir
