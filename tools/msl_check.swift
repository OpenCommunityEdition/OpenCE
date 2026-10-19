// Compiles Metal Shading Language files as OpenCE.app's renderer compiles
// the translated NV2A shaders (MetalRenderer.swift, CREATE_LIBRARY): with
// the same options, checking that each library has the entry point its
// file is named for (vertex_main in *.vertex.metal, fragment_main in
// *.fragment.metal).
//
//     xcrun swift tools/msl_check.swift a.vertex.metal b.fragment.metal ...
//
// Prints "<path>: ok" or "<path>: <the compiler's messages>" for each, and
// exits 1 when one does not compile (tools/test_nv2a_msl.py).

import Foundation
import Metal

guard let device = MTLCreateSystemDefaultDevice() else {
    print("no Metal device")
    exit(2)
}
// (MetalRenderer.libraryOptions)
let options = MTLCompileOptions()
options.preserveInvariance = true
options.mathMode = .safe
options.languageVersion = .version3_2
var failed = false
for path in CommandLine.arguments.dropFirst() {
    guard let source = try? String(contentsOfFile: path, encoding: .utf8) else {
        print("\(path): cannot be read")
        failed = true
        continue
    }
    let entry = path.hasSuffix(".vertex.metal") ? "vertex_main" : "fragment_main"
    do {
        let library = try device.makeLibrary(source: source, options: options)
        if library.makeFunction(name: entry) == nil {
            print("\(path): no \(entry)")
            failed = true
        } else {
            print("\(path): ok")
        }
    } catch {
        print("\(path): \(String(describing: error).replacingOccurrences(of: "\n", with: "\n    "))")
        failed = true
    }
}
exit(failed ? 1 : 0)
