#include <idc.idc>

static main()
{
    auto out, ea;
    auto_wait();
    out = fopen(ARGV[1], "w");
    if (out == 0) qexit(2);
    fprintf(out, "IDA primary scout disassembly; no source-symbol claims\n");
    for (ea = 0x8f80b0; ea <= 0x8f930a; ea = next_head(ea, 0x8f930b)) {
        create_insn(ea);
        fprintf(out, "%08x  %s\n", ea, generate_disasm_line(ea, 0));
    }
    fclose(out);
    qexit(0);
}
