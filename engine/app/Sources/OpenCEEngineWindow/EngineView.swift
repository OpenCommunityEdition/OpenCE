// The view the engine draws in: a CAMetalLayer at the window's backing
// scale, the display link at the display's rate, and the keyboard and
// mouse passed on to its window's controller (EngineWindow.swift).

import AppKit
import Metal
import QuartzCore

public final class EngineView: NSView {
    public let metalLayer = CAMetalLayer()
    public let device: MTLDevice?
    private let queue: MTLCommandQueue?
    private var displayLink: CADisplayLink?
    private let overlay = NSTextField(labelWithString: "")

    /// Called on each of the display's frames, while the link runs.
    public var onFrame: ((CADisplayLink) -> Void)?
    /// Called when the drawable's size changes, in pixels.
    public var onResize: ((Int, Int) -> Void)?
    var onKey: ((UInt16, Bool) -> Void)?
    /// A mouse button pressed or let go, by its code in `MouseButton`.
    var onMouseButton: ((UInt16, Bool) -> Void)?
    /// AppKit's motion, right and down positive (accelerated: used only
    /// without a raw mouse).
    var onMotion: ((Double, Double) -> Void)?
    /// The keys' events before the engine has them (repeats too), true
    /// for one taken: the developer shell's typing.
    public var keyFilter: ((NSEvent) -> Bool)?
    /// The left button's press, drag and release, in pixels from the top
    /// left, before the engine has them: true for one taken.
    public var pointerFilter: ((PointerPhase, Double, Double) -> Bool)?
    /// The wheel's motion up, in pixels (true for precise motion, as a
    /// trackpad's; else in lines), true when taken.
    public var scrollFilter: ((Double, Bool) -> Bool)?

    public enum PointerPhase: Sendable {
        case down, dragged, up
    }

    public init(frame: NSRect, device: MTLDevice? = MTLCreateSystemDefaultDevice()) {
        self.device = device
        queue = device?.makeCommandQueue()
        super.init(frame: frame)
        metalLayer.device = device
        metalLayer.pixelFormat = .bgra8Unorm
        metalLayer.framebufferOnly = true
        metalLayer.contentsGravity = .topLeft
        metalLayer.backgroundColor = NSColor.black.cgColor
        wantsLayer = true
        layerContentsRedrawPolicy = .never
        overlay.font = .monospacedDigitSystemFont(ofSize: 12, weight: .medium)
        overlay.textColor = .white
        overlay.backgroundColor = NSColor.black.withAlphaComponent(0.55)
        overlay.drawsBackground = true
        overlay.isHidden = true
        overlay.translatesAutoresizingMaskIntoConstraints = false
        addSubview(overlay)
        NSLayoutConstraint.activate([
            overlay.leadingAnchor.constraint(equalTo: leadingAnchor, constant: 8),
            overlay.topAnchor.constraint(equalTo: topAnchor, constant: 8),
        ])
        layout(scale: backingScale)
    }

    @available(*, unavailable)
    required init?(coder: NSCoder) {
        fatalError("EngineView is made in code")
    }

    public override func makeBackingLayer() -> CALayer {
        metalLayer
    }

    // ---------- size

    var backingScale: CGFloat {
        window?.backingScaleFactor ?? NSScreen.main?.backingScaleFactor ?? 1
    }

    /// The drawable's size in pixels, which the engine draws at.
    public var sizeInPixels: (width: Int, height: Int) {
        let size = metalLayer.drawableSize
        return (Int(size.width), Int(size.height))
    }

    public override func setFrameSize(_ newSize: NSSize) {
        super.setFrameSize(newSize)
        layout(scale: backingScale)
    }

    public override func viewDidChangeBackingProperties() {
        super.viewDidChangeBackingProperties()
        layout(scale: backingScale)
    }

    public override func viewDidMoveToWindow() {
        super.viewDidMoveToWindow()
        layout(scale: backingScale)
    }

    /// Sizes the drawable for the bounds at `scale` (the window's, or a
    /// test's).
    func layout(scale: CGFloat) {
        let (width, height) = Viewport.pixelSize(points: bounds.size, scale: scale)
        let size = CGSize(width: width, height: height)
        metalLayer.contentsScale = scale
        if metalLayer.drawableSize != size {
            metalLayer.drawableSize = size
            onResize?(width, height)
        }
    }

    // ---------- the display link

    /// Starts calling `onFrame` at the display's rate.
    public func startDisplayLink() {
        guard displayLink == nil else { return }
        let link = displayLink(target: self, selector: #selector(frame(_:)))
        link.preferredFrameRateRange = FrameRate.range(maximumFramesPerSecond: maximumFramesPerSecond)
        link.add(to: .main, forMode: .common)
        displayLink = link
    }

    public func stopDisplayLink() {
        displayLink?.invalidate()
        displayLink = nil
    }

    public var isDisplayLinkRunning: Bool { displayLink != nil }

    /// The rate of the screen the view is on (or the main one's).
    public var maximumFramesPerSecond: Int {
        (window?.screen ?? NSScreen.main)?.maximumFramesPerSecond ?? 60
    }

    @objc private func frame(_ link: CADisplayLink) {
        // (a move to a faster or slower screen changes the rate)
        let range = FrameRate.range(maximumFramesPerSecond: maximumFramesPerSecond)
        if link.preferredFrameRateRange != range {
            link.preferredFrameRateRange = range
        }
        onFrame?(link)
    }

    /// Presents a black frame (until the renderer draws).
    public func clear() {
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

    /// Has `encode` draw into the next drawable, then presents it.
    public func render(_ encode: (MTLCommandBuffer, MTLTexture) -> Void) {
        guard window?.isVisible == true, let queue, metalLayer.drawableSize.width > 0,
              metalLayer.drawableSize.height > 0, let drawable = metalLayer.nextDrawable(),
              let commands = queue.makeCommandBuffer() else { return }
        encode(commands, drawable.texture)
        commands.present(drawable)
        commands.commit()
    }

    // ---------- the frame-time overlay

    /// The overlay's line, or nil to hide it.
    public var overlayText: String? {
        get { overlay.isHidden ? nil : overlay.stringValue }
        set {
            overlay.isHidden = newValue == nil
            if let newValue, overlay.stringValue != newValue {
                overlay.stringValue = newValue
            }
        }
    }

    // ---------- the keyboard and mouse

    public override var acceptsFirstResponder: Bool { true }
    public override func acceptsFirstMouse(for event: NSEvent?) -> Bool { true }

    // (not passed on to super: the engine has the keys, and AppKit would beep)
    public override func keyDown(with event: NSEvent) {
        if keyFilter?(event) == true {
            return
        }
        if !event.isARepeat {
            onKey?(event.keyCode, true)
        }
    }

    public override func keyUp(with event: NSEvent) {
        if keyFilter?(event) == true {
            return
        }
        onKey?(event.keyCode, false)
    }

    // (Shift, Control and Option come as changes of the flags, not keys)
    public override func flagsChanged(with event: NSEvent) {
        guard let flag = ModifierKey.flag(event.keyCode) else { return }
        onKey?(event.keyCode, event.modifierFlags.contains(flag))
    }

    /// `event`'s place in pixels from the top left.
    private func pixels(_ event: NSEvent) -> (Double, Double) {
        let point = convert(event.locationInWindow, from: nil)
        return (Double(point.x * backingScale), Double((bounds.height - point.y) * backingScale))
    }

    private func pointer(_ phase: PointerPhase, _ event: NSEvent) -> Bool {
        guard let pointerFilter else { return false }
        let (x, y) = pixels(event)
        return pointerFilter(phase, x, y)
    }

    public override func mouseDown(with event: NSEvent) {
        if !pointer(.down, event) { onMouseButton?(MouseButton.left, true) }
    }
    public override func mouseUp(with event: NSEvent) {
        // (the engine has its release too, so that no button stays held)
        _ = pointer(.up, event)
        onMouseButton?(MouseButton.left, false)
    }
    public override func rightMouseDown(with event: NSEvent) { onMouseButton?(MouseButton.right, true) }
    public override func rightMouseUp(with event: NSEvent) { onMouseButton?(MouseButton.right, false) }
    public override func otherMouseDown(with event: NSEvent) {
        onMouseButton?(MouseButton.code(event.buttonNumber), true)
    }
    public override func otherMouseUp(with event: NSEvent) {
        onMouseButton?(MouseButton.code(event.buttonNumber), false)
    }

    private func motion(_ event: NSEvent) {
        onMotion?(Double(event.deltaX), Double(event.deltaY))
    }

    public override func mouseMoved(with event: NSEvent) { motion(event) }
    public override func mouseDragged(with event: NSEvent) {
        if !pointer(.dragged, event) { motion(event) }
    }

    public override func scrollWheel(with event: NSEvent) {
        let precise = event.hasPreciseScrollingDeltas
        let delta = Double(event.scrollingDeltaY) * (precise ? Double(backingScale) : 1)
        _ = scrollFilter?(delta, precise)
    }
    public override func rightMouseDragged(with event: NSEvent) { motion(event) }
    public override func otherMouseDragged(with event: NSEvent) { motion(event) }
}
