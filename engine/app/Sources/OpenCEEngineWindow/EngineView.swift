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

    /// Called on each of the display's frames, while the link runs.
    public var onFrame: ((CADisplayLink) -> Void)?
    /// Called when the drawable's size changes, in pixels.
    public var onResize: ((Int, Int) -> Void)?
    var onKey: ((UInt16, Bool) -> Void)?
    var onClick: (() -> Void)?
    /// AppKit's motion, right and down positive (accelerated: used only
    /// without a raw mouse).
    var onMotion: ((Double, Double) -> Void)?

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

    // ---------- the keyboard and mouse

    public override var acceptsFirstResponder: Bool { true }
    public override func acceptsFirstMouse(for event: NSEvent?) -> Bool { true }

    // (not passed on to super: the engine has the keys, and AppKit would beep)
    public override func keyDown(with event: NSEvent) {
        if !event.isARepeat {
            onKey?(event.keyCode, true)
        }
    }

    public override func keyUp(with event: NSEvent) {
        onKey?(event.keyCode, false)
    }

    public override func mouseDown(with event: NSEvent) { onClick?() }
    public override func rightMouseDown(with event: NSEvent) { onClick?() }

    private func motion(_ event: NSEvent) {
        onMotion?(Double(event.deltaX), Double(event.deltaY))
    }

    public override func mouseMoved(with event: NSEvent) { motion(event) }
    public override func mouseDragged(with event: NSEvent) { motion(event) }
    public override func rightMouseDragged(with event: NSEvent) { motion(event) }
    public override func otherMouseDragged(with event: NSEvent) { motion(event) }
}
