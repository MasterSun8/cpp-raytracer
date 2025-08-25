struct HaltonState
{
    int baseIndex;

    HaltonState(int index) : baseIndex(index) {}

    float halton(int index, int base)
    {
        float result = 0;
        float f = 1;
        while (index > 0)
        {
            f /= base;
            result += f * (index % base);
            index /= base;
        }
        return result;
    }

    float dim(int d)
    {
        static const int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
        return halton(baseIndex, primes[d]);
    }
};