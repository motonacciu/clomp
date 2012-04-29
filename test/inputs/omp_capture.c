extern void printf(const char*, ...);

int main() {
	
	int a = 666;
	
	#pragma omp parallel
	{
		int b;
		printf("hell world #%d/%d\n", a, b);
		
		#pragma omp for
		for(int i=0; i<100; ++i) {
			printf("%d", i+a);
		}
	}
}