ht:
	@echo " Compile ht_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/ht_main.c ./src/hash_file.c -lbf -lm -o ./build/ht_main -O2



# bf:
# 	@echo " Compile bf_main ...";
# 	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/bf_main.c -lbf -o ./build/bf_main -O2

run:
	./build/ht_main

clean:
	rm ./build/ht_main
	rm data.db
