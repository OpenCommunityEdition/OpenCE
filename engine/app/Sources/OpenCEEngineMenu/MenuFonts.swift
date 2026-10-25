// The menus' fonts: Overpass for the words and OpenCE for the titles
// (port/assets/fonts), from the bundle's Resources/fonts or, run from the
// package, the repository's folder, registered for this process alone.
import CoreText
import Foundation
import SwiftUI

public enum MenuFonts {
    /// The font files, by what they draw.
    public static let files = (body: "Overpass-750.ttf", heading: "Overpass-900.ttf", title: "OpenCE-Regular.ttf")

    /// The PostScript names of the fonts registered, by file; empty until
    /// `register` finds them.
    @MainActor public private(set) static var names: [String: String] = [:]

    /// The folder with the fonts: the bundle's, else the repository's.
    public static func folder(bundle: Bundle = .main) -> URL? {
        let bundled = bundle.resourceURL?.appendingPathComponent("fonts")
        var repository = URL(fileURLWithPath: #filePath)
        for _ in 0..<5 {
            repository.deleteLastPathComponent()
        }
        repository.appendPathComponent("port/assets/fonts")
        return [bundled, repository].compactMap { $0 }.first {
            FileManager.default.fileExists(atPath: $0.appendingPathComponent(files.body).path)
        }
    }

    /// Registers the fonts in `folder` (once); true if all were found.
    @MainActor
    @discardableResult
    public static func register(folder: URL? = MenuFonts.folder()) -> Bool {
        guard let folder else { return false }
        for file in [files.body, files.heading, files.title] where names[file] == nil {
            let url = folder.appendingPathComponent(file)
            guard let descriptors = CTFontManagerCreateFontDescriptorsFromURL(url as CFURL) as? [CTFontDescriptor],
                let name = descriptors.first.flatMap({
                    CTFontDescriptorCopyAttribute($0, kCTFontNameAttribute) as? String
                })
            else { continue }
            // (already registered by an earlier call is as good)
            CTFontManagerRegisterFontsForURL(url as CFURL, .process, nil)
            names[file] = name
        }
        return names.count == 3
    }

    /// The body's font at `size`, the system's without Overpass.
    @MainActor
    public static func body(_ size: CGFloat) -> Font { font(files.body, size) }

    @MainActor
    public static func heading(_ size: CGFloat) -> Font { font(files.heading, size) }

    @MainActor
    public static func title(_ size: CGFloat) -> Font { font(files.title, size) }

    @MainActor
    private static func font(_ file: String, _ size: CGFloat) -> Font {
        names[file].map { Font.custom($0, fixedSize: size) } ?? .system(size: size, weight: .semibold)
    }
}
