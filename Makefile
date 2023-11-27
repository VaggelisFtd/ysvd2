ht:
	@echo " Compile ht_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/ht_main.c ./src/hash_file.c -lbf -lm -o ./build/runner -O2

run:
	./build/runner

clean:
	rm ./build/runner
	rm data.db
