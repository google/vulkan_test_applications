#include "inputs.h"

kernel void __attribute__((reqd_work_group_size(LOCAL_X_SIZE, 1, 1)))
adder(global uint* A, global uint* B, global uint* C) {
  const size_t index = get_global_id(0);
  C[index] = A[index] + B[index];
}
