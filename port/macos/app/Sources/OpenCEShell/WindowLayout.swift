// The game window's sizes (Sources/OpenCEWindow), apart from AppKit.

import CoreGraphics

public enum WindowLayout {
    /// The drawable's size in pixels of a view `points` big on a screen of
    /// `scale` pixels to the point, rounded as AppKit's backing is.
    public static func pixelSize(points: CGSize, scale: CGFloat) -> (width: Int, height: Int) {
        (Int((points.width * scale).rounded()), Int((points.height * scale).rounded()))
    }

    /// The window's first content size, in points, for the size the game
    /// asks for (`requested`: 640 by 480 times display.window_scale) on a
    /// screen whose visible area (without the menu bar and the Dock) is
    /// `visible`. It is the size asked for when that fits (as the stock
    /// game's window is), else that shrunk, keeping its shape, to fit.
    public static func initialContentSize(requested: CGSize, visible: CGSize) -> CGSize {
        let requested = requested.width > 0 && requested.height > 0 ? requested : CGSize(width: 640, height: 480)
        // (the title bar is outside the content)
        let room = CGSize(width: max(visible.width, 1), height: max(visible.height - titleBarHeight, 1))
        let scale = min(1, room.width / requested.width, room.height / requested.height)
        return CGSize(width: requested.width * scale, height: requested.height * scale)
    }

    /// About the height of a window's title bar, in points.
    static let titleBarHeight: CGFloat = 28
}
