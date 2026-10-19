// The view the game draws in: a CAMetalLayer at the window's backing scale,
// which passes on its keyboard and mouse (GameInput.swift).

import AppKit
import Metal
import OpenCEShell
import QuartzCore

public final class GameView: NSView {
    public let metalLayer = CAMetalLayer()
    private let queue: MTLCommandQueue?
    /// Where the view's keyboard and mouse go (GameWindows').
    var onInput: ((GameInputEvent) -> Void)?

    public init(frame: NSRect, device: MTLDevice? = MTLCreateSystemDefaultDevice()) {
        queue = device?.makeCommandQueue()
        super.init(frame: frame)
        metalLayer.device = device
        metalLayer.pixelFormat = .bgra8Unorm
        metalLayer.framebufferOnly = true
        // (black while the window is resized, not stretched)
        metalLayer.contentsGravity = .topLeft
        metalLayer.backgroundColor = NSColor.black.cgColor
        wantsLayer = true
        layerContentsRedrawPolicy = .never
        updateDrawableSize()
    }

    @available(*, unavailable)
    required init?(coder: NSCoder) {
        fatalError("GameView is made in code")
    }

    public override func makeBackingLayer() -> CALayer {
        metalLayer
    }

    // ---------- the keyboard and mouse

    /// (positions from the top left, as the game's are)
    public override var isFlipped: Bool { true }
    public override var acceptsFirstResponder: Bool { true }
    /// (the click that brings the window forward is the game's too)
    public override func acceptsFirstMouse(for event: NSEvent?) -> Bool { true }

    // (not passed on to super: the game has the keys, and AppKit would beep)
    public override func keyDown(with event: NSEvent) {
        onInput?(.key(code: event.keyCode, down: true, repeating: event.isARepeat))
    }

    public override func keyUp(with event: NSEvent) {
        onInput?(.key(code: event.keyCode, down: false, repeating: false))
    }

    public override func flagsChanged(with event: NSEvent) {
        onInput?(.modifiers(code: event.keyCode, flags: UInt64(event.modifierFlags.rawValue)))
    }

    private func button(_ number: Int, _ down: Bool, _ event: NSEvent) {
        let point = convert(event.locationInWindow, from: nil)
        onInput?(.mouseButton(number: number, down: down, x: Float(point.x), y: Float(point.y)))
    }

    private func motion(_ event: NSEvent) {
        let point = convert(event.locationInWindow, from: nil)
        // (AppKit's deltas are down positive, as SDL's are)
        onInput?(.mouseMotion(x: Float(point.x), y: Float(point.y), dx: Float(event.deltaX), dy: Float(event.deltaY)))
    }

    public override func mouseDown(with event: NSEvent) { button(0, true, event) }
    public override func mouseUp(with event: NSEvent) { button(0, false, event) }
    public override func rightMouseDown(with event: NSEvent) { button(1, true, event) }
    public override func rightMouseUp(with event: NSEvent) { button(1, false, event) }
    public override func otherMouseDown(with event: NSEvent) { button(event.buttonNumber, true, event) }
    public override func otherMouseUp(with event: NSEvent) { button(event.buttonNumber, false, event) }
    public override func mouseMoved(with event: NSEvent) { motion(event) }
    public override func mouseDragged(with event: NSEvent) { motion(event) }
    public override func rightMouseDragged(with event: NSEvent) { motion(event) }
    public override func otherMouseDragged(with event: NSEvent) { motion(event) }

    public override func scrollWheel(with event: NSEvent) {
        onInput?(.wheel(dx: Float(event.scrollingDeltaX), dy: Float(event.scrollingDeltaY),
                        precise: event.hasPreciseScrollingDeltas))
    }

    /// The window's pixels to the point, or the main screen's before the
    /// view is in a window.
    var backingScale: CGFloat {
        window?.backingScaleFactor ?? NSScreen.main?.backingScaleFactor ?? 1
    }

    /// The drawable's size in pixels, which the game draws at.
    public var sizeInPixels: (width: Int, height: Int) {
        WindowLayout.pixelSize(points: bounds.size, scale: backingScale)
    }

    public override func setFrameSize(_ newSize: NSSize) {
        super.setFrameSize(newSize)
        updateDrawableSize()
    }

    public override func viewDidChangeBackingProperties() {
        super.viewDidChangeBackingProperties()
        updateDrawableSize()
    }

    public override func viewDidMoveToWindow() {
        super.viewDidMoveToWindow()
        updateDrawableSize()
    }

    private func updateDrawableSize() {
        let scale = backingScale
        let (width, height) = sizeInPixels
        let size = CGSize(width: width, height: height)
        metalLayer.contentsScale = scale
        if metalLayer.drawableSize != size {
            metalLayer.drawableSize = size
            clear()
        }
    }

    /// Presents a black frame (until the renderer presents the game's).
    public func clear() {
        // (a window off the screen has nothing to show)
        guard window?.isVisible == true, let queue, metalLayer.drawableSize.width > 0,
              metalLayer.drawableSize.height > 0, let drawable = metalLayer.nextDrawable(),
              let commands = queue.makeCommandBuffer() else { return }
        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture = drawable.texture
        pass.colorAttachments[0].loadAction = .clear
        pass.colorAttachments[0].storeAction = .store
        pass.colorAttachments[0].clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 1)
        commands.makeRenderCommandEncoder(descriptor: pass)?.endEncoding()
        commands.present(drawable)
        commands.commit()
    }
}
