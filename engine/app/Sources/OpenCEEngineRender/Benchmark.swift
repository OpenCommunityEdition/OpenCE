// The benchmark: the level drawn offscreen along a fixed flight path for a
// set time, as fast as the GPU goes, with up to three frames in flight as
// the window has. A frame's time is the time between the ends of two
// frames' GPU work, so it counts whichever of the CPU and the GPU is slower.

import Foundation
import Metal
import QuartzCore
import simd

/// The benchmark's flight: a loop around the start view, rising and
/// falling, looking ahead along the loop and down and up as it goes, once
/// around in `period` seconds.
public struct BenchmarkPath: Sendable {
    public let centre: SIMD3<Float>
    public let radius: Float
    public let height: Float
    public let period: Double

    public init(centre: SIMD3<Float>, radius: Float = 30, height: Float = 6, period: Double = 60) {
        self.centre = centre
        self.radius = radius
        self.height = height
        self.period = period
    }

    /// The camera `seconds` into the flight.
    public func camera(at seconds: Double, verticalFov: Float = 70 * .pi / 180) -> ViewCamera {
        let turn = Float((seconds / period).truncatingRemainder(dividingBy: 1)) * 2 * .pi
        let rise = height * (0.5 + 0.5 * sin(2 * turn))
        let position = centre + SIMD3(radius * cos(turn), radius * sin(turn), rise)
        // (the loop's tangent, counter-clockwise)
        let yaw = turn + .pi / 2
        let pitch = -0.15 + 0.2 * sin(3 * turn)
        return ViewCamera(position: position, yaw: yaw, pitch: pitch, verticalFov: verticalFov)
    }
}

/// The benchmark's frame times and what they were drawn at.
public struct BenchmarkResult: Sendable {
    public let width: Int
    public let height: Int
    public let seconds: Double
    /// Each frame's time, in milliseconds.
    public let frameTimes: [Double]
    /// Each frame's GPU time (from its first command to its last), in milliseconds.
    public let gpuTimes: [Double]

    /// The `percentile` (0 to 100) of `times`, by the nearest rank.
    public static func percentile(_ percentile: Double, of times: [Double]) -> Double {
        guard !times.isEmpty else { return 0 }
        let sorted = times.sorted()
        let rank = Int((percentile / 100 * Double(sorted.count)).rounded(.up)) - 1
        return sorted[min(max(rank, 0), sorted.count - 1)]
    }

    public var p50: Double { Self.percentile(50, of: frameTimes) }
    public var p99: Double { Self.percentile(99, of: frameTimes) }
    public var gpuP50: Double { Self.percentile(50, of: gpuTimes) }
    public var gpuP99: Double { Self.percentile(99, of: gpuTimes) }

    /// The result as JSON, for tools/test_engine_benchmark.py.
    public var json: String {
        func number(_ value: Double) -> String { String(format: "%.3f", value) }
        return "{\"width\": \(width), \"height\": \(height), \"seconds\": \(number(seconds)), "
            + "\"frames\": \(frameTimes.count), \"p50_ms\": \(number(p50)), \"p99_ms\": \(number(p99)), "
            + "\"gpu_p50_ms\": \(number(gpuP50)), \"gpu_p99_ms\": \(number(gpuP99))}"
    }
}

/// The frames' ends, gathered from the command buffers' completion handlers.
private final class FrameClock: @unchecked Sendable {
    private let lock = NSLock()
    private var lastEnd: CFTimeInterval?
    private(set) var frameTimes: [Double] = []
    private(set) var gpuTimes: [Double] = []

    func completed(start: CFTimeInterval, end: CFTimeInterval) {
        lock.lock()
        defer { lock.unlock() }
        if let lastEnd {
            frameTimes.append((end - lastEnd) * 1000)
        }
        gpuTimes.append((end - start) * 1000)
        lastEnd = end
    }

    var results: ([Double], [Double]) {
        lock.lock()
        defer { lock.unlock() }
        return (frameTimes, gpuTimes)
    }
}

extension LevelRenderer {
    /// Flies `path` for `seconds`, drawing at `width` by `height`, after
    /// `warmUp` frames that are not counted (the pipelines' and the
    /// temporal history's first use).
    public func benchmark(width: Int, height: Int, path: BenchmarkPath, seconds: Double, objects: [DrawnObject] = [],
                          warmUp: Int = 60, queue: MTLCommandQueue) throws -> BenchmarkResult {
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: outputFormat, width: width,
                                                                  height: height, mipmapped: false)
        descriptor.usage = [.renderTarget, .shaderRead]
        descriptor.storageMode = .private
        let inFlight = 3
        var targets: [MTLTexture] = []
        for _ in 0..<inFlight {
            guard let target = device.makeTexture(descriptor: descriptor) else { throw FrameCaptureError.resources }
            targets.append(target)
        }
        let slots = DispatchSemaphore(value: inFlight)
        let clock = FrameClock()
        var frame = 0
        var started: CFTimeInterval?
        var last: MTLCommandBuffer?
        while true {
            let now = CACurrentMediaTime()
            if frame == warmUp {
                last?.waitUntilCompleted()
                started = CACurrentMediaTime()
            }
            let elapsed = started.map { now - $0 } ?? 0
            if started != nil, elapsed >= seconds { break }
            slots.wait()
            guard let commands = queue.makeCommandBuffer() else {
                slots.signal()
                throw FrameCaptureError.resources
            }
            let counted = started != nil
            commands.addCompletedHandler { buffer in
                if counted {
                    clock.completed(start: buffer.gpuStartTime, end: buffer.gpuEndTime)
                }
                slots.signal()
            }
            encode(into: commands, target: targets[frame % inFlight], camera: path.camera(at: elapsed),
                   objects: objects)
            commands.commit()
            last = commands
            frame += 1
        }
        // (every frame's handler has run once its slot is back)
        // (and given back: a semaphore freed below its first value traps)
        for _ in 0..<inFlight { slots.wait() }
        for _ in 0..<inFlight { slots.signal() }
        let (frameTimes, gpuTimes) = clock.results
        return BenchmarkResult(width: width, height: height, seconds: seconds, frameTimes: frameTimes,
                               gpuTimes: gpuTimes)
    }
}
