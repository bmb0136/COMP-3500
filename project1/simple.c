#include <math.h>
#include <stdio.h>

int main() {
  int nums[10] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
  double total = 0.0;
  for (size_t i = 0; i < 10; i++) {
    total += sqrt(nums[i]);
  }
  printf("%f", total / 10.0);
  return 0;
}
