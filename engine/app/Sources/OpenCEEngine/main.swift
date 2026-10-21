// The app's entry. Later phases open the Metal window here.
import AppKit
import OpenCEEngineCore

if CommandLine.arguments.contains("--version") {
    print("OpenCE Engine \(engineVersion())")
    exit(0)
}

let app = NSApplication.shared
app.setActivationPolicy(.regular)
app.run()
