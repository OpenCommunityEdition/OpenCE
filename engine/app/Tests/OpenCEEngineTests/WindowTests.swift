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

    func testAMenuKeepsTheMouseFree() {
        var capture = MouseCapture()
        XCTAssertTrue(capture.handle(.focused))
        XCTAssertTrue(capture.allow(false), "a menu lets it go")
        XCTAssertFalse(capture.captured)
        XCTAssertFalse(capture.handle(.click), "a click is the menu's")
        _ = capture.handle(.unfocused)
        XCTAssertFalse(capture.handle(.focused), "and the focus does not take it")
        XCTAssertFalse(capture.allow(true), "allowed again, it waits for a click")
        XCTAssertTrue(capture.handle(.click))
        XCTAssertTrue(capture.captured)
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

    func testTheFrameCapHolds() {
        // (the cap below the display's rate, the display's below the cap, none)
        XCTAssertEqual(FramePacing.rate(display: 120, cap: 60, activity: .focused, keepsRunning: false), 60)
        XCTAssertEqual(FramePacing.rate(display: 60, cap: 144, activity: .focused, keepsRunning: false), 60)
        XCTAssertEqual(FramePacing.rate(display: 120, cap: 0, activity: .focused, keepsRunning: false), 120)
        XCTAssertEqual(FramePacing.rate(display: 0, cap: 0, activity: .focused, keepsRunning: false), 1)
    }

    func testTheWindowIdlesUnfocusedAndStopsWhenItCannotBeSeen() {
        XCTAssertEqual(FramePacing.rate(display: 120, cap: 60, activity: .unfocused, keepsRunning: false),
                       FramePacing.idleRate)
        XCTAssertNil(FramePacing.rate(display: 120, cap: 60, activity: .hidden, keepsRunning: false))
        // (a network game or the control socket keeps the rate: others wait on it)
        XCTAssertEqual(FramePacing.rate(display: 120, cap: 60, activity: .hidden, keepsRunning: true), 60)
        XCTAssertEqual(FramePacing.rate(display: 120, cap: 60, activity: .unfocused, keepsRunning: true), 60)
        XCTAssertEqual(FramePacing.rate(display: 5, cap: 0, activity: .unfocused, keepsRunning: false), 5)
    }

    func testTheActivityFollowsTheWindow() {
        XCTAssertEqual(WindowActivity(key: true, appActive: true, visible: true, miniaturised: false), .focused)
        XCTAssertEqual(WindowActivity(key: true, appActive: false, visible: true, miniaturised: false), .unfocused)
        XCTAssertEqual(WindowActivity(key: false, appActive: true, visible: true, miniaturised: false), .unfocused)
        XCTAssertEqual(WindowActivity(key: true, appActive: true, visible: false, miniaturised: false), .hidden)
        XCTAssertEqual(WindowActivity(key: false, appActive: false, visible: true, miniaturised: true), .hidden)
    }

    func testFramesInFlightStayBounded() async {
        let inFlight = FramesInFlight(limit: 2)
        XCTAssertTrue(inFlight.begin())
        XCTAssertTrue(inFlight.begin())
        XCTAssertFalse(inFlight.begin(), "a third frame waits for the GPU")
        XCTAssertEqual(inFlight.count, 2)
        inFlight.end()
        XCTAssertTrue(inFlight.begin())
        // (the GPU's completions come on its own threads)
        await withTaskGroup(of: Void.self) { group in
            for _ in 0..<2 {
                group.addTask { inFlight.end() }
            }
        }
        XCTAssertEqual(inFlight.count, 0)
        for _ in 0..<100 {
            if inFlight.begin() {
                XCTAssertLessThanOrEqual(inFlight.count, 2)
            }
        }
        XCTAssertEqual(inFlight.count, 2)
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

    func testTheViewIsPacedByItsActivityAndCap() {
        let view = controller.window.engineView
        let display = view.maximumFramesPerSecond
        view.frameCap = 30
        view.activity = .focused
        XCTAssertEqual(view.pacedRate, min(30, display))
        view.activity = .unfocused
        XCTAssertEqual(view.pacedRate, min(FramePacing.idleRate, display))
        view.startDisplayLink()
        defer { view.stopDisplayLink() }
        XCTAssertFalse(view.isDisplayLinkPaused)
        view.activity = .hidden
        XCTAssertNil(view.pacedRate)
        XCTAssertTrue(view.isDisplayLinkPaused)
        view.keepsRunning = true
        XCTAssertFalse(view.isDisplayLinkPaused)
    }

    func testLosingTheFocusIdlesTheView() {
        post(NSWindow.didResignKeyNotification)
        XCTAssertNotEqual(controller.window.engineView.activity, .focused)
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
