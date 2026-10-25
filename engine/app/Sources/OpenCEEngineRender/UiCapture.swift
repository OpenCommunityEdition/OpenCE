// The reference captures of the original menus (--capture-ui): every
// screen ui.map's shell reaches, walked by the pointer and the back key as
// ui_walk.rs walks them, drawn headless over the UI scenario's scene and
// written as PNGs, with the palette the menus drew sampled from them. They
// go to assets/ui_reference, which git ignores: they are pictures of the
// user's own game data, for designing against, never shipped.

import Foundation
import Metal
import OpenCEEngineCore

/// The colours a set of frames drew most, by their share of the texels.
public enum UiPalette {
    /// A colour and the share of the sampled texels near it.
    public struct Swatch: Equatable, Codable, Sendable {
        public let red: UInt8
        public let green: UInt8
        public let blue: UInt8
        public let share: Double

        public var rgb: SIMD3<UInt8> { SIMD3(red, green, blue) }
        public var hex: String { String(format: "#%02X%02X%02X", red, green, blue) }
    }

    /// The `count` commonest colours of `frames`, joined in buckets of 16
    /// levels a channel and averaged within each. With a scene beside a
    /// frame, only the texels the menu changed over that scene count.
    public static func sample(_ frames: [(drawn: CapturedFrame, scene: CapturedFrame?)], count: Int) -> [Swatch] {
        var buckets = [Int: (texels: Int, sum: SIMD3<Int>)]()
        var total = 0
        for (drawn, scene) in frames {
            let under = scene.flatMap { $0.bytes.count == drawn.bytes.count ? $0.bytes : nil }
            for at in stride(from: 0, to: drawn.bytes.count, by: 4) {
                let colour = SIMD3(Int(drawn.bytes[at + 2]), Int(drawn.bytes[at + 1]), Int(drawn.bytes[at]))
                if let under {
                    let behind = SIMD3(Int(under[at + 2]), Int(under[at + 1]), Int(under[at]))
                    let change = colour &- behind
                    if max(abs(change.x), abs(change.y), abs(change.z)) <= 8 { continue }
                }
                let key = (colour.x >> 4) << 8 | (colour.y >> 4) << 4 | colour.z >> 4
                let bucket = buckets[key] ?? (0, .zero)
                buckets[key] = (bucket.texels + 1, bucket.sum &+ colour)
                total += 1
            }
        }
        return buckets.sorted {
            $0.value.texels != $1.value.texels ? $0.value.texels > $1.value.texels : $0.key < $1.key
        }
        .prefix(max(count, 0))
        .map { _, bucket in
            let mean = bucket.sum / bucket.texels
            return Swatch(
                red: UInt8(mean.x), green: UInt8(mean.y), blue: UInt8(mean.z),
                share: Double(bucket.texels) / Double(total))
        }
    }
}

/// Every screen of a menu drawn headless and written as PNGs.
public final class UiCapture {
    /// What a capture wrote: the screens' files, in the order reached, and
    /// the palette sampled from them.
    public struct Report: Codable {
        public let screens: [String]
        public let palette: [UiPalette.Swatch]
    }

    /// What the player does on a screen: clicks its `n`th target, or goes back.
    enum Step: Equatable {
        case click(Int)
        case back
    }

    let device: MTLDevice
    let queue: MTLCommandQueue
    let width: Int
    let height: Int

    /// Captures at `width` by `height` (640 by 480 is the menus' own size).
    public init(device: MTLDevice, queue: MTLCommandQueue, width: Int = 1280, height: Int = 960) {
        self.device = device
        self.queue = queue
        self.width = width
        self.height = height
    }

    /// Where the captures go: assets/ui_reference under the repository.
    public static func defaultFolder(root: URL) -> URL {
        root.appendingPathComponent("assets/ui_reference")
    }

    /// A screen's file: its path under ui\shell, its folders joined by -.
    public static func fileName(screen: String) -> String {
        var path = screen.replacingOccurrences(of: "/", with: "\\")
        if path.hasPrefix("ui\\shell\\") { path.removeFirst("ui\\shell\\".count) }
        let parts = path.split(separator: "\\").map { $0.replacingOccurrences(of: " ", with: "_") }
        return parts.joined(separator: "-") + ".png"
    }

    /// A menu on `geometry` with `widget` open and `steps` taken.
    func replay(_ geometry: LevelGeometry, widget: String, steps: [Step]) -> Menu? {
        let menu = Menu(geometry: geometry)
        let entries = { (names: [String]) in names.map { MenuEntry(map: $0, name: $0.uppercased()) } }
        menu.setLevels(levels: entries(["a10", "a30", "a50"]))
        menu.setMaps(maps: entries(["bloodgulch", "sidewinder"]))
        menu.setServers(servers: ["a server"])
        guard menu.open(widget: widget) else { return nil }
        var now: UInt64 = 0
        menu.update(nowMs: now)
        let (w, h) = (UInt32(width), UInt32(height))
        for step in steps {
            switch step {
            case .click(let n):
                let targets = menu.targets(width: w, height: h)
                if n < targets.count {
                    menu.pointerMoved(x: targets[n].x, y: targets[n].y, width: w, height: h)
                    menu.click(x: targets[n].x, y: targets[n].y, width: w, height: h, right: false)
                }
            case .back:
                menu.key(key: .escape)
            }
            // (long enough for the screens that close themselves)
            for _ in 0..<4 {
                now += 2_000
                menu.update(nowMs: now)
            }
            _ = menu.takeActions()
        }
        return menu
    }

    /// Every screen reached from `widget`, with the first steps found to
    /// it, breadth first (a screen's widgets can change while it stays
    /// open, so a place is its screen and its targets).
    func walk(_ geometry: LevelGeometry, widget: String) -> [(screen: String, steps: [Step])] {
        var screens = [(screen: String, steps: [Step])]()
        var seen = Set<String>()
        var queue = [[Step]()]
        var next = 0
        let (w, h) = (UInt32(width), UInt32(height))
        while next < queue.count {
            let steps = queue[next]
            next += 1
            guard let menu = replay(geometry, widget: widget, steps: steps), let screen = menu.screen() else {
                continue
            }
            let targets = menu.targets(width: w, height: h)
            let place = screen + targets.map { String(format: " %.0f,%.0f", $0.x, $0.y) }.joined()
            if steps.count > 12 || seen.count > 600 || !seen.insert(place).inserted { continue }
            if !screens.contains(where: { $0.screen == screen }) { screens.append((screen, steps)) }
            for step in (0..<targets.count).map(Step.click) + [.back] {
                queue.append(steps + [step])
            }
        }
        return screens
    }

    /// Draws every screen `widget` reaches on `geometry` over its scene
    /// into `folder`, with palette.json beside them.
    @discardableResult
    public func capture(geometry: LevelGeometry, widget: String, into folder: URL) throws -> Report {
        try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
        let renderer = try LevelRenderer(
            device: device, geometry: geometry,
            settings: VideoSettings(antiAliasing: .native, renderScale: 1))
        let atlas = Menu(geometry: geometry).glyphAtlas()
        renderer.ui = try UiPass(device: device, atlas: atlas, outputFormat: .bgra8Unorm)
        let start = geometry.startView()
        let camera = ViewCamera(
            position: [start.x, start.y, start.z], yaw: start.yaw, pitch: start.pitch,
            verticalFov: 70 * .pi / 180)
        let scene = try renderer.capture(width: width, height: height, camera: camera, queue: queue)
        var files = [String]()
        var frames = [(drawn: CapturedFrame, scene: CapturedFrame?)]()
        for (screen, steps) in walk(geometry, widget: widget) {
            guard let menu = replay(geometry, widget: widget, steps: steps) else { continue }
            let quads = menu.draw(width: UInt32(width), height: UInt32(height))
            let drawn = try renderer.capture(
                width: width, height: height, camera: camera, uiQuads: quads,
                queue: queue)
            let name = Self.fileName(screen: screen)
            try drawn.writePNG(to: folder.appendingPathComponent(name))
            files.append(name)
            frames.append((drawn, scene))
        }
        let report = Report(screens: files, palette: UiPalette.sample(frames, count: 24))
        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        try encoder.encode(report).write(to: folder.appendingPathComponent("palette.json"))
        return report
    }
}
