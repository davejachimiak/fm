fm: ./vm/main.c ./vm/instructions.c ./vm/instructions.h
	gcc ./vm/main.c ./vm/instructions.c -o ./build/fm

tests: fm
	./vm/run_tests

asm-lex: ./asm/lexer.l
	flex ./asm/lexer.l

asm-parse: ./asm/parser.y
	bison ./asm/parser.y -o ./asm/parser.tab.c

assembler: asm-lex asm-parse
	gcc ./vm/instructions.c ./asm/lexer.c ./asm/parser.tab.c -o ./build/fm-asm

asm-tests: assembler
	./asm/run_tests
