import AppKit
import OpenCEWindow
import Testing

/// The keyboard and mouse of the game's windows (Sources/OpenCEWindow): what
/// their views pass on of AppKit's events, made here by hand.
@MainActor
struct GameInputTests {
    let windows = GameWindows()

    final class Recorder: GameInputSink, PointerControl {
        var events: [GameInputEvent] = []
        var captured: [Bool] = []

        func gameInput(window: UInt32, _ event: GameInputEvent) {
            events.append(event)
        }

        func setCaptured(_ captured: Bool, in window: NSWindow) {
            self.captured.append(captured)
        }
    }

    func make() throws -> (UInt32, GameWindow, Recorder) {
        let recorder = Recorder()
        windows.input = recorder
        windows.pointer = recorder
        let id = windows.create(title: "Halo", width: 640, height: 480, show: false)
        try #require(id != 0)
        return (id, try #require(windows.window(id)), recorder)
    }

    func key(_ type: NSEvent.EventType, code: UInt16, repeating: Bool = false, flags: NSEvent.ModifierFlags = [],
             in window: NSWindow) throws -> NSEvent {
        try #require(NSEvent.keyEvent(with: type, location: .zero, modifierFlags: flags, timestamp: 0,
                                      windowNumber: window.windowNumber, context: nil, characters: "",
                                      charactersIgnoringModifiers: "", isARepeat: repeating, keyCode: code))
    }

    func mouse(_ type: NSEvent.EventType, at point: NSPoint, in window: NSWindow) throws -> NSEvent {
        try #require(NSEvent.mouseEvent(with: type, location: point, modifierFlags: [], timestamp: 0,
                                        windowNumber: window.windowNumber, context: nil, eventNumber: 0,
                                        clickCount: 1, pressure: 1))
    }

    @Test func theViewTakesTheKeyboard() throws {
        let (id, window, _) = try make()
        defer { windows.close(id) }
        #expect(window.gameView.acceptsFirstResponder)
        #expect(window.firstResponder === window.gameView)
        #expect(window.acceptsMouseMovedEvents)
    }

    @Test func keysArePassedOnByTheirCodes() throws {
        let (id, window, recorder) = try make()
        defer { windows.close(id) }
        window.gameView.keyDown(with: try key(.keyDown, code: 0x0d, in: window))
        window.gameView.keyDown(with: try key(.keyDown, code: 0x0d, repeating: true, in: window))
        window.gameView.keyUp(with: try key(.keyUp, code: 0x0d, in: window))
        window.gameView.flagsChanged(with: try key(.flagsChanged, code: 0x38, flags: .shift, in: window))
        #expect(recorder.events == [
            .key(code: 0x0d, down: true, repeating: false),
            .key(code: 0x0d, down: true, repeating: true),
            .key(code: 0x0d, down: false, repeating: false),
            .modifiers(code: 0x38, flags: UInt64(NSEvent.ModifierFlags.shift.rawValue)),
        ])
    }

    @Test func mouseButtonsArePassedOnWhereTheyAreFromTheTop() throws {
        let (id, window, recorder) = try make()
        defer { windows.close(id) }
        let height = window.gameView.bounds.height
        // (AppKit's window coordinates are from the bottom left)
        window.gameView.mouseDown(with: try mouse(.leftMouseDown, at: NSPoint(x: 10, y: height - 20), in: window))
        window.gameView.rightMouseUp(with: try mouse(.rightMouseUp, at: NSPoint(x: 10, y: height - 20), in: window))
        #expect(recorder.events == [
            .mouseButton(number: 0, down: true, x: 10, y: 20),
            .mouseButton(number: 1, down: false, x: 10, y: 20),
        ])
    }

    @Test func focusIsPassedOn() throws {
        let (id, window, recorder) = try make()
        defer { windows.close(id) }
        NotificationCenter.default.post(name: NSWindow.didResignKeyNotification, object: window)
        NotificationCenter.default.post(name: NSWindow.didBecomeKeyNotification, object: window)
        #expect(recorder.events == [.focus(false), .focus(true)])
    }

    @Test func theCaptureIsThePointersAndIsLetGoAsTheWindowCloses() throws {
        let (id, _, recorder) = try make()
        #expect(windows.setRelativeMouse(id, true))
        #expect(windows.setRelativeMouse(id, true))
        #expect(windows.setRelativeMouse(id, false))
        #expect(windows.setRelativeMouse(id, true))
        windows.close(id)
        // (asked for twice, captured once; closing the window lets it go)
        #expect(recorder.captured == [true, false, true, false])
        #expect(!windows.setRelativeMouse(id, true))
    }
}
