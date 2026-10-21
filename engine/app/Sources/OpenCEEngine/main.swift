// The app's entry. Later phases open the Metal window here.
import AppKit
import OpenCEEngineCore
import OpenCEEngineKit

if CommandLine.arguments.contains("--version") {
    print("OpenCE Engine \(engineVersion())")
    exit(0)
}

do {
    try checkSnapshotLayout()
} catch {
    FileHandle.standardError.write(Data("OpenCE Engine: \(error)\n".utf8))
    exit(1)
}

let app = NSApplication.shared
app.setActivationPolicy(.regular)
app.run()
