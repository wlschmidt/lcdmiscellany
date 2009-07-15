#include "List.h"
#include "../../vm.h"
#include "../../malloc.h"

void clear(ScriptValue &s, ScriptValue *args) {
	if (args->type != SCRIPT_LIST || args->listVal->numVals == 0) {
		return;
	}
	int i = 0;
	if (args->listVal->vals[0].type == SCRIPT_LIST ||
		args->listVal->vals[0].type == SCRIPT_DICT) {
			s = args->listVal->vals[0];
			s.AddRef();
	}
	do {
		if (args->listVal->vals[i].type == SCRIPT_LIST) {
			ListValue * l = args->listVal->vals[i].listVal;
			for (int j=0; j<l->numVals; j++) {
				l->vals[j].Release();
			}
			l->numVals = 0;
		}
		else if (args->listVal->vals[i].type == SCRIPT_DICT) {
			DictValue * d = args->listVal->vals[i].dictVal;
			for (int j=0; j<d->numEntries; j++) {
				d->entries[j].key.Release();
				d->entries[j].val.Release();
			}
			d->numEntries = 0;
		}
		i++;
	}
	while (i < args->listVal->numVals);
}

void resize(ScriptValue &s, ScriptValue *args) {
	if (args->type != SCRIPT_LIST || args->listVal->numVals >= 2) {
		return;
	}
	int i = 0;
	args->listVal->vals[1].AddRef();
	ScriptValue sv;
	CoerceIntNoRelease(args->listVal->vals[1], sv);
	if (sv.intVal < 0) i = 0;
	if (args->listVal->vals[0].type == SCRIPT_LIST) {
		if (args->listVal->vals[0].listVal->Resize(i, i|2)) {
			s = args->listVal->vals[0];
			s.AddRef();
		}
	}
}

void realloc(ScriptValue &s, ScriptValue *args) {
	if (args->type != SCRIPT_LIST || args->listVal->numVals < 2 || args->listVal->vals[0].type != SCRIPT_LIST) {
		return;
	}
	int i = 0;
	args->listVal->vals[1].AddRef();
	ScriptValue sv;
	CoerceIntNoRelease(args->listVal->vals[1], sv);
	if (sv.intVal < 1) i = 1;
	if (i < args->listVal->vals[0].listVal->numVals) i = args->listVal->vals[0].listVal->numVals;
	if (!srealloc(args->listVal->vals[0].listVal->vals, sizeof(ScriptValue) * i)) {
		return;
	}
	args->listVal->vals[0].listVal->size = i;
}

void push_back(ScriptValue &s, ScriptValue *args) {
	if (args->type != SCRIPT_LIST || args->listVal->numVals == 0 || args->listVal->vals[0].type != SCRIPT_LIST) {
		return;
	}
	CreateIntValue(s, 0);
	int i = 1;
	while (i < args->listVal->numVals) {
		args->listVal->vals[i].AddRef();
		if (!args->listVal->vals[0].listVal->PushBack(args->listVal->vals[i]))
			return;
		i++;
	}
}

void insert(ScriptValue &s, ScriptValue *args) {
	if (args->type != SCRIPT_LIST || args->listVal->numVals < 3 || args->listVal->vals[0].type != SCRIPT_LIST) {
		return;
	}
	CreateIntValue(s, 0);
	ScriptValue sv;
	CoerceIntNoRelease(args->listVal->vals[1], sv);
	int pos = sv.i32;
	if (sv.intVal < 0 || sv.intVal > args->listVal->vals[0].listVal->numVals)
		pos = args->listVal->vals[0].listVal->numVals;
	int i = 2;
	while (i < args->listVal->numVals) {
		args->listVal->vals[i].AddRef();
		if (!args->listVal->vals[0].listVal->Insert(args->listVal->vals[i], pos++))
			return;
		i++;
	}
}

void pop_range(ScriptValue &s, ScriptValue *args) {
	if (args->type != SCRIPT_LIST || args->listVal->numVals < 1 || args->listVal->vals[0].type != SCRIPT_LIST) {
		return;
	}
	ScriptValue sv;
	ListValue *list = args[0].listVal;
	ListValue *list2 = list->vals[0].listVal;
	int start=0, stop=list2->numVals-1;
	if (!list2->numVals) return;
	if (list->numVals >= 2) {
		CoerceIntNoRelease(list->vals[1], sv);
		start = sv.i32;
		if (start < 0 || start >= list2->numVals) start = list2->numVals-1;
		if (list->numVals >= 3) {
			CoerceIntNoRelease(list->vals[2], sv);
			stop = sv.i32;
			if (stop < 0 || stop >= list2->numVals) stop = list2->numVals-1;
		}
	}
	int dir = 1;
	if (start > stop) dir = -1;
	if (CreateListValue(s, 1+dir*(stop-start))) {
		int i;
		for (i = start; i != stop; i += dir) {
			s.listVal->PushBack(list2->vals[i]);
		}
		s.listVal->PushBack(list2->vals[i]);
	}
	if (start > stop) {
		dir = start;
		start = stop;
		stop = dir;
	}
	int i;
	if (!s.intVal) {
		for (i = start; i<=stop; i++) {
			list2->vals[i].Release();
		}
	}
	for (i = stop+1; i < list2->numVals; i++) {
		list2->vals[i-stop+start-1] = list2->vals[i];
	}
	list2->numVals += start-stop - 1;
}

void range(ScriptValue &s, ScriptValue *args) {
	if (args->type != SCRIPT_LIST || args->listVal->numVals < 1 || args->listVal->vals[0].type != SCRIPT_LIST) {
		return;
	}
	ScriptValue sv;
	ListValue *list = args[0].listVal;
	ListValue *list2 = list->vals[0].listVal;
	int start=0, stop=list2->numVals-1;
	if (!list2->numVals) return;
	if (list->numVals >= 2) {
		CoerceIntNoRelease(list->vals[1], sv);
		start = sv.i32;
		if (start < 0 || start >= list2->numVals) start = list2->numVals-1;
		if (list->numVals >= 3) {
			CoerceIntNoRelease(list->vals[2], sv);
			stop = sv.i32;
			if (stop < 0 || stop >= list2->numVals) stop = list2->numVals-1;
		}
	}
	int dir = 1;
	if (start > stop) dir = -1;
	if (CreateListValue(s, 1+dir*(stop-start))) {
		int i;
		for (i = start; i != stop; i += dir) {
			list2->vals[i].AddRef();
			s.listVal->PushBack(list2->vals[i]);
		}
		list2->vals[i].AddRef();
		s.listVal->PushBack(list2->vals[i]);
	}
	if (start > stop) {
		dir = start;
		start = stop;
		stop = dir;
	}
}

void pop(ScriptValue &s, ScriptValue *args) {
	if (args->type != SCRIPT_LIST || args->listVal->numVals < 1 || args->listVal->vals[0].type != SCRIPT_LIST) {
		return;
	}
	ScriptValue sv;
	ListValue *list = args[0].listVal;
	ListValue *list2 = list->vals[0].listVal;
	if (!list2->numVals) return;
	int elt = list2->numVals-1;
	if (list->numVals >= 2) {
		CoerceIntNoRelease(list->vals[1], sv);
		elt = sv.i32;
		if (elt < 0 || elt >= list2->numVals) elt = list2->numVals-1;
	}
	s = list2->vals[elt];
	for (int i = elt+1; i<list2->numVals; i++) {
		list2->vals[i-1] = list2->vals[i];
	}
	list2->numVals --;
}

int Compare(ScriptValue &sv, ScriptValue &sv2) {
	if (sv.type & (SCRIPT_INT | SCRIPT_DOUBLE)) {
		if (!(sv2.type & (SCRIPT_INT | SCRIPT_DOUBLE))) return -1;
		if (sv.type & SCRIPT_INT) {
			if (sv2.type & SCRIPT_INT) return (sv2.intVal < sv.intVal) - (sv2.intVal > sv.intVal);
			return (sv2.doubleVal < sv.intVal) - (sv2.doubleVal > sv.intVal);
		}
		else {
			if (sv2.type & SCRIPT_INT) return (sv2.intVal < sv.doubleVal) - (sv2.intVal > sv.doubleVal);
			return (sv2.doubleVal < sv.doubleVal) - (sv2.doubleVal > sv.doubleVal);
		}
	}
	if (sv2.type & (SCRIPT_INT | SCRIPT_DOUBLE)) {
		return 1;
	}
	if (sv.type == SCRIPT_STRING) {
		if (sv2.type == SCRIPT_STRING) {
			return scriptstricmp(sv, sv2);
		}
		return -1;
	}
	if (sv2.type == SCRIPT_STRING) return 1;
	if (sv.type == sv2.type) return 0;
	if (sv.type == SCRIPT_LIST) return -1;
	if (sv2.type == SCRIPT_LIST) return 1;
	if (sv.type == SCRIPT_DICT) return -1;
	if (sv2.type == SCRIPT_DICT) return 1;
	if (sv.type == SCRIPT_OBJECT) return -1;
	if (sv2.type == SCRIPT_OBJECT) return 1;
	return 0;
}

void dictlist(ScriptValue &s, ScriptValue *args) {
	if (args[0].type == SCRIPT_DICT && CreateListValue(s, args[0].dictVal->numEntries*2)) {
		DictValue *dict = args[0].dictVal;
		for (int i=0; i<dict->numEntries; i++) {
			dict->entries[i].key.AddRef();
			s.listVal->PushBack(dict->entries[i].key);
			dict->entries[i].val.AddRef();
			s.listVal->PushBack(dict->entries[i].val);
		}
	}
}

void dictkeys(ScriptValue &s, ScriptValue *args) {
	if (args[0].type == SCRIPT_DICT && CreateListValue(s, args[0].dictVal->numEntries)) {
		DictValue *dict = args[0].dictVal;
		for (int i=0; i<dict->numEntries; i++) {
			dict->entries[i].key.AddRef();
			s.listVal->PushBack(dict->entries[i].key);
		}
	}
}

void dictvalues(ScriptValue &s, ScriptValue *args) {
	if (args[0].type == SCRIPT_DICT && CreateListValue(s, args[0].dictVal->numEntries)) {
		DictValue *dict = args[0].dictVal;
		for (int i=0; i<dict->numEntries; i++) {
			dict->entries[i].val.AddRef();
			s.listVal->PushBack(dict->entries[i].val);
		}
	}
}

void MergeSort(ScriptValue *vals, int numVals) {
	ScriptValue sv;
	static const int skip[] = {1, 4, 10, 23, 57, 132, 301, 701};
	int w = sizeof(skip)/sizeof(int)-1;
	while (w && skip[w] >= numVals) w--;
	for (; w>= 0; w--) {
		int jump = skip[w];
		for (int offset = 0; offset<jump; offset ++) {
			for (int i = offset; i<numVals; i+=jump) {
				int j;
				sv = vals[i];
				for (j=i; j >= jump; j-=jump) {
					if (Compare(vals[j-jump], sv) == -1) break;
					vals[j] = vals[j-jump];
				}
				vals[j] = sv;
			}
		}
	}
}

void sort(ScriptValue &s, ScriptValue *args) {
	if (args->type == SCRIPT_LIST) {
		if (CreateListValue(s,1|args->listVal->numVals)) {
			if (args->listVal->numVals) {
				ListValue *list = args[0].listVal;
				s = args[0];
				list->AddRef();
				if (list->numVals > 1) {
					int *indices = (int*)malloc(sizeof(int) * list->numVals);
					if (indices) {
						MergeSort(list->vals, list->numVals);
						free(indices);
					}
				}
			}
		}
	}
}

void SortIndices(ScriptValue *indices, ScriptValue *vals, int numVals) {
	static const int skip[] = {1, 4, 10, 23, 57, 132, 301, 701};
	int w = sizeof(skip)/sizeof(int)-1;
	while (w && skip[w] >= numVals) w--;
	for (; w>= 0; w--) {
		int jump = skip[w];
		for (int offset = 0; offset<jump; offset ++) {
			for (int i = offset; i<numVals; i+=jump) {
				int j;
				int v = indices[i].i32;
				for (j=i; j >= jump; j-=jump) {
					if (Compare(vals[indices[j-jump].i32], vals[v]) == -1) break;
					indices[j].i32 = indices[j-jump].i32;
				}
				indices[j].i32 = v;
			}
		}
	}
}


void indexsort(ScriptValue &s, ScriptValue *args) {
	if (args->type == SCRIPT_LIST) {
		if (CreateListValue(s,1|args->listVal->numVals)) {
			if (args->listVal->numVals) {
				ListValue *list = args[0].listVal;
				for (int i=list->numVals-1; i>=0; i--) {
					CreateIntValue(s.listVal->vals[i], i);
				}
				s.listVal->numVals = list->numVals;
				SortIndices(s.listVal->vals, list->vals, list->numVals);
			}
		}
	}
}

#define MERGE_MIN 6

void MergeSortIndices(int *in[2], ScriptValue *vals, int numVals) {
	int end = numVals-1;
	int start = end-(end%MERGE_MIN);
	int j;
	while (start >= 0) {
		in[1][start] = start;
		for (int i=start+1; i<=end; i++) {
			int v = i;
			for ( j=i; j>start; j--) {
				if (Compare(vals[in[1][j-1]], vals[v]) <= 0) break;
				in[1][j] = in[1][j-1];
			}
			in[1][j] = v;
		}
		start-=MERGE_MIN;
		end = start + MERGE_MIN-1;
	}
	int step = MERGE_MIN;
	if (numVals < step) return;
	do {
		int *temp = in[1];
		in[1] = in[0];
		in[0] = temp;
		step *= 2;
		int *first = in[0];
		int *end = first+numVals-1;
		int *start = end-((numVals-1)%step);
		int *outp = in[1] + (start-first);
		while (start >= first) {
			int *min = start;
			int *mid = start + step/2;
			int *minStop = mid;
			int *write = outp;
			if (mid <= end) {
				while (1) {
					if (Compare(vals[*min], vals[*mid]) <= 0) {
						*write = *min;
						write++;
						min++;
						if (min == minStop) {
							min = mid;
							break;
						}
					}
					else {
						*write = *mid;
						write++;
						mid++;
						if (mid > end) {
							end = minStop-1;
							break;
						}
					}
				}
			}
			while (min <= end) {
				*write = *min;
				write++;
				min++;
			}
			outp -= step;
			start -= step;
			end = start + step-1;
		}
	}
	while (step < numVals);
}

void indexstablesort(ScriptValue &s, ScriptValue *args) {
	if (args->type == SCRIPT_LIST) {
		if (CreateListValue(s,1|args->listVal->numVals)) {
			if (args->listVal->numVals) {
				ListValue *list = args[0].listVal;
				int *indices[2];
				int *mem = indices[0] = (int*) malloc(2*sizeof(int) * args->listVal->numVals);
				if (!mem) {
					free(mem);
					s.Release();
					return;
				}
				indices[1] = mem + args->listVal->numVals;
				/*for (int i=list->numVals-1; i>=0; i--) {
					index[0][i] = i;
				}//*/
				MergeSortIndices(indices, list->vals, list->numVals);
				for (int i=list->numVals-1; i>=0; i--) {
					CreateIntValue(s.listVal->vals[i], indices[1][i]);
				}
				free(mem);
				s.listVal->numVals = list->numVals;
			}
		}
	}
}
