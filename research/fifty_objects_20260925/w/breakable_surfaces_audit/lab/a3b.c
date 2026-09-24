static const long src_states[9] = {1,2,3,4,5,6,7,8,-1};
static const long dst_states[9] = {9,10,11,12,13,14,15,16,-1};
static const long op_states[9] = {17,18,19,20,21,22,23,24,-1};
long f(long j) { return src_states[j] + dst_states[j] + op_states[j]; }
