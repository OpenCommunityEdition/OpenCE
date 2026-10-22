// Draws a frame offscreen and reads it back: the golden frames, and a
// check that the renderer draws without a window.

import CoreGraphics
import Foundation
import ImageIO
import Metal
import OpenCEEngineCore
import UniformTypeIdentifiers

public enum FrameCaptureError: Error, CustomStringConvertible {
    case resources
    case image(String)

    public var description: String {
        switch self {
        case .resources: "the frame's texture or commands could not be made"
        case .image(let path): "\(path) could not be written"
        }
    }
}

/// A frame read back from the GPU: BGRA, 8 bits a channel, top row first.
public struct CapturedFrame {
    public let width: Int
    public let height: Int
    public let bytes: [UInt8]

    /// The red, green and blue of the texel at (x, y).
    public func rgb(x: Int, y: Int) -> SIMD3<UInt8> {
        let at = (y * width + x) * 4
        return SIMD3(bytes[at + 2], bytes[at + 1], bytes[at])
    }

    /// How many distinct colours the frame has (a level drawn has many;
    /// a frame of only the clear colour has one).
    public var distinctColors: Int {
        var seen = Set<UInt32>()
        for index in stride(from: 0, to: bytes.count, by: 4) {
            seen.insert(UInt32(bytes[index]) | UInt32(bytes[index + 1]) << 8 | UInt32(bytes[index + 2]) << 16)
        }
        return seen.count
    }

    /// The largest difference of a channel from `other`'s, and the share of
    /// texels differing by more than `tolerance`.
    public func difference(from other: CapturedFrame, tolerance: UInt8 = 8) -> (largest: Int, share: Double)? {
        guard other.width == width, other.height == height, other.bytes.count == bytes.count else { return nil }
        var largest = 0
        var differing = 0
        for index in stride(from: 0, to: bytes.count, by: 4) {
            var texel = 0
            for channel in 0..<3 {
                texel = max(texel, abs(Int(bytes[index + channel]) - Int(other.bytes[index + channel])))
            }
            largest = max(largest, texel)
            if texel > Int(tolerance) { differing += 1 }
        }
        return (largest, Double(differing) / Double(max(width * height, 1)))
    }

    /// Writes the frame as a PNG.
    public func writePNG(to url: URL) throws {
        let space = CGColorSpace(name: CGColorSpace.sRGB) ?? CGColorSpaceCreateDeviceRGB()
        let info = CGBitmapInfo(rawValue: CGImageAlphaInfo.noneSkipFirst.rawValue
            | CGBitmapInfo.byteOrder32Little.rawValue)
        guard let provider = CGDataProvider(data: Data(bytes) as CFData),
              let image = CGImage(width: width, height: height, bitsPerComponent: 8, bitsPerPixel: 32,
                                  bytesPerRow: width * 4, space: space, bitmapInfo: info, provider: provider,
                                  decode: nil, shouldInterpolate: false, intent: .defaultIntent),
              let destination = CGImageDestinationCreateWithURL(url as CFURL, UTType.png.identifier as CFString,
                                                                1, nil) else {
            throw FrameCaptureError.image(url.path)
        }
        CGImageDestinationAddImage(destination, image, nil)
        guard CGImageDestinationFinalize(destination) else { throw FrameCaptureError.image(url.path) }
    }

    /// A PNG read back (as `writePNG` writes them), or nil.
    public static func readPNG(at url: URL) -> CapturedFrame? {
        guard let source = CGImageSourceCreateWithURL(url as CFURL, nil),
              let image = CGImageSourceCreateImageAtIndex(source, 0, nil) else { return nil }
        let width = image.width
        let height = image.height
        var bytes = [UInt8](repeating: 0, count: width * height * 4)
        let space = CGColorSpace(name: CGColorSpace.sRGB) ?? CGColorSpaceCreateDeviceRGB()
        let info = CGImageAlphaInfo.noneSkipFirst.rawValue | CGBitmapInfo.byteOrder32Little.rawValue
        let drawn = bytes.withUnsafeMutableBytes { raw -> Bool in
            guard let context = CGContext(data: raw.baseAddress, width: width, height: height, bitsPerComponent: 8,
                                          bytesPerRow: width * 4, space: space, bitmapInfo: info) else { return false }
            context.draw(image, in: CGRect(x: 0, y: 0, width: width, height: height))
            return true
        }
        return drawn ? CapturedFrame(width: width, height: height, bytes: bytes) : nil
    }
}

extension LevelRenderer {
    /// Draws the level from `camera` at `width` by `height` offscreen
    /// `frames` times (for the temporal history) and reads the last frame
    /// back (the renderer's output format must be bgra8Unorm).
    public func capture(width: Int, height: Int, camera: ViewCamera, frames: Int = 1, objects: [DrawnObject] = [],
                        hudQuads: [HudQuad] = [], debugLines: [DebugSegment] = [],
                        queue: MTLCommandQueue) throws -> CapturedFrame {
        try capture(width: width, height: height, cameras: Array(repeating: camera, count: max(frames, 1)),
                    objects: objects, hudQuads: hudQuads, debugLines: debugLines, queue: queue)
    }

    /// Draws a frame from each of `cameras` in turn and reads the last back.
    public func capture(width: Int, height: Int, cameras: [ViewCamera], objects: [DrawnObject] = [],
                        hudQuads: [HudQuad] = [], debugLines: [DebugSegment] = [],
                        queue: MTLCommandQueue) throws -> CapturedFrame {
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: outputFormat, width: width,
                                                                  height: height, mipmapped: false)
        descriptor.usage = [.renderTarget, .shaderRead]
        descriptor.storageMode = .shared
        guard outputFormat == .bgra8Unorm, !cameras.isEmpty,
              let target = device.makeTexture(descriptor: descriptor) else { throw FrameCaptureError.resources }
        for camera in cameras {
            guard let commands = queue.makeCommandBuffer() else { throw FrameCaptureError.resources }
            encode(into: commands, target: target, camera: camera, objects: objects, hudQuads: hudQuads,
                   debugLines: debugLines)
            commands.commit()
            commands.waitUntilCompleted()
        }
        var bytes = [UInt8](repeating: 0, count: width * height * 4)
        bytes.withUnsafeMutableBytes { raw in
            guard let base = raw.baseAddress else { return }
            target.getBytes(base, bytesPerRow: width * 4, from: MTLRegionMake2D(0, 0, width, height), mipmapLevel: 0)
        }
        return CapturedFrame(width: width, height: height, bytes: bytes)
    }

    /// The last temporal frame's motion, in uv units (x right, y down),
    /// texel by texel from the top row, or nil if the last frame was not
    /// temporal.
    public func readMotion(queue: MTLCommandQueue) throws -> (width: Int, height: Int, motion: [SIMD2<Float>])? {
        guard let temporal else { return nil }
        let source = temporal.motion
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: source.pixelFormat, width: source.width,
                                                                  height: source.height, mipmapped: false)
        descriptor.storageMode = .shared
        guard let copy = device.makeTexture(descriptor: descriptor), let commands = queue.makeCommandBuffer(),
              let blit = commands.makeBlitCommandEncoder() else { throw FrameCaptureError.resources }
        blit.copy(from: source, to: copy)
        blit.endEncoding()
        commands.commit()
        commands.waitUntilCompleted()
        var halves = [Float16](repeating: 0, count: source.width * source.height * 2)
        halves.withUnsafeMutableBytes { raw in
            guard let base = raw.baseAddress else { return }
            copy.getBytes(base, bytesPerRow: source.width * 4, from: MTLRegionMake2D(0, 0, source.width, source.height),
                          mipmapLevel: 0)
        }
        let motion = stride(from: 0, to: halves.count, by: 2).map { SIMD2(Float(halves[$0]), Float(halves[$0 + 1])) }
        return (source.width, source.height, motion)
    }
}
