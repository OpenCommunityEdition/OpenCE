// What a pass on colour texture 0 draws to: the game's window (a
// CAMetalLayer's drawables), or a texture off the screen for the tests and
// the frame captures.

import Foundation
import Metal
import QuartzCore

/// A frame of the window, drawn by the passes on colour 0 until PRESENT.
public struct RenderFrame {
    public let texture: MTLTexture
    let drawable: MTLDrawable?
}

public protocol RenderWindow: AnyObject, Sendable {
    /// The size in pixels the guest's window passes are checked against.
    var drawableSize: (width: Int, height: Int) { get }
    /// The frame to draw next, or nil when there is none to have.
    func nextFrame() -> RenderFrame?
    /// Shows `frame` once `commands` have run (before they are committed).
    func present(_ frame: RenderFrame, in commands: MTLCommandBuffer)
    /// After a PRESENT's `commands` are committed: something for the log,
    /// or nil.
    func committed(_ commands: MTLCommandBuffer) -> String?
}

extension RenderWindow {
    public func committed(_ commands: MTLCommandBuffer) -> String? { nil }
}

/// The game's window: its view's CAMetalLayer, BGRA8 (GameView), once the
/// guest has opened it (the guest makes its textures before).
public final class LayerWindow: RenderWindow, @unchecked Sendable {
    private let lock = NSLock()
    private var shown: CAMetalLayer?

    public init(layer: CAMetalLayer? = nil) {
        shown = layer
    }

    /// The layer drawn to (set on the main thread, read on the guest's).
    public var layer: CAMetalLayer? {
        get {
            lock.lock()
            defer { lock.unlock() }
            return shown
        }
        set {
            lock.lock()
            defer { lock.unlock() }
            shown = newValue
        }
    }

    public var drawableSize: (width: Int, height: Int) {
        guard let size = layer?.drawableSize else { return (0, 0) }
        return (Int(size.width), Int(size.height))
    }

    public func nextFrame() -> RenderFrame? {
        // (blocks while the layer's drawables are all in flight, which keeps
        // the guest no more than their number of frames ahead)
        guard let drawable = layer?.nextDrawable() else { return nil }
        return RenderFrame(texture: drawable.texture, drawable: drawable)
    }

    public func present(_ frame: RenderFrame, in commands: MTLCommandBuffer) {
        if let drawable = frame.drawable {
            commands.present(drawable)
        }
    }
}

/// A window off the screen: one BGRA8 texture, which each frame draws over,
/// and a count of the frames presented. With a capture, the frame it names
/// is written to its PNG before the guest is answered, so that the guest
/// may exit at once.
public final class OffscreenWindow: RenderWindow, @unchecked Sendable {
    public let texture: MTLTexture
    private let capture: FrameCapture?
    private var presentedFrames = 0
    private var capturing = false

    public init?(device: MTLDevice, width: Int, height: Int, capture: FrameCapture? = nil) {
        self.capture = capture
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .bgra8Unorm, width: width,
                                                                  height: height, mipmapped: false)
        descriptor.usage = [.renderTarget, .shaderRead]
        descriptor.storageMode = .shared
        guard let texture = device.makeTexture(descriptor: descriptor) else { return nil }
        self.texture = texture
    }

    public var drawableSize: (width: Int, height: Int) { (texture.width, texture.height) }

    public func nextFrame() -> RenderFrame? {
        RenderFrame(texture: texture, drawable: nil)
    }

    public func present(_ frame: RenderFrame, in commands: MTLCommandBuffer) {
        presentedFrames += 1
        capturing = presentedFrames == capture?.frame
    }

    public func committed(_ commands: MTLCommandBuffer) -> String? {
        guard capturing, let capture else { return nil }
        capturing = false
        commands.waitUntilCompleted()
        if let failed = capture.write(pixels().flatMap { $0 }) {
            return "frame \(capture.frame) was not captured: \(failed)"
        }
        return "captured frame \(capture.frame) to \(capture.path.path)"
    }

    /// The frames presented so far.
    public var presented: Int { presentedFrames }

    /// The texture's pixels, RGBA, rows from the top (once the renderer has
    /// finished drawing them).
    public func pixels() -> [[UInt8]] {
        let (width, height) = drawableSize
        var bytes = [UInt8](repeating: 0, count: width * height * 4)
        texture.getBytes(&bytes, bytesPerRow: width * 4, from: MTLRegionMake2D(0, 0, width, height), mipmapLevel: 0)
        return stride(from: 0, to: bytes.count, by: 4).map { [bytes[$0 + 2], bytes[$0 + 1], bytes[$0], bytes[$0 + 3]] }
    }
}
