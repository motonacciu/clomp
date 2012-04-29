
int main() {
	
	int a = 666;
	
	#pragma omp parallel
	{
		int b;
		printf("hell world #%d/%d\n", a, b);
		#pragma omp barrier
	}
}