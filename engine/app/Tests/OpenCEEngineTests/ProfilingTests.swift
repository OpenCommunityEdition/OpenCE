import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import XCTest

/// The app's profiling marks: scopes opened and closed around their work,
/// and nothing done while frames are not measured.
final class ProfilingTests: XCTestCase {
    final class FakeTimer: ScopeTimer {
        var measuring: Bool
        var calls: [String] = []

        init(measuring: Bool) {
            self.measuring = measuring
        }

        func profileBegin(name: String) -> Bool {
            calls.append("begin \(name)")
            return measuring
        }

        func profileEnd() {
            calls.append("end")
        }
    }

    struct Failure: Error {}

    func testAScopeClosesAroundItsWork() {
        let timer = FakeTimer(measuring: true)
        let value = profiled("outer", timer) { () -> Int in
            timer.calls.append("work")
            return profiled("inner", timer) { 7 }
        }
        XCTAssertEqual(value, 7)
        XCTAssertEqual(timer.calls, ["begin outer", "work", "begin inner", "end", "end"])
    }

    func testAScopeClosesWhenItsWorkThrows() {
        let timer = FakeTimer(measuring: true)
        XCTAssertThrowsError(try profiled("throws", timer) { throw Failure() })
        XCTAssertEqual(timer.calls, ["begin throws", "end"])
    }

    func testNothingIsClosedWhenNothingIsMeasured() {
        let timer = FakeTimer(measuring: false)
        XCTAssertEqual(profiled("off", timer) { 1 }, 1)
        XCTAssertEqual(profiled("none", nil) { 2 }, 2)
        XCTAssertEqual(timer.calls, ["begin off"])
    }

    func testTheEnginesScopesAreTakenWhileFramesAreMeasured() throws {
        let engine = Engine()
        engine.startEmptyWorld()
        XCTAssertFalse(engine.profileBegin(name: "render"))
        XCTAssertTrue(engine.runCommand(line: "stat frame").ok)
        _ = try engine.advance(frameSeconds: 1.0 / 30.0)
        profiled("render", engine) {}
        _ = try engine.advance(frameSeconds: 1.0 / 30.0)
        let shown = engine.runCommand(line: "stat frame")
        XCTAssertTrue(shown.text.contains("render"), shown.text)
        XCTAssertTrue(engine.runCommand(line: "stat frame off").ok)
    }
}
