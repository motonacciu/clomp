#include <stdio.h>
#include <assert.h>

#define A_SIZE 64

int main() {
	
	int a[A_SIZE];
	int b[A_SIZE];
	int c[A_SIZE];
	for(unsigned i = 0; i<A_SIZE; ++i) {
		a[i] = i;
		b[i] = A_SIZE-i;
		c[i] = 0;
	}
	
	#pragma omp parallel
	{
		printf("Hello from thread %d!\n", omp_get_thread_num());
		#pragma omp for
		for(unsigned i=0; i<A_SIZE; ++i) {
			c[i] = a[i] + b[i];
		}
	}
	
	for(unsigned i = 0; i<A_SIZE; ++i) {
		assert(c[i] == A_SIZE);
	}
}
