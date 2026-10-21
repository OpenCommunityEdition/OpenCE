import OpenCEEngineCore
import OpenCEEngineKit
import XCTest

final class ContractTests: XCTestCase {
    func testTheHeaderMatchesTheLibrarysLayout() throws {
        XCTAssertNoThrow(try checkSnapshotLayout())
    }

    func testAnEmptyWorldIsReadInPlace() throws {
        let engine = Engine()
        let reader = SnapshotReader(engine: engine)
        XCTAssertEqual(reader.acquire().tick, 0)
        engine.startEmptyWorld()
        XCTAssertEqual(try engine.step(), 1)
        XCTAssertEqual(try engine.step(), 2)
        let frame = reader.acquire()
        XCTAssertEqual(frame.tick, 2)
        XCTAssertEqual(frame.objects.count, 0)
        XCTAssertEqual(frame.nodes.count, 0)
        XCTAssertEqual(frame.camera.current.scale, 1)
        XCTAssertGreaterThan(frame.camera.vertical_fov_radians, 1)
    }

    func testStepWithoutAWorldThrows() {
        XCTAssertThrowsError(try Engine().step()) { error in
            XCTAssertEqual(error as? EngineError, .NoWorld)
        }
    }
}
