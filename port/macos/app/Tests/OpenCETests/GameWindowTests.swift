import AppKit
import OpenCEShell
import OpenCEWindow
import QuartzCore
import Testing

/// The game's windows (Sources/OpenCEWindow), made off-screen: what the
/// host's window imports report for them.
@MainActor
struct GameWindowTests {
    let windows = GameWindows()

    func make(width: Int = 640, height: Int = 480) throws -> (UInt32, GameWindow) {
        let id = windows.create(title: "Halo", width: width, height: height, show: false)
        try #require(id != 0)
        return (id, try #require(windows.window(id)))
    }

    @Test func aWindowReportsItsBackingSize() throws {
        let (id, window) = try make()
        defer { windows.close(id) }
        let scale = window.backingScaleFactor
        let points = try #require(window.contentView).bounds.size
        let size = try #require(windows.sizeInPixels(id))
        #expect(size.width == Int((points.width * scale).rounded()))
        #expect(size.height == Int((points.height * scale).rounded()))
        // and its layer draws at that size, at the screen's scale
        let layer = window.gameView.metalLayer
        #expect(layer.contentsScale == scale)
        #expect(layer.drawableSize == CGSize(width: size.width, height: size.height))
    }

    @Test func aResizeIsReported() throws {
        let (id, window) = try make()
        defer { windows.close(id) }
        window.setContentSize(NSSize(width: 800, height: 450))
        window.contentView?.layoutSubtreeIfNeeded()
        let scale = window.backingScaleFactor
        let size = try #require(windows.sizeInPixels(id))
        #expect(size.width == Int((800 * scale).rounded()))
        #expect(size.height == Int((450 * scale).rounded()))
        #expect(window.gameView.metalLayer.drawableSize == CGSize(width: size.width, height: size.height))
    }

    @Test func itCanGoFullScreenInASpaceOfItsOwn() throws {
        let (id, window) = try make()
        defer { windows.close(id) }
        #expect(window.collectionBehavior.contains(.fullScreenPrimary))
        #expect(window.styleMask.contains(.resizable))
        #expect(window.title == "Halo")
    }

    @Test func aWindowItDoesNotHaveHasNoSize() throws {
        #expect(windows.sizeInPixels(0) == nil)
        #expect(windows.sizeInPixels(99) == nil)
        let (id, _) = try make()
        windows.close(id)
        #expect(windows.sizeInPixels(id) == nil)
    }
}

struct WindowLayoutTests {
    @Test func pixelsAreThePointsAtTheScale() {
        #expect(WindowLayout.pixelSize(points: CGSize(width: 640, height: 480), scale: 2) == (1280, 960))
        // (a fraction of a pixel rounds, as AppKit's backing does)
        #expect(WindowLayout.pixelSize(points: CGSize(width: 100.25, height: 50.75), scale: 2) == (201, 102))
        #expect(WindowLayout.pixelSize(points: .zero, scale: 2) == (0, 0))
    }

    @Test func theFirstSizeFitsTheScreenInTheGamesShape() {
        let requested = CGSize(width: 640, height: 480)
        for visible in [CGSize(width: 1512, height: 944), CGSize(width: 3008, height: 1660),
                        CGSize(width: 600, height: 400), CGSize(width: 640, height: 480)] {
            let size = WindowLayout.initialContentSize(requested: requested, visible: visible)
            #expect(size.width <= visible.width && size.height <= visible.height, "\(visible)")
            #expect(size.width > 0 && size.height > 0, "\(visible)")
            #expect(abs(size.width / size.height - 4.0 / 3.0) < 0.01, "\(visible)")
        }
    }
}
