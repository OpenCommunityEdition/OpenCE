import Foundation
import OpenCEEngineCore
import OpenCEEngineKit
import opence_ffiFFI
import simd
import XCTest

final class PlayControlsTests: XCTestCase {
    func testTheBoundKeysAndMouseAreTheInput() {
        let controls = PlayControls(sensitivity: 0.001)
        let buttons = inputButtons()
        XCTAssertEqual(controls.input(held: [], mouse: (0, 0)),
                       InputState(buttons: 0, forward: 0, right: 0, up: 0, yaw: 0, pitch: 0))
        // W, D, Space and the left Control: forward, right, jump, crouch
        let held = controls.input(held: [13, 2, 49, 59], mouse: (250, -100))
        XCTAssertEqual([held.forward, held.right, held.up], [127, 127, 0])
        XCTAssertEqual(held.buttons, buttons.jump | buttons.crouch)
        XCTAssertEqual(held.yaw, -(Float(250) * 0.001))
        XCTAssertEqual(held.pitch, Float(-100) * 0.001)
        let back = controls.input(held: [1, 0, 62], mouse: (0, 0))
        XCTAssertEqual([back.forward, back.right], [-127, -127])
        XCTAssertEqual(back.buttons, buttons.crouch)
        // (Shift is no button of the player's; opposite keys cancel)
        XCTAssertEqual(controls.input(held: [56, 13, 1], mouse: (.nan, 0)), controls.input(held: [], mouse: (0, 0)))
    }

    func testRebindingMovesTheActions() throws {
        // (E jumps, the arrows move; the rest stay stock)
        let json = Data(#"{"jump": [14], "forward": [126], "back": [125]}"#.utf8)
        let bindings = try KeyBindings.read(json)
        let controls = PlayControls(bindings: bindings)
        XCTAssertEqual(controls.input(held: [14], mouse: (0, 0)).buttons, inputButtons().jump)
        XCTAssertEqual(controls.input(held: [49], mouse: (0, 0)).buttons, 0)
        XCTAssertEqual(controls.input(held: [126], mouse: (0, 0)).forward, 127)
        XCTAssertEqual(controls.input(held: [13], mouse: (0, 0)).forward, 0)
        XCTAssertEqual(controls.input(held: [59], mouse: (0, 0)).buttons, inputButtons().crouch)
        XCTAssertEqual(try KeyBindings.read(bindings.written()), bindings)
        XCTAssertThrowsError(try KeyBindings.read(Data(#"{"fly": [1]}"#.utf8))) { error in
            XCTAssertEqual(error as? BindingError, .unknownAction("fly"))
        }
        XCTAssertThrowsError(try KeyBindings.read(Data("not json".utf8)))
    }

    func testTheViewTogglesOncePerPress() {
        let controls = PlayControls()
        XCTAssertTrue(controls.togglesView(held: [9], earlier: []))
        XCTAssertFalse(controls.togglesView(held: [9], earlier: [9]))
        XCTAssertFalse(controls.togglesView(held: [], earlier: [9]))
    }

    /// The player on bloodgulch: W runs it the way it faces, the
    /// first-person camera staying at its eyes, and V's view sees it from
    /// behind (skipped without assets/maps).
    func testTheCameraFollowsTheBiped() throws {
        let map = StockLevel.root.appendingPathComponent("assets/maps/bloodgulch.map")
        guard FileManager.default.fileExists(atPath: map.path) else {
            throw XCTSkip("no assets/maps/bloodgulch.map")
        }
        let cache = StockLevel.root.appendingPathComponent("build/engine/cache")
        try FileManager.default.createDirectory(at: cache, withIntermediateDirectories: true)
        let geometry = try LevelGeometry.load(map: map.path, cacheFolder: cache.path, bsp: 0)
        let engine = Engine()
        let reader = SnapshotReader(engine: engine)
        _ = try engine.startLevel(geometry: geometry, difficulty: 1)
        try engine.spawnPlayer(start: 0)
        XCTAssertTrue(engine.playing())
        let controls = PlayControls()
        engine.setInput(input: controls.input(held: [], mouse: (0, 0)))
        for _ in 0..<30 { _ = try engine.step() }
        let start = reader.acquire().camera.current
        engine.setInput(input: controls.input(held: [13], mouse: (0, 0)))
        for _ in 0..<60 { _ = try engine.step() }
        let ran = reader.acquire().camera.current
        // (along the way it faces, at about its running speed)
        let facing = simd_act(start.rotation, SIMD3<Float>(1, 0, 0))
        let moved = SIMD3<Float>(ran.position.x - start.position.x, ran.position.y - start.position.y, 0)
        XCTAssertGreaterThan(simd_dot(moved, simd_normalize(SIMD3(facing.x, facing.y, 0))), 1.5)
        let drawn = reader.acquire().objects.count
        XCTAssertTrue(engine.setThirdPerson(thirdPerson: true))
        let behind = reader.acquire()
        XCTAssertEqual(behind.objects.count, drawn + 1)
        let back = SIMD3<Float>(ran.position.x - behind.camera.current.position.x,
                                ran.position.y - behind.camera.current.position.y, 0)
        XCTAssertGreaterThan(simd_dot(back, simd_normalize(SIMD3(facing.x, facing.y, 0))), 0.5)
    }
}

private func simd_act(_ rotation: OCEQuat, _ vector: SIMD3<Float>) -> SIMD3<Float> {
    simd_quatf(ix: rotation.x, iy: rotation.y, iz: rotation.z, r: rotation.w).act(vector)
}
