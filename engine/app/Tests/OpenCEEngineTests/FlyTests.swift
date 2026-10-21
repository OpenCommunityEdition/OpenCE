import AppKit
import Foundation
import Metal
import OpenCEEngineCore
import OpenCEEngineKit
import OpenCEEngineRender
@testable import OpenCEEngineWindow
import opence_ffiFFI
import simd
import XCTest

/// A frame of recorded input: the keys held and the mouse's counts.
struct RecordedFrame {
    var held: Set<UInt16> = []
    var dx: Double = 0
    var dy: Double = 0
}

/// Flies an empty world's free camera from the origin, one tick a frame,
/// and returns the camera after each frame.
func fly(_ frames: [RecordedFrame], controls: FlyControls = FlyControls()) throws -> [OCETransform] {
    let engine = Engine()
    let reader = SnapshotReader(engine: engine)
    engine.startEmptyWorld()
    try engine.placeCamera(placement: CameraPlacement(x: 0, y: 0, z: 0, yaw: 0, pitch: 0))
    return try frames.map { frame in
        engine.setInput(input: controls.input(held: frame.held, mouse: (frame.dx, frame.dy)))
        _ = try engine.step()
        return reader.acquire().camera.current
    }
}

final class FlyControlsTests: XCTestCase {
    typealias Key = FlyControls.Key

    func testTheKeysAndMouseAreTheInput() {
        let controls = FlyControls(sensitivity: 0.001)
        let still = controls.input(held: [], mouse: (0, 0))
        XCTAssertEqual(still, InputState(buttons: 0, forward: 0, right: 0, up: 0, yaw: 0, pitch: 0))
        let held = controls.input(held: [Key.w, Key.d, Key.space, Key.leftShift], mouse: (250, -100))
        XCTAssertEqual(held.forward, 127)
        XCTAssertEqual(held.right, 127)
        XCTAssertEqual(held.up, 127)
        XCTAssertEqual(held.buttons, inputButtons().sprint)
        // (right turns right, a negative yaw; up looks up)
        // (exactly the counts times the sensitivity, in Float)
        XCTAssertEqual(held.yaw, -(Float(250) * 0.001))
        XCTAssertEqual(held.pitch, Float(-100) * 0.001)
        let back = controls.input(held: [Key.s, Key.a, Key.rightControl, Key.rightShift], mouse: (0, 0))
        XCTAssertEqual([back.forward, back.right, back.up], [-127, -127, -127])
        XCTAssertEqual(back.buttons, inputButtons().sprint)
        // opposite keys cancel; other keys do nothing
        let both = controls.input(held: [Key.w, Key.s, Key.a, Key.d, 31], mouse: (.nan, .infinity))
        XCTAssertEqual(both, still)
        XCTAssertEqual(FlyControls(sensitivity: 0.001, invertPitch: true).input(held: [], mouse: (0, 10)).pitch,
                       -(Float(10) * 0.001))
    }

    func testTheButtonsAreTheSimulations() {
        let buttons = inputButtons()
        XCTAssertEqual([buttons.jump, buttons.crouch, buttons.fire, buttons.sprint], [1, 2, 4, 8])
    }

    func testRecordedKeysFlyTheCameraExactly() throws {
        // a second ahead, half a second up, then a second ahead sprinting
        let frames = Array(repeating: RecordedFrame(held: [Key.w]), count: 60)
            + Array(repeating: RecordedFrame(held: [Key.space]), count: 30)
            + Array(repeating: RecordedFrame(held: [Key.w, Key.leftShift]), count: 60)
        let cameras = try fly(frames)
        // (the simulation's own sums, in Float, tick by tick)
        let tick = Float(1) / 60
        var x: Float = 0
        var z: Float = 0
        for (index, frame) in frames.enumerated() {
            if frame.held.contains(Key.space) {
                z += 2 * tick
            } else {
                x += (frame.held.contains(Key.leftShift) ? 8 : 2) * tick
            }
            let position = cameras[index].position
            XCTAssertEqual(position.x, x, "frame \(index)")
            XCTAssertEqual(position.y, 0, "frame \(index)")
            XCTAssertEqual(position.z, z, "frame \(index)")
        }
        XCTAssertEqual(x, 10, accuracy: 1e-4)
        XCTAssertEqual(z, 1, accuracy: 1e-4)
    }

    func testRecordedMouseTurnsTheCameraExactly() throws {
        let controls = FlyControls(sensitivity: 0.001)
        // a quarter turn left over 10 frames, then 30 frames ahead; then a
        // look up past straight, which stops just short of it
        let left = Double.pi / 2 / 10 / 0.001
        let frames = Array(repeating: RecordedFrame(dx: -left), count: 10)
            + Array(repeating: RecordedFrame(held: [Key.w]), count: 30)
            + [RecordedFrame(dy: 4000)]
        let cameras = try fly(frames, controls: controls)
        let turned = cameras[39]
        let yaw = 2 * atan2(turned.rotation.z, turned.rotation.w)
        XCTAssertEqual(yaw, .pi / 2, accuracy: 1e-5)
        XCTAssertEqual(turned.position.x, 0, accuracy: 1e-5)
        XCTAssertEqual(turned.position.y, 1, accuracy: 1e-4)
        let up = cameras[40].rotation
        let looking = simd_quatf(ix: up.x, iy: up.y, iz: up.z, r: up.w).act([1, 0, 0])
        XCTAssertGreaterThan(looking.z, 0.9999)
        XCTAssertLessThan(looking.z, 1)
    }

    func testTheSameRecordingFliesTheSameWay() throws {
        let frames = (0..<240).map { (index: Int) -> RecordedFrame in
            let held: Set<UInt16> = index % 50 < 30 ? [Key.w, Key.d] : [Key.space, Key.leftShift]
            return RecordedFrame(held: held, dx: Double(index % 7) - 3.25, dy: Double(index % 5) - 1.5)
        }
        let first = try fly(frames)
        let second = try fly(frames)
        for (a, b) in zip(first, second) {
            XCTAssertEqual([a.position.x, a.position.y, a.position.z], [b.position.x, b.position.y, b.position.z])
            XCTAssertEqual([a.rotation.x, a.rotation.y, a.rotation.z, a.rotation.w],
                           [b.rotation.x, b.rotation.y, b.rotation.z, b.rotation.w])
        }
    }

    func testTheModifiersAreKeys() {
        XCTAssertEqual(ModifierKey.flag(FlyControls.Key.leftShift), .shift)
        XCTAssertEqual(ModifierKey.flag(FlyControls.Key.rightControl), .control)
        XCTAssertEqual(ModifierKey.flag(58), .option)
        XCTAssertNil(ModifierKey.flag(55), "Command is the menus'")
        XCTAssertNil(ModifierKey.flag(FlyControls.Key.w))
    }

    func testTheFrameTimesAreTheLastFrames() {
        var times = FrameTimes(capacity: 4)
        XCTAssertEqual(times.text, "–")
        for frame in [0.010, 0.020, .nan, -1, 0, 0.010, 0.020, 0.008, 0.008] {
            times.add(frame)
        }
        // (the last four: 0.010, 0.020, 0.008, 0.008)
        XCTAssertEqual(times.seconds.count, 4)
        XCTAssertEqual(times.meanMilliseconds, 11.5, accuracy: 1e-9)
        XCTAssertEqual(times.slowestMilliseconds, 20, accuracy: 1e-9)
        XCTAssertEqual(times.text, "11.50 ms  87 fps  slowest 20.00 ms")
    }
}

/// The modifier keys and F3 through the window, made off the screen.
@MainActor
final class FlyWindowTests: XCTestCase {
    func testShiftAndControlAreHeldAndF3ShowsTheFrameTimes() throws {
        let controller = EngineWindowController(contentSize: CGSize(width: 320, height: 200), pointer: FakePointer(),
                                                rawMouse: FakeMouse())
        defer { controller.window.close() }
        NotificationCenter.default.post(name: NSWindow.didBecomeKeyNotification, object: controller.window)
        func flags(_ code: UInt16, _ flags: CGEventFlags) throws {
            let event = try XCTUnwrap(CGEvent(keyboardEventSource: nil, virtualKey: code, keyDown: true))
            event.type = .flagsChanged
            event.flags = flags
            controller.window.engineView.flagsChanged(with: try XCTUnwrap(NSEvent(cgEvent: event)))
        }
        try flags(FlyControls.Key.leftShift, .maskShift)
        try flags(FlyControls.Key.leftControl, [.maskShift, .maskControl])
        XCTAssertEqual(controller.keys.codes, [FlyControls.Key.leftShift, FlyControls.Key.leftControl])
        try flags(FlyControls.Key.leftShift, .maskControl)
        XCTAssertEqual(controller.keys.codes, [FlyControls.Key.leftControl])

        XCTAssertFalse(controller.showsFrameTimes)
        let f3 = try XCTUnwrap(NSEvent.keyEvent(with: .keyDown, location: .zero, modifierFlags: [], timestamp: 0,
                                                windowNumber: 0, context: nil, characters: "",
                                                charactersIgnoringModifiers: "", isARepeat: false,
                                                keyCode: KeyCode.frameTimes))
        controller.window.engineView.keyDown(with: f3)
        XCTAssertTrue(controller.showsFrameTimes)
        XCTAssertFalse(controller.keys.isHeld(KeyCode.frameTimes))
        controller.window.engineView.overlayText = "8.33 ms"
        XCTAssertEqual(controller.window.engineView.overlayText, "8.33 ms")
        controller.window.engineView.overlayText = nil
        XCTAssertNil(controller.window.engineView.overlayText)
    }
}

final class MapLibraryTests: XCTestCase {
    func testTheLevelsAreListedWithoutTheResourceMaps() throws {
        let folder = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        try FileManager.default.createDirectory(at: folder.appendingPathComponent("folder.map"),
                                                withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: folder) }
        for name in ["bloodgulch.map", "a30.MAP", "Bitmaps.map", "sounds.map", "loc.map", "notes.txt", "map10.map",
                     "map2.map"] {
            try Data().write(to: folder.appendingPathComponent(name))
        }
        XCTAssertEqual(MapLibrary.maps(in: folder).map(\.lastPathComponent),
                       ["a30.MAP", "bloodgulch.map", "map2.map", "map10.map"])
        XCTAssertEqual(MapLibrary.maps(in: folder.appendingPathComponent("none")), [])
        let root = URL(fileURLWithPath: "/data")
        XCTAssertEqual(MapLibrary.folders(named: nil, root: root).map(\.path),
                       ["/data/assets/maps", "/data/assets/custom_maps"])
        XCTAssertEqual(MapLibrary.folders(named: "/elsewhere", root: root).map(\.path), ["/elsewhere"])
    }

    /// The map the picker lists, or the command line names, loads with its
    /// start view (skipped without assets/maps).
    func testAListedMapLoads() throws {
        let listed = MapLibrary.maps(in: MapLibrary.folders(named: nil, root: StockLevel.root))
        guard let map = listed.first(where: { $0.lastPathComponent == "bloodgulch.map" }) else {
            throw XCTSkip("no assets/maps/bloodgulch.map")
        }
        guard let device = MTLCreateSystemDefaultDevice() else { throw XCTSkip("no Metal device") }
        let cache = StockLevel.root.appendingPathComponent("build/engine/cache")
        try FileManager.default.createDirectory(at: cache, withIntermediateDirectories: true)
        let level = try LoadedLevel(map: map.path, cacheFolder: cache.path, device: device,
                                    settings: VideoSettings(antiAliasing: .native))
        XCTAssertEqual(level.map, map.path)
        XCTAssertGreaterThan(level.geometry.draws().count, 0)
        XCTAssertTrue([level.start.x, level.start.y, level.start.z].allSatisfy(\.isFinite))
        let engine = Engine()
        engine.startEmptyWorld()
        XCTAssertNoThrow(try engine.placeCamera(placement: level.start))
        XCTAssertThrowsError(try LoadedLevel(map: "/no/such.map", cacheFolder: cache.path, device: device,
                                             settings: VideoSettings()))
    }
}
