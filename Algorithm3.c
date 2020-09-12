#include <stdio.h>
#include <stdlib.h>
#include "modules.h"
#include "spmat.h"
#include "Algorithm2.h"
#include "ModularityMaximization.h"
#include <assert.h>

/*TODO: is include file.c ok? or should we do headers?*/
/*TODO: add checks for all mallocs.*/
/*adds in start*/
void add_groupDivision(struct division* D, struct divisionGroup* g) {
	struct node* add = (struct node*) malloc(sizeof(struct node));
	if (add == NULL)
		exit(1); /*TODO: print error before exit.*/
	add->data.group = g;
	add->next = NULL;
	if (D->len == 0)
		D->divisions = add;
	else
	{
		add->next = D->divisions;
		 D->divisions = add; /*todo make sure no cycle*/
	}

	D->len = (D->len) + 1;
}

/* removes  first group from D and returns it*/
struct divisionGroup* removeFirstGroup(struct division* D) {
	struct node* group;
	struct node* nextGroup;

	group= D->divisions;
	nextGroup= group->next;
	printf("%s", "removeFirstGroup B 3\n");
	group->next = NULL;
	D->divisions = nextGroup;
	D->len = (D->len) - 1;
	printf("%s", "removeFirstGroup END algo 3\n");
	return group->data.group;
}

void updateDivisionPostSplit(struct divisionGroup* g, struct division* P,
		struct division* O) {
	if (g->groupSize == 1)
		add_groupDivision(O, g);
	else
		add_groupDivision(P, g);

}

/* splitByS helper, returns the size of g1*/
int calc_size (double* vectorS, int n) {
	int i, size;
	size = 0;

	for (i = 0; i < n; i++) {
		if (vectorS[i] == 1) {
			size++;
		}
	}
	return size;
}

/* splitByS helper, updates the mat rows,
 * removes irrelevant nodes and updates sum of rows accordingly*/
void update_mat_rows(double* vectorS, int num_members, int group_indicator, struct spmat_node** gt_rows, int* gt_sum_of_rows) {
	int i_row;
	struct spmat_node *cur, *prev, *next;

	/* delete irrelevant nodes*/
	for (i_row = 0; i_row < num_members; i_row++) {
		prev = NULL;
		cur = gt_rows[i_row];
		while (cur != NULL) {
			next = cur->next;

			/* remove cur, prev remains the same*/
			if (vectorS[cur->index] != group_indicator) {
				gt_sum_of_rows[i_row]--;
				if (prev == NULL) {
					gt_rows[i_row] = cur->next;
				} else {
					prev->next = cur->next;
				}
				free(cur);
			/* don't remove cur, prev should advance*/
			} else {
				prev = cur;
			}
			cur = next;
		}
	}
}

/* splitByS helper, updates nodes index according to the new mat */
void update_mat_rows_index(int* g_group_members, int num_members, struct spmat_node** g_rows) {
	int i_row, count, i;
	int* map_name_to_col_index;
	struct spmat_node *cur;

	/* create a map, will be used to update index field to fit the new column index in the new spmat*/
	count = 0;
	map_name_to_col_index = (int*) malloc((g_group_members[num_members-1]+1) * sizeof(int));
	for (i = 0; i < num_members; i++) {
		map_name_to_col_index[g_group_members[i]] = count;
		count++;
	}

	/* delete irrelevant nodes*/
	for (i_row = 0; i_row < num_members; i_row++) {
		cur = g_rows[i_row];
		while (cur != NULL) {
			/*printf("%d \n",i_row);
			printf("%d \n",cur->index);
			printf("%d \n",cur->node_name);*/
			cur->index = map_name_to_col_index[cur->node_name];
			cur = cur->next;
		}
	}
	free(map_name_to_col_index);
}

/* splitByS helper, updates group members of target, according to group indicator (+-1)*/
void update_group_members(double* vectorS, int* source_group_members, int* target_group_members, int group_indicator, int n) {
	int i, i_target;
	i_target = 0;

	for (i = 0; i < n; i++) {
		if (vectorS[i] == group_indicator) {
			target_group_members[i_target] = source_group_members[i];
			i_target++;
		}
	}
}

/* splitByS helper, updates the group fields*/
void create_division_group(struct divisionGroup* g, int size, struct _spmat* mat, int* sum_of_rows, int* group_members) {
	g->groupSize = size;
	g->groupSubmatrix = mat;
	g->sumOfRows = sum_of_rows;
	g->groupMembers = group_members;
}

/*splitByS helper, free old div group*/
void free_div_group(struct divisionGroup* g) {

	free(g->sumOfRows);
	free(g->groupMembers);

	/*free submatrix, don't use free_ll, since we don't want to free rows*/
	free(get_private((struct _spmat*)g->groupSubmatrix));
	free(g->groupSubmatrix);
	free(g);
}

/* splits g to groups, populates g1 and g2
 * if there's a group of size 0, g1 = g, g2 = NULL */
void splitByS(double* vectorS, struct divisionGroup* g, struct divisionGroup* g1, struct divisionGroup* g2) {
	int i, n, g1_size, g2_size, i1, i2;
	struct _spmat *g1_mat;
	struct _spmat *g2_mat;
	struct spmat_node **g_rows, **g1_rows, **g2_rows;
	int *g_sum_of_rows, *g1_sum_of_rows, *g2_sum_of_rows;
	int *g1_group_members, *g2_group_members, *g_group_members;

	n = g->groupSize;
	g_rows = get_private((struct _spmat*)g->groupSubmatrix);
	g_sum_of_rows = g->sumOfRows;
	g_group_members = g->groupMembers;
	printf("%s", "splitbyS A\n");
	/* if there's a group of size 0, g1 = g, g2 = NULL
	 * in this case, no need to free g*/
	g1_size = calc_size(vectorS, n);
	printf("%s", "splitbyS B\n");
	g2_size = n - g1_size;
	if (g1_size == 0 || g2_size == 0) {
		g1 = g;
		g2 = NULL;
		return;
	}

	/* allocate spmats*/
	g1_mat = spmat_allocate_list(g1_size);
	g2_mat = spmat_allocate_list(g2_size);
	printf("%s", "splitbyS C\n");
	/* allocate sumOfRows*/
	g1_sum_of_rows = (int*)malloc(g1_size * sizeof(int));
	assert(g1_sum_of_rows != NULL);							/* TODO: error module*/
	g2_sum_of_rows = (int*)malloc(g2_size * sizeof(int));
	assert(g2_sum_of_rows != NULL);							/* TODO: error module*/
	/* allocate groupMembers*/
	g1_group_members = (int*)malloc(g1_size * sizeof(int));
	assert(g1_group_members != NULL);						/* TODO: error module*/
	g2_group_members = (int*)malloc(g2_size * sizeof(int));
	assert(g2_group_members != NULL);						/* TODO: error module*/
	/* allocate rows*/
	printf("%s", "splitbyS C1\n");
	g1_rows = (struct spmat_node**)malloc(g1_size * sizeof(struct spmat_node*));
	assert(g1_group_members != NULL);						/* TODO: error module*/
	g2_rows = (struct spmat_node**)malloc(g2_size * sizeof(struct spmat_node*));
	assert(g2_group_members != NULL);						/* TODO: error module*/
	printf("%s", "splitbyS C2\n");
	/* move rows from g to g1, g2*/
	i1 = 0;
	i2 = 0;
	for (i = 0; i < n; i++) {
		if (vectorS[i] == 1) {

			/* printf("%d \n", g_rows[i][0].index);
			printf("%d \n", g_rows[i][0].node_name);*/
			g1_rows[i1] = g_rows[i];
			printf("%s", "splitbyS C3 B\n");
			g1_sum_of_rows[i1] = g_sum_of_rows[i];
			i1++;
		} else {
			printf("%s", "splitbyS C4\n");
			g2_rows[i2] = g_rows[i];
			g2_sum_of_rows[i2] = g_sum_of_rows[i];
			i2++;
		}
	}
	printf("%s", "splitbyS D\n");
	/* edit rows and sum_of_rows, remove irrelevant nodes*/
	update_mat_rows(vectorS, g1_size, 1, g1_rows, g1_sum_of_rows);
	update_mat_rows(vectorS, g2_size, -1, g2_rows, g2_sum_of_rows);
	/* update group_members*/
	update_group_members(vectorS, g_group_members, g1_group_members, 1, n);
	update_group_members(vectorS, g_group_members, g2_group_members, -1, n);
	/* update index value=s of the nodes*/
	update_mat_rows_index(g1_group_members, g1_size, g1_rows);
	update_mat_rows_index(g2_group_members, g2_size, g2_rows);
	/* create divisionGroups*/
	create_division_group(g1, g1_size, g1_mat, g1_sum_of_rows, g1_group_members);
	create_division_group(g2, g2_size, g2_mat, g2_sum_of_rows, g2_group_members);
	/* update spmats*/
	set_private(g1_mat, g1_rows);
	set_private(g2_mat, g2_rows);
	/* free memory*/
	/*free_div_group(g);*/
}

struct division* new_division() {
	struct division* D = (struct division*) malloc(sizeof(struct division));
	if (D == NULL)
		exit(1);  /*TODO: print error before exit.*/
	D->len = 0;
	D->divisions = NULL;
	return D;
}


/* PARAMS : n is number of nodes in graph
 * DESC : Returns a divisonGroup with all nodes in graph
 */
struct divisionGroup* createTrivialDivision(struct graph* inputGraph) {
	int i;
	int n;
	int* group_members;
	struct divisionGroup* group = (struct divisionGroup*)malloc(sizeof(struct divisionGroup));
	if (group == NULL)
		exit(1); /* TODO: error module*/
	n = inputGraph -> numOfNodes;
	group->groupSize = n;
	group->groupSubmatrix = (inputGraph->A);
	group->sumOfRows = (int*) malloc(n * sizeof(int));
	group_members = (int*)malloc(n * sizeof(int));
	assert(group_members != NULL);						/* TODO: error module*/
	for (i = 0; i < n; i++) {
		group->sumOfRows[i] = 0;
		group_members[i] = i;
	}
	group->groupMembers = group_members;

	return group;
}


struct division* Algorithm3(struct graph* inputGraph) {
	struct divisionGroup* g;
	struct divisionGroup* g1;
	struct divisionGroup* g2;
	double* vectorS;
	struct division* P = new_division();
	struct division* O = new_division();
	add_groupDivision(P, createTrivialDivision( inputGraph));
	/* TODO: calc sum of rows*/

	g1 = (struct divisionGroup*)malloc(sizeof(struct divisionGroup));
	g2 = (struct divisionGroup*)malloc(sizeof(struct divisionGroup));
	while (P->len > 0) {
		printf("%s", "startLoop algo 3 \n");
		g = removeFirstGroup(P);
		printf("%s", "removed FirstGroup algo 3 \n");
		vectorS = (double*) malloc(g->groupSize * sizeof(double)); /*vectorS is size of group g.*/
		if (vectorS == NULL)
		{
			printf("%s", "vectorS malloc failed algo3 \n");
			exit(1); /*TODO: print error before exit.*/
		}

		printf("%s", "callALgo2 \n");
		Algorithm2(vectorS, g,inputGraph);
		printf("%s", "return algo2 algo 3\n");
		modularityMaximization(inputGraph,vectorS, g);
		printf("%s", "pre splitbyS algo3\n");
		splitByS(vectorS, g, g1, g2);
		printf("%s", "post splitbyS algo3\n");
		if (g2 == NULL)
			add_groupDivision(O, g1);

		else {
			updateDivisionPostSplit(g1, P, O);
			updateDivisionPostSplit(g2, P, O);
		}
		free(vectorS);
	}
	printf("%s", "return algo 3\n");
	return O;

}

