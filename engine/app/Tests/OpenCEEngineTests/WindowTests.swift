import AppKit
@testable import OpenCEEngineWindow
import XCTest

/// The pointer, as a test sees it.
@MainActor
final class FakePointer: PointerControl {
    var captured = false
    var changes = 0
    func setCaptured(_ captured: Bool, in window: NSWindow) {
        self.captured = captured
        changes += 1
    }
}

/// A raw mouse a test moves.
@MainActor
final class FakeMouse: RawMouse {
    var available = true
    var onMotion: ((Double, Double) -> Void)?
    func move(_ dx: Double, _ dy: Double) { onMotion?(dx, dy) }
}

final class WindowModelTests: XCTestCase {
    func testPixelsAreThePointsAtTheScale() {
        XCTAssertTrue(Viewport.pixelSize(points: CGSize(width: 960, height: 540), scale: 2) == (1920, 1080))
        XCTAssertTrue(Viewport.pixelSize(points: CGSize(width: 1720, height: 720), scale: 2) == (3440, 1440))
        XCTAssertTrue(Viewport.pixelSize(points: CGSize(width: 3840, height: 2160), scale: 1) == (3840, 2160))
        // (a fraction of a pixel rounds, as AppKit's backing does)
        XCTAssertTrue(Viewport.pixelSize(points: CGSize(width: 100.25, height: 50.75), scale: 2) == (201, 102))
        XCTAssertTrue(Viewport.pixelSize(points: .zero, scale: 2) == (0, 0))
    }

    func testTheMouseIsCapturedWithTheFocusUntilEscape() {
        var capture = MouseCapture()
        XCTAssertFalse(capture.captured)
        XCTAssertFalse(capture.handle(.click), "a click before the focus")
        XCTAssertTrue(capture.handle(.focused))
        XCTAssertTrue(capture.captured)
        XCTAssertTrue(capture.handle(.escape))
        XCTAssertFalse(capture.captured)
        XCTAssertFalse(capture.handle(.escape))
        XCTAssertTrue(capture.handle(.click))
        XCTAssertTrue(capture.captured)
        XCTAssertTrue(capture.handle(.unfocused))
        XCTAssertFalse(capture.captured)
        XCTAssertFalse(capture.handle(.click))
    }

    func testRawMotionIsSummedExactlyAndTaken() {
        var motion = MouseMotion()
        for _ in 0..<3 {
            motion.add(dx: 1.5, dy: -0.25)
        }
        motion.add(dx: .nan, dy: 1)
        motion.add(dx: 1, dy: .infinity)
        let taken = motion.take()
        XCTAssertEqual(taken.dx, 4.5)
        XCTAssertEqual(taken.dy, -0.75)
        XCTAssertTrue(motion.take() == (0, 0))
    }

    func testTheDisplayLinkRunsAtTheDisplaysRate() {
        for rate in [60, 120, 144] {
            let range = FrameRate.range(maximumFramesPerSecond: rate)
            XCTAssertEqual(range.minimum, Float(rate))
            XCTAssertEqual(range.maximum, Float(rate))
            XCTAssertEqual(range.preferred, Float(rate))
        }
        XCTAssertEqual(FrameRate.range(maximumFramesPerSecond: 0).preferred, 1)
    }
}

/// The window made off the screen, with a fake pointer and mouse.
@MainActor
final class EngineWindowTests: XCTestCase {
    var pointer: FakePointer!
    var mouse: FakeMouse!
    var controller: EngineWindowController!

    override func setUp() async throws {
        pointer = FakePointer()
        mouse = FakeMouse()
        controller = EngineWindowController(contentSize: CGSize(width: 640, height: 360), pointer: pointer,
                                            rawMouse: mouse)
    }

    override func tearDown() async throws {
        controller.window.close()
        controller = nil
    }

    func key(_ code: UInt16, down: Bool) {
        let event = NSEvent.keyEvent(with: down ? .keyDown : .keyUp, location: .zero, modifierFlags: [],
                                     timestamp: 0, windowNumber: controller.window.windowNumber, context: nil,
                                     characters: "", charactersIgnoringModifiers: "", isARepeat: false,
                                     keyCode: code)
        guard let event else { return XCTFail("no key event") }
        if down { controller.window.engineView.keyDown(with: event) } else { controller.window.engineView.keyUp(with: event) }
    }

    func mouseEvent(_ type: NSEvent.EventType) -> NSEvent {
        guard let event = NSEvent.mouseEvent(with: type, location: .zero, modifierFlags: [], timestamp: 0,
                                             windowNumber: controller.window.windowNumber, context: nil,
                                             eventNumber: 0, clickCount: 1, pressure: 1) else {
            preconditionFailure("no mouse event")
        }
        return event
    }

    func post(_ name: Notification.Name) {
        NotificationCenter.default.post(name: name, object: controller.window)
    }

    func testItCanGoFullScreenAndBeAnySize() {
        let window = controller.window
        XCTAssertTrue(window.collectionBehavior.contains(.fullScreenPrimary))
        XCTAssertTrue(window.styleMask.contains(.resizable))
        XCTAssertTrue(window.contentView === window.engineView)
        // ultrawide, and as tall as wide
        for size in [CGSize(width: 1720, height: 720), CGSize(width: 800, height: 800)] {
            window.setContentSize(size)
            let scale = window.backingScaleFactor
            XCTAssertTrue(window.engineView.sizeInPixels == Viewport.pixelSize(points: size, scale: scale), "\(size)")
        }
    }

    func testTheDrawableFollowsTheScreensScale() {
        let view = controller.window.engineView
        var resized: [Int] = []
        view.onResize = { width, _ in resized.append(width) }
        for scale in [1.0, 2.0, 1.0] {
            view.layout(scale: scale)
            XCTAssertEqual(view.metalLayer.contentsScale, scale)
            XCTAssertEqual(view.metalLayer.drawableSize,
                           CGSize(width: view.bounds.width * scale, height: view.bounds.height * scale))
        }
        XCTAssertEqual(Array(resized.suffix(2)), [1280, 640])
    }

    func testTheMouseIsCapturedOnFocusAndLetGoOnEscapeOrLossOfFocus() {
        post(NSWindow.didBecomeKeyNotification)
        XCTAssertTrue(pointer.captured)
        key(KeyCode.escape, down: true)
        XCTAssertFalse(pointer.captured)
        controller.window.engineView.mouseDown(with: mouseEvent(.leftMouseDown))
        XCTAssertTrue(pointer.captured)
        post(NSWindow.didResignKeyNotification)
        XCTAssertFalse(pointer.captured)
        XCTAssertEqual(pointer.changes, 4)
    }

    func testRawMotionCountsOnlyWhileCaptured() {
        mouse.move(5, 5)
        XCTAssertTrue(controller.takeMouseMotion() == (0, 0))
        post(NSWindow.didBecomeKeyNotification)
        mouse.move(3, -1)
        mouse.move(0.5, 2)
        // (AppKit's accelerated motion is not used while a raw mouse is there)
        controller.window.engineView.mouseMoved(with: mouseEvent(.mouseMoved))
        XCTAssertTrue(controller.takeMouseMotion() == (3.5, 1))
        XCTAssertTrue(controller.takeMouseMotion() == (0, 0))
        key(KeyCode.escape, down: true)
        mouse.move(7, 7)
        XCTAssertTrue(controller.takeMouseMotion() == (0, 0))
    }

    func testKeysAreHeldUntilLetGoOrTheFocusIsLost() {
        post(NSWindow.didBecomeKeyNotification)
        key(13, down: true)  // W
        key(1, down: true)   // S
        key(13, down: false)
        XCTAssertEqual(controller.keys.codes, [1])
        post(NSWindow.didResignKeyNotification)
        XCTAssertEqual(controller.keys.codes, [])
        // (Escape is the window's, not a key held)
        key(KeyCode.escape, down: true)
        XCTAssertFalse(controller.keys.isHeld(KeyCode.escape))
    }

    func testMouseButtonsAreHeldOnlyWhileCaptured() {
        let view = controller.window.engineView
        post(NSWindow.didBecomeKeyNotification)
        key(KeyCode.escape, down: true)
        // (the click that captures the mouse again is not the player's)
        view.mouseDown(with: mouseEvent(.leftMouseDown))
        XCTAssertTrue(pointer.captured)
        XCTAssertEqual(controller.keys.codes, [])
        view.mouseUp(with: mouseEvent(.leftMouseUp))
        view.mouseDown(with: mouseEvent(.leftMouseDown))
        view.rightMouseDown(with: mouseEvent(.rightMouseDown))
        XCTAssertEqual(controller.keys.codes, [MouseButton.left, MouseButton.right])
        view.mouseUp(with: mouseEvent(.leftMouseUp))
        XCTAssertEqual(controller.keys.codes, [MouseButton.right])
        // (let go with the mouse)
        key(KeyCode.escape, down: true)
        XCTAssertEqual(controller.keys.codes, [])
        XCTAssertEqual(MouseButton.code(2), MouseButton.middle)
        XCTAssertEqual(MouseButton.code(9), MouseButton.fifth)
    }
}
