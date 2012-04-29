extern void printf(const char*, ...);

int main() {

	#pragma omp parallel
	{
		printf("hell world\n");
	}
}