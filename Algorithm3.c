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
	group->next = NULL;
	D->divisions = nextGroup;
	D->len = (D->len) - 1;
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
	/*free(get_private((struct _spmat*)g->groupSubmatrix));*/
	/*free(g->groupSubmatrix);*/
	/*free(g);*/
}

/* splitByS helper, copies list_s to list_t*/
void int_list_copy(int size, int *list_t, int *list_s) {
	int i;
	for (i = 0; i < size; i++) {
		list_t[i] = list_s[i];
	}
}

/* splitByS helper, copies node_s to node_t*/
void spmat_node_copy(struct spmat_node* node_t, struct spmat_node* node_s) {
	node_t->data = node_s->data;
	node_t->index = node_s->index;
	node_t->node_name = node_s->node_name;
	node_t->next = node_s->next;
}

/* splitByS helper, copies list_s to list_t*/
void spmat_node_list_copy(int size, struct spmat_node** list_t, struct spmat_node** list_s) {
	int i;
	for (i = 0; i < size; i++) {
		if (list_s[i] != NULL) {
			list_t[i] = (struct spmat_node*)malloc(sizeof(struct spmat_node));
			spmat_node_copy(list_t[i], list_s[i]);
		}
	}
}

/* splitByS helper, deep copies spmat_s to spmat_t*/
void spmat_deep_copy(int size, struct _spmat *spmat_t, struct _spmat *spmat_s) {
	struct spmat_node **gt_rows, **gs_rows;

	gs_rows = get_private(spmat_s);
	gt_rows = get_private(spmat_t);

	spmat_node_list_copy(size, gt_rows, gs_rows);

	/* copy*/
	spmat_t->n = spmat_s->n;
	spmat_t->add_row = spmat_s->add_row;
	spmat_t->mult = spmat_s->mult;
	spmat_t->free = spmat_s->free;
	spmat_t->private = gt_rows;
}

/* splitByS helper, deep copies gs to gt*/
void divisionGroup_deep_copy(int gt_size, struct divisionGroup* gt, struct divisionGroup* gs){
	struct _spmat *gt_mat, *gs_mat;
	int *gt_sum_of_rows, *gt_group_members, *gs_sum_of_rows, *gs_group_members;

	/* allocate spmat*/
	gt_mat = spmat_allocate_list(gt_size);
	/* allocate sumOfRows*/
	gt_sum_of_rows = (int*)malloc(gt_size * sizeof(int));
	assert(gt_sum_of_rows != NULL);							/* TODO: error module*/
	/* allocate groupMembers*/
	gt_group_members = (int*)malloc(gt_size * sizeof(int));
	assert(gt_group_members != NULL);						/* TODO: error module*/

	gs_mat = (struct _spmat*)gs->groupSubmatrix;
	gs_sum_of_rows = (int*)gs->sumOfRows;
	gs_group_members = (int*)gs->groupMembers;

	spmat_deep_copy(gt_size, gt_mat, gs_mat);
	int_list_copy(gt_size, gt_sum_of_rows, gs_sum_of_rows);
	int_list_copy(gt_size, gt_group_members, gs_group_members);

	gt->groupSize = gs->groupSize;
	gt->groupMembers = gt_group_members;
	gt->groupSubmatrix = gt_mat;
	gt->sumOfRows = gt_sum_of_rows;
}

/* splits g to groups, populates g1 and g2
 * if there's a group of size 0, g1 = g, g2 = NULL */
struct divisionGroup* splitByS(double* vectorS, struct divisionGroup* g, struct divisionGroup* g1) {
	int i, n, g1_size, g2_size, i1, i2;
	struct _spmat *g1_mat;
	struct _spmat *g2_mat;
	struct spmat_node **g_rows, **g1_rows, **g2_rows;
	struct divisionGroup* g2;
	int *g_sum_of_rows, *g1_sum_of_rows, *g2_sum_of_rows;
	int *g1_group_members, *g2_group_members, *g_group_members;

	g2 = (struct divisionGroup*)malloc(sizeof(struct divisionGroup));
	assert(g2 != NULL);										/* TODO: error module*/

	printf("%s", "SplitByS 1\n");
	n = g->groupSize;
	g_rows = get_private((struct _spmat*)g->groupSubmatrix);
	g_sum_of_rows = g->sumOfRows;
	g_group_members = g->groupMembers;
	printf("%s", "SplitByS 2\n");

	/* if there's a group of size 0, g1 = g, g2 = NULL
	 * in this case, no need to free g*/
	g1_size = calc_size(vectorS, n);
	printf("%s", "SplitByS 3\n");

	g2_size = n - g1_size;
	if (g1_size == n || g2_size == n) {
		printf("%s", "SplitByS 3a\n");
		divisionGroup_deep_copy(n, g1, g);
		/*TODO: free g pointer, g2 pointer?*/
		printf("%s", "SplitByS 3b\n");
		g2 = NULL;
		return g2;
	}
	printf("%s", "SplitByS 4\n");

	/* allocate spmats*/
	g1_mat = spmat_allocate_list(g1_size);
	g2_mat = spmat_allocate_list(g2_size);
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
	g1_rows = (struct spmat_node**)malloc(g1_size * sizeof(struct spmat_node*));
	assert(g1_group_members != NULL);						/* TODO: error module*/
	g2_rows = (struct spmat_node**)malloc(g2_size * sizeof(struct spmat_node*));
	assert(g2_group_members != NULL);						/* TODO: error module*/

	printf("%s", "SplitByS 5\n");

	/* move rows from g to g1, g2*/
	i1 = 0;
	i2 = 0;
	for (i = 0; i < n; i++) {
		if (vectorS[i] == 1) {
			g1_rows[i1] = g_rows[i];
			g1_sum_of_rows[i1] = g_sum_of_rows[i];
			i1++;
		} else {
			g2_rows[i2] = g_rows[i];
			g2_sum_of_rows[i2] = g_sum_of_rows[i];
			i2++;
		}
	}

	printf("%s", "SplitByS 6\n");

	/* edit rows and sum_of_rows, remove irrelevant nodes*/
	update_mat_rows(vectorS, g1_size, 1, g1_rows, g1_sum_of_rows);
	update_mat_rows(vectorS, g2_size, -1, g2_rows, g2_sum_of_rows);
	/* update group_members*/
	update_group_members(vectorS, g_group_members, g1_group_members, 1, n);
	update_group_members(vectorS, g_group_members, g2_group_members, -1, n);
	/* update index value=s of the nodes*/
	update_mat_rows_index(g1_group_members, g1_size, g1_rows);
	update_mat_rows_index(g2_group_members, g2_size, g2_rows);
	/* update spmats*/
	set_private(g1_mat, g1_rows);
	set_private(g2_mat, g2_rows);
	/* create divisionGroups*/
	create_division_group(g1, g1_size, g1_mat, g1_sum_of_rows, g1_group_members);
	create_division_group(g2, g2_size, g2_mat, g2_sum_of_rows, g2_group_members);
	/* free memory*/
	/*free_div_group(g);*/
	printf("%s", "SplitByS 7\n");

	return g2;
}

struct division* new_division() {
	struct division* D = (struct division*) malloc(sizeof(struct division));
	if (D == NULL)
		exit(1);  /*TODO: print error before exit.*/
	D->len = 0;
	D->divisions = NULL;
	return D;
}


double sumOfRow(int row,struct spmat_node** gt_rows)
{
	double sum = 0;
	struct spmat_node* cur;
	cur = gt_rows[row];

	if(cur == NULL) return 0;
	else while(cur != NULL)
	{
		sum+=cur->data;
		cur = cur->next;
	}
	return sum;
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
	int cnt;
	struct division* P = new_division();
	struct division* O = new_division();
	add_groupDivision(P, createTrivialDivision( inputGraph));
	/* TODO: calc sum of rows*/

	cnt = 0;
	while (P->len > 0) {

		g1 = (struct divisionGroup*)malloc(sizeof(struct divisionGroup));
		printf("%d \n", cnt);
		g = removeFirstGroup(P);
		printf("%s", "POST remove \n");
		vectorS = (double*) malloc(g->groupSize * sizeof(double)); /*vectorS is size of group g.*/
		printf("%s", "POST malloc \n");
		if (vectorS == NULL)
		{
			printf("%s", "vectorS malloc failed algo3 \n");
			exit(1); /*TODO: print error before exit.*/
		}

		cnt++;
		Algorithm2(vectorS, g,inputGraph);
		printf("%s", "POST algo2 \n");
		/*modularityMaximization(inputGraph,vectorS, g);
		printf("%s", "POST modMax \n");*/
		g2 = splitByS(vectorS, g, g1);
		printf("%s", "POST splitByS \n");

		if (g2 == NULL)
		{
			printf("%s", "PRE g2=null \n");
			add_groupDivision(O, g1);
			printf("%s", "POST g2=null \n");
		}

		else {
			printf("%s", "PRE update division \n");
			updateDivisionPostSplit(g1, P, O);
			updateDivisionPostSplit(g2, P, O);
			printf("%s", "POST update division \n");
		}

		free(vectorS);

	}
	printf("%s", "return algo 3\n");
	return O;

}

