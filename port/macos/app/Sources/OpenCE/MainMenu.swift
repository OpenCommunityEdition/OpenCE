// The menu bar: the app's menu, and View's fullscreen (Control-Command-F).

import AppKit

@MainActor
enum MainMenu {
    static func make() -> NSMenu {
        let bar = NSMenu()
        let app = NSMenu(title: "OpenCE")
        app.addItem(withTitle: "Hide OpenCE", action: #selector(NSApplication.hide(_:)), keyEquivalent: "h")
        app.addItem(.separator())
        app.addItem(withTitle: "Quit OpenCE", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q")
        bar.addItem(submenu(app))

        let view = NSMenu(title: "View")
        // (AppKit names it Exit Full Screen while the window is fullscreen)
        let fullscreen = view.addItem(withTitle: "Enter Full Screen", action: #selector(NSWindow.toggleFullScreen(_:)),
                                      keyEquivalent: "f")
        fullscreen.keyEquivalentModifierMask = [.control, .command]
        bar.addItem(submenu(view))

        let window = NSMenu(title: "Window")
        window.addItem(withTitle: "Minimize", action: #selector(NSWindow.performMiniaturize(_:)), keyEquivalent: "m")
        bar.addItem(submenu(window))
        NSApp.windowsMenu = window
        return bar
    }

    private static func submenu(_ menu: NSMenu) -> NSMenuItem {
        let item = NSMenuItem(title: menu.title, action: nil, keyEquivalent: "")
        item.submenu = menu
        return item
    }
}
