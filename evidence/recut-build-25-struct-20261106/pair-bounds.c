#define main exhaustive_main
#include "../../tests/host/drum909_tanh_pair_test.c"
#undef main
int main(void)
{
    for (unsigned i = 0; i < X0X_TANH_N; ++i) {
        float a, b;
        d9_tanh_pair(i, &a, &b);
        if (bits(a) != bits(x0x_tanh_value(i)) || bits(b) != bits(x0x_tanh_value(i + 1u))) return 1;
    }
    puts("PASS: every tanh pair under AddressSanitizer and UndefinedBehaviorSanitizer");
    return 0;
}
