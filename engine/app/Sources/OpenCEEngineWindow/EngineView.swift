// The view the engine draws in: a CAMetalLayer at the window's backing
// scale, its CAMetalDisplayLink paced by the frame cap and the window's
// activity (WindowModel.swift's FramePacing), and the keyboard and mouse
// passed on to its window's controller (EngineWindow.swift).

import AppKit
import Metal
import QuartzCore

/// A frame of the display link's.
public struct FrameTick: Sendable {
    /// When the frame is to be shown, in CACurrentMediaTime's seconds.
    public let timestamp: CFTimeInterval
    /// Whether it is the first since the link started or was paused (the
    /// time since the last is then not the game's).
    public let resumed: Bool
}

public final class EngineView: NSView {
    public let metalLayer = CAMetalLayer()
    public let device: MTLDevice?
    private let queue: MTLCommandQueue?
    private var displayLink: CAMetalDisplayLink?
    private let linkDelegate = LinkDelegate()
    /// The frame's drawable, while `onFrame` runs.
    private var drawable: CAMetalDrawable?
    private var resumed = true
    /// The frames committed and not yet finished by the GPU.
    public let inFlight = FramesInFlight(limit: EngineView.framesInFlight)
    private let overlay = NSTextField(labelWithString: "")

    /// The frames the CPU may be ahead of the GPU by.
    public static let framesInFlight = 2

    /// Called on each of the display link's frames.
    public var onFrame: ((FrameTick) -> Void)?
    /// The most frames a second (0 the display's rate).
    public var frameCap = 60 {
        didSet { pace() }
    }
    /// How much of the window the player has (its controller sets it).
    public var activity = WindowActivity.focused {
        didSet { pace() }
    }
    /// Whether the frames go on at the cap's rate whatever the window.
    public var keepsRunning = false {
        didSet { pace() }
    }
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
    /// The left button's press, drag and release, the pointer's moves
    /// with no button held, and the right button's press, in pixels from
    /// the top left, before the engine has them: true for one taken.
    public var pointerFilter: ((PointerPhase, Double, Double) -> Bool)?
    /// The wheel's motion up, in pixels (true for precise motion, as a
    /// trackpad's; else in lines), true when taken.
    public var scrollFilter: ((Double, Bool) -> Bool)?

    public enum PointerPhase: Sendable {
        case down, dragged, up, moved, rightDown
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
        linkDelegate.view = self
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

    /// Starts calling `onFrame`, at the rate `pacedRate` gives.
    public func startDisplayLink() {
        guard displayLink == nil else { return }
        let link = CAMetalDisplayLink(metalLayer: metalLayer)
        link.delegate = linkDelegate
        // (two drawables queued at most: the frame shown and the one drawn)
        link.preferredFrameLatency = Float(EngineView.framesInFlight)
        link.add(to: .main, forMode: .common)
        displayLink = link
        resumed = true
        pace()
    }

    public func stopDisplayLink() {
        displayLink?.invalidate()
        displayLink = nil
    }

    public var isDisplayLinkRunning: Bool { displayLink != nil }

    /// Whether the link runs but draws nothing (the window cannot be seen).
    public var isDisplayLinkPaused: Bool { displayLink?.isPaused ?? false }

    /// The rate of the screen the view is on (or the main one's).
    public var maximumFramesPerSecond: Int {
        (window?.screen ?? NSScreen.main)?.maximumFramesPerSecond ?? 60
    }

    /// The frames a second drawn now, nil for none.
    public var pacedRate: Int? {
        FramePacing.rate(
            display: maximumFramesPerSecond, cap: frameCap, activity: activity,
            keepsRunning: keepsRunning)
    }

    /// Sets the link's rate, or pauses it.
    private func pace() {
        guard let link = displayLink else { return }
        guard let rate = pacedRate else {
            if !link.isPaused {
                link.isPaused = true
                resumed = true
            }
            return
        }
        let range = FrameRate.range(maximumFramesPerSecond: rate)
        if link.preferredFrameRateRange != range {
            link.preferredFrameRateRange = range
        }
        link.isPaused = false
    }

    fileprivate func update(_ update: CAMetalDisplayLink.Update) {
        // (a move to a faster or slower screen changes the rate)
        pace()
        let tick = FrameTick(timestamp: update.targetTimestamp, resumed: resumed)
        resumed = false
        drawable = update.drawable
        defer { drawable = nil }
        onFrame?(tick)
    }

    /// The display link's callbacks, on the main thread's run loop (the
    /// link is added to it; the conformance checks so as it is called).
    @MainActor
    private final class LinkDelegate: NSObject, @preconcurrency CAMetalDisplayLinkDelegate {
        weak var view: EngineView?

        func metalDisplayLink(_ link: CAMetalDisplayLink, needsUpdate update: CAMetalDisplayLink.Update) {
            view?.update(update)
        }
    }

    /// The drawable to draw into: the link's frame's (once), else (before
    /// the link runs) the layer's next.
    private func nextDrawable() -> CAMetalDrawable? {
        guard window?.isVisible == true, metalLayer.drawableSize.width > 0,
            metalLayer.drawableSize.height > 0
        else { return nil }
        if displayLink != nil {
            defer { drawable = nil }
            return drawable
        }
        return metalLayer.nextDrawable()
    }

    /// Commits `commands`, presenting `drawable`, with its place in flight
    /// given back when the GPU is done.
    private func commit(_ commands: MTLCommandBuffer, presenting drawable: CAMetalDrawable) {
        let inFlight = inFlight
        commands.addCompletedHandler { _ in inFlight.end() }
        commands.present(drawable)
        commands.commit()
    }

    /// Presents a black frame (until the renderer draws).
    public func clear() {
        guard let queue, let drawable = nextDrawable(), inFlight.begin() else { return }
        guard let commands = queue.makeCommandBuffer() else {
            inFlight.end()
            return
        }
        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture = drawable.texture
        pass.colorAttachments[0].loadAction = .clear
        pass.colorAttachments[0].storeAction = .store
        pass.colorAttachments[0].clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 1)
        commands.makeRenderCommandEncoder(descriptor: pass)?.endEncoding()
        commit(commands, presenting: drawable)
    }

    /// Has `encode` draw into the frame's drawable, then presents it (a
    /// frame with every place in flight taken is skipped).
    public func render(_ encode: (MTLCommandBuffer, MTLTexture) -> Void) {
        guard let queue, let drawable = nextDrawable(), inFlight.begin() else { return }
        guard let commands = queue.makeCommandBuffer() else {
            inFlight.end()
            return
        }
        encode(commands, drawable.texture)
        commit(commands, presenting: drawable)
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
    public override func rightMouseDown(with event: NSEvent) {
        if !pointer(.rightDown, event) { onMouseButton?(MouseButton.right, true) }
    }
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

    public override func mouseMoved(with event: NSEvent) {
        if !pointer(.moved, event) { motion(event) }
    }
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
