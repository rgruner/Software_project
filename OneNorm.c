#include <stdio.h>
#include <stdlib.h>
#include "modules.h"
#include "spmat.h"
/*TODO make sure include c is fine*/

double columnSum(struct graph* graph, struct divisionGroup* g, int column) {
	double sum = 0;
	int cnt;
	struct spmat_node* currentNode;
	 currentNode = get_private(g->groupSubmatrix)[column];
	/*iterate over all rows*/
	 cnt = 0;
	while(currentNode != NULL) { /*since matrix is sparse, we don't know how many rows there are */
		sum = sum + (currentNode->data) - g->sumOfRows[column];
		sum = sum
				- ((graph->vectorDegrees[g->groupMembers[cnt++]] * graph->vectorDegrees[column])
						/ graph->M);
		currentNode = currentNode->next;

	}
	return sum;
}


/* method to calculate the 1-norm of matrix mat.
 TODO: check if double is necessary */
double one_norm(struct graph* graph, struct divisionGroup* g) {
	double maxColumn = 0;
	double currentSum = 0;
	int i;
	/* iterate through columns*/
	for (i = 0; i < g->groupSize; i++) {
		currentSum = columnSum(graph, g, i);
		if (maxColumn < currentSum)
			maxColumn = currentSum;
	}
	return maxColumn;
}



