compile:
	gcc main.c instructions.c -o ./build/fm

tests: compile
	./run_tests
