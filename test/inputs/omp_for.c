

int main() {

 int a,n;
 #pragma omp parallel for private(a)
 for(int i=0;i<10;i++) {
   a += i;
 }

 #pragma omp parallel
 {
  #pragma omp for firstprivate(a) \
       nowait
  for(a=0;a<n;a++) {
   	#pragma omp barrier
  }
 }

}
