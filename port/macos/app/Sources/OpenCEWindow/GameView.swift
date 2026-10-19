// The view the game draws in: a CAMetalLayer at the window's backing scale.

import AppKit
import Metal
import OpenCEShell
import QuartzCore

public final class GameView: NSView {
    public let metalLayer = CAMetalLayer()
    private let queue: MTLCommandQueue?

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
