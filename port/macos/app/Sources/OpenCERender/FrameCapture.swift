// Frame captures (docs/macos_host.md, "Frame captures"): the app draws the
// guest's window off the screen, at a size of its own, and writes one of
// its frames to a PNG, which tools/test_macos_frames.py compares with an
// approved reference.

import Foundation
import ImageIO

public struct FrameCapture: Equatable, Sendable {
    /// The PNG written.
    public let path: URL
    /// The PRESENT whose frame is written, from 1.
    public let frame: Int
    public let width: Int
    public let height: Int

    public init(path: URL, frame: Int, width: Int, height: Int) {
        self.path = path
        self.frame = frame
        self.width = width
        self.height = height
    }

    public struct Invalid: Error, CustomStringConvertible {
        public let description: String
    }

    /// HALO_HOST_CAPTURE (the PNG), HALO_HOST_CAPTURE_FRAME (1) and
    /// HALO_HOST_CAPTURE_SIZE (640x480); nil without HALO_HOST_CAPTURE.
    public static func parse(_ environment: [String: String]) throws -> FrameCapture? {
        guard let path = environment["HALO_HOST_CAPTURE"], !path.isEmpty else { return nil }
        var frame = 1
        if let text = environment["HALO_HOST_CAPTURE_FRAME"] {
            guard let value = Int(text), value >= 1 else {
                throw Invalid(description: "HALO_HOST_CAPTURE_FRAME is \(text), not a frame from 1")
            }
            frame = value
        }
        var (width, height) = (640, 480)
        if let text = environment["HALO_HOST_CAPTURE_SIZE"] {
            // (Metal's largest texture is 16384 a side)
            let sides = text.split(separator: "x").compactMap { Int($0) }
            guard sides.count == 2, sides.allSatisfy({ (1...16384).contains($0) }) else {
                throw Invalid(description: "HALO_HOST_CAPTURE_SIZE is \(text), not a size such as 640x480")
            }
            (width, height) = (sides[0], sides[1])
        }
        return FrameCapture(path: URL(fileURLWithPath: path), frame: frame, width: width, height: height)
    }

    /// Writes `pixels` (RGBA, rows from the top, width by height) to the
    /// PNG; why it could not, or nil.
    func write(_ pixels: [UInt8]) -> String? {
        guard pixels.count == width * height * 4, let provider = CGDataProvider(data: Data(pixels) as CFData),
              let image = CGImage(width: width, height: height, bitsPerComponent: 8, bitsPerPixel: 32,
                                  bytesPerRow: width * 4, space: CGColorSpaceCreateDeviceRGB(),
                                  bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.last.rawValue),
                                  provider: provider, decode: nil, shouldInterpolate: false, intent: .defaultIntent),
              let destination = CGImageDestinationCreateWithURL(path as CFURL, "public.png" as CFString, 1, nil)
        else { return "the frame could not be made into a PNG" }
        CGImageDestinationAddImage(destination, image, nil)
        return CGImageDestinationFinalize(destination) ? nil : "\(path.path) could not be written"
    }
}
