/* Separate translation unit prevents inlining away the aggregate-return ABI. */
union probe_value { long bits; float real; };
union probe_value mac_probe_return(long value)
{
    union probe_value result;
    result.bits = value;
    return result;
}
