#include <stdio.h>
int main() {

 #pragma omp parallel
 {

	 ;

 }

 int a,b;
 #pragma omp parallel private(a) default(shared) private(b)
 { }

 #pragma omp master
 printf("hello world\n");

 for(int i=0;i<10;i++) 
	#pragma omp single
	printf("HELLO again!");
}

