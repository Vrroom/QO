#include <glib.h>
#include <stdio.h>

struct worker_data{
	GArray * rels;
	int part_id;
	int n_workers;
};

struct worker_data items[4];

// ASSUME Queries are indexed from 0 to n-1
int less (gconstpointer a, gconstpointer b){
	gint *first = (gint *) a;
	gint *second = (gint *) b;
	return *first - *second;
}

int ptr_less (gconstpointer a, gconstpointer b){
	GArray * a1 = *(GArray **) a;
	GArray * b1 = *(GArray **) b;
	return a1->len - b1->len;
}

void add_ptrs(gpointer data, gpointer dest){
	g_ptr_array_add(dest, data);
}

// PASSED TO g_hash_table_new_full and g_array_create wala function
void delete_array(gpointer arr){
	if(arr != NULL)
		g_array_free(arr, TRUE);
}

// constr, q1 and q2 are not modified...
GPtrArray * constrained_power_set(GPtrArray * constr, gint q1, gint q2){
	GPtrArray * cps = g_ptr_array_new_with_free_func(delete_array);
	GArray * empty = g_array_new(FALSE, FALSE, sizeof(gint));
	g_ptr_array_add(cps, empty);
	gboolean include_q1 = TRUE;
	gboolean include_q2 = TRUE;
	for(int i = 0; i < constr->len; i++){
		GArray * ci = g_ptr_array_index(constr, i);
		if(g_array_index(ci, gint, 1) == q1)
			include_q1 = FALSE;
		else
			include_q2 = FALSE;
	}
	if(include_q1){
		GArray * arr = g_array_new(FALSE, FALSE, sizeof(gint));
		g_array_append_val(arr, q1);
		g_ptr_array_add(cps, arr);
	}
	if(include_q2){
		GArray * arr = g_array_new(FALSE, FALSE, sizeof(gint));
		g_array_append_val(arr, q2);
		g_ptr_array_add(cps, arr);
	}
	if(include_q1 && include_q2){
		GArray * arr = g_array_new(FALSE, FALSE, sizeof(gint));
		g_array_append_val(arr, q1);
		g_array_append_val(arr, q2);
		g_ptr_array_add(cps, arr);
	}
	return cps;
}

// The arguments aren't modified here also
GPtrArray * part_constraints(GArray * rels, int part_id,int n_workers){
	GPtrArray * pc = g_ptr_array_new_with_free_func(delete_array);
	for(gint i = 0; (1 << i) < n_workers; i++){
		gint select = part_id & (1 << i);
		gint q1, q2;
		if(select > 0){
			q1 = g_array_index(rels, gint, 2*i + 1);
			q2 = g_array_index(rels, gint, 2*i);			
		}else{
			q1 = g_array_index(rels, gint, 2*i);
			q2 = g_array_index(rels, gint, 2*i + 1);
		}
		GArray * arr = g_array_new(FALSE, FALSE, sizeof(gint));
		g_array_append_val(arr, q1);
		g_array_append_val(arr, q2);
		g_ptr_array_add(pc, arr);
	}
	return pc;
}

// This will destroy the old instance and return the new instance. Always pass a copy
GArray * remove_duplicates(GArray * arr){
	if(arr->len == 0)
		return arr;
	g_array_sort(arr, (GCompareFunc) less);
	GArray * new_arr = g_array_new(FALSE, FALSE, sizeof(gint));
	g_array_append_val(new_arr, g_array_index(arr, gint, 0));
	for(guint i = 1, j = 0; i < arr->len; i++){
		gint tp = g_array_index(new_arr, gint, j);
		gint cur = g_array_index(arr, gint, i);
		if(tp != cur){
			g_array_append_val(new_arr, cur);
			j++;
		}
	}
	g_array_free(arr, TRUE);
	return new_arr;
}

// Insert b at the end of a
void combine (GArray * a, GArray * b){
	for(int i = 0; i < b->len; i++){
		g_array_append_val(a, g_array_index(b, gint, i));
	}
}

// Destroy a and b and return there cartesian product. Always pass a copy
GPtrArray * cartesian_product(GPtrArray * a, GPtrArray * b){
	GPtrArray * new_arr = g_ptr_array_new_with_free_func((GDestroyNotify) delete_array);
	for(int i = 0; i < b->len; i++){
		GPtrArray * uni = g_ptr_array_new();
		for(int j = 0; j < a->len; j++){
			GArray * aj_cpy = g_array_new(FALSE, FALSE, sizeof (gint));
			combine(aj_cpy, g_ptr_array_index(a, j));
			combine(aj_cpy, g_ptr_array_index(b, i));
			aj_cpy = remove_duplicates(aj_cpy);
			g_ptr_array_add(uni, aj_cpy);

		}
		g_ptr_array_foreach(uni, (GFunc) add_ptrs, new_arr);
		g_ptr_array_free(uni, TRUE);
	}
	g_ptr_array_free(a, TRUE);
	g_ptr_array_free(b, TRUE);
	return new_arr;
}

GPtrArray * adm_join_results(GArray * rels, GPtrArray * constr){
	GPtrArray * join_res = g_ptr_array_new_with_free_func((GDestroyNotify) delete_array);
	GArray * empty = g_array_new(FALSE, FALSE, sizeof(gint));
	g_ptr_array_add(join_res, empty);
	for(int i = 0; 2*i + 1 < rels->len; i++){
		gint q1 = g_array_index(rels, gint, 2*i);
		gint q2 = g_array_index(rels, gint, 2*i + 1);
		GPtrArray * cps = constrained_power_set(constr, q1, q2);
		join_res = cartesian_product(join_res, cps);
	}
	return join_res;
}

// If p is better than free the current occupants. 
// Else free p.
void prune(GPtrArray * P, GArray * p, gint bitmap){
	gboolean exists = g_ptr_array_index(P, bitmap) != NULL;
	if(!exists){
		GArray ** arr_at_ind; 
		arr_at_ind = (GArray **) &g_ptr_array_index(P, bitmap);
		*arr_at_ind = p;
		return;
	}
	GArray * val = g_ptr_array_index(P, bitmap);
	gboolean replace = TRUE;
	for(int i = 0; i < p->len; i++){
		gint pi = g_array_index(p, gint, i); 
		gint vali = g_array_index(val, gint, i);
		if(pi < vali){
			break;
		}else if(vali > pi){
			replace = FALSE;
			break;
		}
	}
	if(replace){
		GArray ** arr_at_ind; 
		arr_at_ind = (GArray **) &g_ptr_array_index(P, bitmap);
		g_array_free(*arr_at_ind, TRUE);
		*arr_at_ind = p;
	}else{
		g_array_free(p, TRUE);
	}
}

// Due to the bitmap, we are constrained by joins of 32 tables.
void try_splits(GArray * sub_rels, GPtrArray * constr, GPtrArray * P, gint n){
	GArray * valid = g_array_sized_new(FALSE, FALSE, sizeof(gboolean), n);
	GArray * present = g_array_sized_new(FALSE, FALSE, sizeof(gboolean), n);
	gboolean tr = TRUE;
	gboolean fls = FALSE;
	gint bitmap = 0;
	for(int i = 0; i < n; i++){
		g_array_append_val(present, fls);
		g_array_append_val(valid, tr);
	}
	for(int i = 0; i < sub_rels->len; i++){
		gint num = g_array_index(sub_rels, gint, i);
		bitmap |= (1 << num);
		gboolean *bit = &g_array_index(present, gboolean, num);
		*bit = TRUE;
	}
	for(int i = 0; i < constr->len; i++){
		GArray * ci = g_ptr_array_index(constr, i);
		gint q1 = g_array_index(ci, gint, 0);
		gint q2 = g_array_index(ci, gint, 1);
		if(g_array_index(present, gboolean, q1) 
			&& g_array_index(present, gboolean, q2)){
			gboolean *bit = &g_array_index(valid, gboolean, q1);
			*bit = FALSE;
		}
	}
	for(int i = 0; i < sub_rels->len; i++){
		gint u = g_array_index(sub_rels, gint, i);
		if(g_array_index(valid, gboolean, u)){
			GArray * l_splt = g_array_sized_new(FALSE, FALSE, sizeof(gint), sub_rels->len - 1);
			GArray * r_splt = g_array_new(FALSE, FALSE, sizeof(gint));
			gint l_bitmp = bitmap & ~(1 << u);
			for(int j = 0; j < sub_rels->len; j++){
				if(j != i){
					g_array_append_val(l_splt, g_array_index(sub_rels, gint, j));
				}
			}
			g_array_append_val(r_splt, u);
			GArray * l_sub = g_ptr_array_index(P, l_bitmp);
			GArray * r_sub = g_ptr_array_index(P, (1 << u));
			g_array_free(l_splt, TRUE);
			g_array_free(r_splt, TRUE);
			GArray * comb = g_array_sized_new(FALSE, FALSE, sizeof(gint), sub_rels->len);
			for(int i = 0; i < l_sub->len; i++){
				g_array_append_val(comb, g_array_index(l_sub, gint, i));
			}
			g_array_append_val(comb, g_array_index(r_sub, gint, 0));
			prune(P, comb, bitmap);
		}
	}
	g_array_free(valid, TRUE);
	g_array_free(present, TRUE);	
}

gpointer worker(gpointer data){
	struct worker_data * wi = (struct worker_data *) data;
	GArray * rels = wi->rels;
	int part_id = wi->part_id;
	int n_workers = wi->n_workers;
	int m = rels->len; // REMOVE
	GPtrArray * constr =  part_constraints(rels, part_id, n_workers);
	GPtrArray * join_res = adm_join_results(rels, constr);
	GPtrArray * P = g_ptr_array_new_full((1 << rels->len), delete_array); 
	for(int i = 0; i < (1 << rels->len); i++){
		g_ptr_array_add(P, NULL);
	}
	for(int i = 0; i < rels->len; i++){
		gint q = g_array_index(rels, gint, i);
		GArray * arr = g_array_new(FALSE, FALSE, sizeof(gint));
		g_array_append_val(arr, q);
		GArray ** arr_at_ind; 
		arr_at_ind = (GArray **) &g_ptr_array_index(P,(1 << q));
		*arr_at_ind = arr;
	}
	g_ptr_array_sort(join_res, ptr_less);
	for(int i = 0; i < join_res->len; i++){
		GArray * q = g_ptr_array_index(join_res, i);
		if(q->len > 1){
			try_splits(q, constr, P, rels->len);
		}
	}
	GArray * result = g_array_sized_new(FALSE, FALSE, sizeof(gint), rels->len);
	GArray * best = g_ptr_array_index(P, ((1 << rels->len) - 1));
	for(int i = 0; i < rels->len; i++){
		g_array_append_val(result, g_array_index(best, gint, i));
	}
	g_ptr_array_free(P, TRUE);
	g_ptr_array_free(join_res, TRUE);
	g_ptr_array_free(constr, TRUE);
	return result;
}

void print_array(GArray *a){
	for(int i = 0; i < a->len; i++){
		printf("%d ", g_array_index(a, gint, i));
	}
	printf("\n");
}

GArray * set_best(GArray * a, GArray * b){
	GArray * best = a;
	GArray * nbest = b;
	for(int i = 0; i < a->len; i++){
		int ai = g_array_index(a, gint, i);
		int bi = g_array_index(b, gint, i);
		if(ai > bi){
			break;
		}else if(bi < ai){
			best = b;
			nbest = a;
			break;
		}
	}
	g_array_free(nbest, TRUE);
	return best;
}

void master(){
	int n_workers = 1;
	//int r[16] = {0,2,3,1,4,5,9,8,7,10,11,12,15,14,13,6};
	int r[4] = {0,3,2,1};
	GArray * rels = g_array_new(FALSE, FALSE, sizeof(gint));
	rels = g_array_append_vals(rels, &r, 4);
	gchar * name = "";
	GPtrArray * threads = g_ptr_array_new();
	for(int i = 0; i < n_workers; i++){
		items[i].rels = rels;
		items[i].n_workers = n_workers;
		items[i].part_id = i + 1;
		GThread * t = g_thread_new(name, worker, &items[i]);
		g_ptr_array_add(threads, t);
	}
	GArray * best = g_thread_join(g_ptr_array_index(threads, 0));
	for(int i = 1; i < threads->len; i++){
		GArray * that = g_thread_join(g_ptr_array_index(threads, i));
		best = set_best(best, that);
	}
	print_array(best);
}

int main(){
	master();
	return 0;
}