// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* fails if doit makes a heap call */
#include "tools.h"

static volatile long calls; /* heap calls, seen through the linker's --wrap */
void *__real_malloc(size_t n);
void *__real_calloc(size_t a, size_t b);
void *__real_realloc(void *p, size_t n);
void *__real_memalign(size_t a, size_t n);
int __real_posix_memalign(void **p, size_t a, size_t n);
void *__real_aligned_alloc(size_t a, size_t n);
void __real_free(void *p);
void *__wrap_malloc(size_t n){ calls++; return __real_malloc(n); }
void *__wrap_calloc(size_t a, size_t b){ calls++; return __real_calloc(a, b); }
void *__wrap_realloc(void *p, size_t n){ calls++; return __real_realloc(p, n); }
void *__wrap_memalign(size_t a, size_t n){ calls++; return __real_memalign(a, n); }
int __wrap_posix_memalign(void **p, size_t a, size_t n){ calls++; return __real_posix_memalign(p, a, n); }
void *__wrap_aligned_alloc(size_t a, size_t n){ calls++; return __real_aligned_alloc(a, n); }
void __wrap_free(void *p){ calls++; __real_free(p); }

int main(int argc, char **argv){
	main_init(&argc, &argv);
	static int sizes[4096];
	int count = argc>1 ? read_sizes(argv[1], sizes, 4096) : 0, checked = 0, bad = 0;
	long planned = 0;
	if (count==0){
		fprintf(stderr, "usage: alloc-check <sizes file>\n");
		return 2;
	}
	for (int s = 0; s<count; s++){
		bench_problem *p = rof_problem(sizes[s]);
		if (!p){
			continue;
		}
		long c0 = calls;
		setup(p);
		rof_input(p);
		long c1 = calls;
		doit(100, p);
		long c2 = calls;
		done(p);
		problem_destroy(p);
		planned += c1-c0;
		checked++;
		if (c2!=c1){
			bad++;
			printf("%6d: %ld heap calls in 100 transforms\n", sizes[s], c2-c1);
		}
	}
	long c0 = calls;
	void *volatile control = malloc(16);
	long once = calls-c0;
	free(control);
	printf("alloc-check: %d sizes; setup made %ld heap calls; one malloc counted as %ld\n", checked, planned, once);
	if (checked==0){
		printf("alloc-check: skipped: no size runs on this CPU\n");
		return 77;
	}
	if (planned==0 && once==1){
		printf("alloc-check: skipped: the library's heap calls can't be seen (a shared library?)\n");
		return 77;
	}
	if (bad || checked==0 || once!=1){
		printf("alloc-check: FAILED (%d sizes with heap calls in doit)\n", bad);
		return 1;
	}
	printf("alloc-check: no heap calls in doit\n");
	return 0;
}
