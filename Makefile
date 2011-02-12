convolution: convolution.c
	gcc -lsndfile -Os --std=c99 $< -o $@
