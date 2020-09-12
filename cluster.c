#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "modules.h"
#include "spmat.h"
#include "Algorithm3.h"

/* remove assert todo*/

void read_row(int i, int n, FILE* input, struct _spmat* A) {
	double* row;
	int k, j, cur;

	row = (double*)malloc(n * sizeof(double));
	assert(row!=NULL);				/* TODO: error module*/

	for (j = 0; j < n; j++) {
		k = fread(&cur, sizeof(int), 1, input);
		assert(k==1); 				/* TODO: error module*/
		row[j] = cur;
	}
	add_row_of_size_n(A, row, i, n, 1);
	free(row);
}

void create_graph(FILE* input, struct graph* new_graph) {
	struct _spmat* A;
	long* vector_degrees;
	int i;
	int k;
	int n;
	int cur_deg;
	int deg_sum;
	/*allocating memory*/
	k = fread(&n, sizeof(int), 1, input);
	assert(k==1); 					/* TODO: error module*/
	A = spmat_allocate_list(n);
	vector_degrees = (long*)malloc(n * sizeof(long));
	assert(vector_degrees!=NULL);	/* TODO: error module*/

	/*reading input to struct*/
	deg_sum = 0;
	for (i = 0; i < n; i++) {
		k = fread(&cur_deg, sizeof(int), 1, input);
		assert(k==1); 				/* TODO: error module*/
		deg_sum += cur_deg;
		vector_degrees[i] = cur_deg;
		read_row(i, cur_deg, input, A);
	}

	/*initializing graph*/
	new_graph->A = A;
	new_graph->vectorDegrees = vector_degrees;
	new_graph->M = deg_sum;
	new_graph->numOfNodes = n;
}


int main(int args, char** argv) {
	struct graph* inputGraph;
	FILE* input;
	if(args != 2) exit(1); /* todo error module*/
	inputGraph = (struct graph*)malloc(sizeof(struct graph));
	assert(inputGraph!=NULL);		/* TODO: error module*/
	input = fopen(argv[1], "r");
	assert(input!=NULL);			/* TODO: error module*/
	create_graph(input, inputGraph);
	setvbuf (stdout, NULL, _IONBF, 0);
	printf("%s  \n", "call Algo 3");
	Algorithm3(inputGraph);

	printf("%s", "done main\n");
	return 1; /*todo: check ok */
}
