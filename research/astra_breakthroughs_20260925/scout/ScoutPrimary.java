// Read-only headless scout export; any temporary function creation is rolled back.
// @category Halo.Research
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import java.io.PrintWriter;
import java.io.File;

public class ScoutPrimary extends GhidraScript {
    public void run() throws Exception {
        File out = new File(getScriptArgs()[0]);
        Address start = toAddr(0x008f80b0L);
        Address end = toAddr(0x008f930aL);
        int transaction = currentProgram.startTransaction("temporary scout analysis");
        DecompInterface decompiler = new DecompInterface();
        try {
            new DisassembleCommand(start, new AddressSet(start, end), true)
                .applyTo(currentProgram, monitor);
            Function fn = getFunctionAt(start);
            if (fn == null) fn = createFunction(start, "scout_temporary_analysis");
            try (PrintWriter w = new PrintWriter(new File(out, "ghidra_scout.asm"))) {
                w.println("Program: " + currentProgram.getExecutablePath());
                w.println("SHA256: " + currentProgram.getExecutableSHA256());
                w.println("Start: " + start + "; end: " + end);
                if (fn != null) w.println("Function body: " + fn.getBody());
                for (Instruction ins = getInstructionAt(start); ins != null && ins.getAddress().compareTo(end) <= 0; ins = ins.getNext()) {
                    w.println(ins.getAddress() + "  " + ins.toString());
                }
            }
            decompiler.openProgram(currentProgram);
            if (fn != null && fn.getBody().getMinAddress().equals(start)
                && fn.getBody().getMaxAddress().compareTo(end) <= 0) {
                DecompileResults result = decompiler.decompileFunction(fn, 30, monitor);
                try (PrintWriter w = new PrintWriter(new File(out, "ghidra_scout.c"))) {
                    if (result.decompileCompleted()) w.print(result.getDecompiledFunction().getC());
                    else w.println("DECOMPILE FAILED: " + result.getErrorMessage());
                }
            } else throw new IllegalStateException("Function missing or outside bounded scout range");
            println("Scout primary export complete; no project changes saved.");
        } finally {
            decompiler.dispose();
            currentProgram.endTransaction(transaction, false);
        }
    }
}
