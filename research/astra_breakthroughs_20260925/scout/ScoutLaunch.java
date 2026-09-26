import ghidra.GhidraApplicationLayout;
import ghidra.GhidraLaunchable;
import ghidra.framework.Application;
import ghidra.framework.HeadlessGhidraApplicationConfiguration;
import ghidra.base.project.GhidraProject;
import ghidra.program.model.listing.Program;
import ghidra.app.script.GhidraState;
import ghidra.util.task.TaskMonitor;
import java.io.PrintWriter;

public class ScoutLaunch implements GhidraLaunchable {
    public void launch(GhidraApplicationLayout layout, String[] args) throws Exception {
        Application.initializeApplication(layout, new HeadlessGhidraApplicationConfiguration());
        GhidraProject project = GhidraProject.openProject(args[0], args[1], false);
        Object consumer = new Object();
        Program program = null;
        try {
            program = (Program)project.getRootFolder().getFile("halo_cache_symbols.exe")
                .getReadOnlyDomainObject(consumer, -1, TaskMonitor.DUMMY);
            ScoutPrimary script = new ScoutPrimary();
            script.setScriptArgs(new String[] {args[2]});
            script.set(new GhidraState(null, project.getProject(), program, null, null, null),
                TaskMonitor.DUMMY, new PrintWriter(System.out, true));
            script.run();
        } finally {
            if (program != null) program.release(consumer);
            project.close();
        }
    }
}
