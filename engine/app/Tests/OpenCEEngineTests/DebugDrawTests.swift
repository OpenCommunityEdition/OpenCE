import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
import simd
import XCTest

/// The debug drawing: the snapshot's shapes, their vertices and labels,
/// and the lines in a frame.
final class DebugDrawTests: XCTestCase {
    let camera = ViewCamera(position: [0, 0, 0], yaw: 0, pitch: 0, verticalFov: 70 * .pi / 180)

    func testAColourIsUnpackedFromTheTop() {
        XCTAssertEqual(debugColor(0xff00_80ff), SIMD4(1, 0, Float(0x80) / 255, 1))
    }

    func testEachEndCarriesItsColoursBits() {
        let vertices = DebugPass.vertices([DebugSegment(from: [1, 2, 3], to: [4, 5, 6], color: 0x1234_5678)])
        XCTAssertEqual(vertices.count, 2)
        XCTAssertEqual(SIMD3(vertices[1].x, vertices[1].y, vertices[1].z), [4, 5, 6])
        XCTAssertEqual(vertices[0].w.bitPattern, 0x1234_5678)
        XCTAssertEqual(vertices[1].w.bitPattern, 0x1234_5678)
    }

    func testALabelAheadIsCentredAndOneBehindIsDropped() {
        let viewProjection = camera.viewProjection(aspect: 2)
        let marks = [
            DebugMark(at: [10, 0, 0], color: 0xffff_ffff, text: "ahead"),
            DebugMark(at: [-10, 0, 0], color: 0xffff_ffff, text: "behind"),
            DebugMark(at: [10, 100, 0], color: 0xffff_ffff, text: "aside"),
            DebugMark(at: [10, 0, 0], color: 0xffff_ffff, text: ""),
        ]
        let items = DebugPass.labels(
            marks, viewProjection: viewProjection, width: 800, height: 400,
            cellWidth: 10, cellHeight: 20)
        XCTAssertEqual(items.count, 2)
        guard case let .text(text, x, y, color) = items[1] else { return XCTFail("no text") }
        XCTAssertEqual(text, "ahead")
        XCTAssertEqual(x, 400 - 25)
        XCTAssertEqual(y, 200 - 10)
        XCTAssertEqual(color, SIMD4(1, 1, 1, 1))
    }

    func testTheShellsShapesReachTheSnapshot() throws {
        let engine = Engine()
        let reader = SnapshotReader(engine: engine)
        engine.startEmptyWorld()
        XCTAssertTrue(engine.runCommand(line: "debug_line 0 0 0 1 2 3 5").ok)
        XCTAssertTrue(engine.runCommand(line: "debug_text 4 5 6 hello there").ok)
        _ = try engine.step()
        let frame = reader.acquire()
        let segments = frame.debugSegments()
        XCTAssertEqual(segments.count, 1)
        XCTAssertEqual(segments.first?.to, [1, 2, 3])
        XCTAssertEqual(
            frame.debugMarks(),
            [
                DebugMark(
                    at: [4, 5, 6], color: frame.debugMarks().first?.color ?? 0,
                    text: "hello there")
            ])
        XCTAssertTrue(engine.runCommand(line: "debug_clear").ok)
        _ = try engine.step()
        XCTAssertTrue(reader.acquire().debugSegments().isEmpty)
        XCTAssertTrue(reader.acquire().debugMarks().isEmpty)
    }

    // (nearer than anything at the start view: the depth test hides what is behind the level)
    func testALineAheadIsDrawnOverBloodGulch() throws {
        let level = try StockLevel(map: "bloodgulch")
        let renderer = try level.renderer(VideoSettings(antiAliasing: .native, renderScale: 1))
        let without = try renderer.capture(width: 640, height: 360, camera: level.camera, queue: level.queue)
        XCTAssertEqual(renderer.debug.lastDrawn, 0)
        let ahead = level.camera.position + level.camera.rotation.act([0.25, 0, 0])
        let side = level.camera.rotation.act([0, 0.1, 0])
        let line = DebugSegment(from: ahead - side, to: ahead + side, color: 0xff00_ffff)
        let with = try renderer.capture(
            width: 640, height: 360, camera: level.camera, debugLines: [line],
            queue: level.queue)
        XCTAssertEqual(renderer.debug.lastDrawn, 1)
        let difference = try XCTUnwrap(with.difference(from: without))
        XCTAssertGreaterThan(difference.share, 0, "the line changed nothing in the frame")
        // (magenta across the middle)
        let column = (176...184).map { with.rgb(x: 320, y: $0) }
        XCTAssertTrue(column.contains { Int($0.x) > Int($0.y) + 100 && Int($0.z) > Int($0.y) + 100 })
    }
}
