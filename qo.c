#include <glib.h>
#include <stdio.h>

int less (gconstpointer a, gconstpointer b){
	gint *first = (gint *) a;
	gint *second = (gint *) b;
	return *first - *second;
}

void add_ptrs(gpointer data, gpointer dest){
	g_ptr_array_add(dest, data);
}

void delete_array(gpointer arr){
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
}

// The arguments aren't modified here also
GPtrArray * part_constraints(GArray * rels, int part_id){
	GPtrArray * pc = g_ptr_array_new_with_free_func(delete_array);
	for(gint i = 0; (1 << i) < rels->len; i++){
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
		g_ptr_array_add(arr);
	}
}

// This will destroy the old instance and return the new instance. Always pass a copy
GArray * remove_duplicates(GArray * arr){
	if(arr->len == 0)
		return NULL;
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

GPtrArray * 

void print_array(GArray *a){
	for(int i = 0; i < a->len; i++){
		printf("%d ", g_array_index(a, gint, i));
	}
	printf("\n");
}

int main(){
	GPtrArray * one = g_ptr_array_new_with_free_func(delete_array);
	GPtrArray * two = g_ptr_array_new_with_free_func(delete_array);
	GArray * a = g_array_new(FALSE, FALSE, sizeof(gint));
	GArray * c = g_array_new(FALSE, FALSE, sizeof(gint));
	int a1[1] = {1};
	int a3[1] = {4};
	g_array_append_vals(a, &a1, 1);
	g_array_append_vals(c, &a3, 1);
	g_ptr_array_add(one, a);
	g_ptr_array_add(two, c);
	GPtrArray * three = cartesian_product(one, two);
	g_ptr_array_free(three, TRUE);
	// g_ptr_array_free(one, TRUE);
	// g_array_free(a, TRUE);
	//g_array_free(c, TRUE);
	return 0;
}