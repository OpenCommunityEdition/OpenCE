import Foundation
import OpenCEShell
import Testing

struct DataRootTests {
    let support = URL(fileURLWithPath: "/Users/someone/Library/Application Support", isDirectory: true)

    @Test func theRootIsHaloDataRootWhenSet() {
        let root = DataRoot.resolve(environment: ["HALO_DATA_ROOT": "/Volumes/Games/Halo"], applicationSupport: support)
        #expect(root.path == "/Volumes/Games/Halo")
    }

    @Test func otherwiseItIsOpenCEsApplicationSupportFolder() {
        #expect(DataRoot.resolve(environment: [:], applicationSupport: support).path
            == "/Users/someone/Library/Application Support/OpenCE")
        // (set but empty is not set)
        #expect(DataRoot.resolve(environment: ["HALO_DATA_ROOT": ""], applicationSupport: support).path
            == "/Users/someone/Library/Application Support/OpenCE")
    }

    func temporaryRoot() throws -> URL {
        let root = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString, isDirectory: true)
        try FileManager.default.createDirectory(at: root, withIntermediateDirectories: true)
        return root
    }

    @Test func noMapsFolderGivesTheDialogsMessage() throws {
        let root = try temporaryRoot()
        defer { try? FileManager.default.removeItem(at: root) }
        let message = try #require(DataRoot.missingMapsMessage(root: root))
        // it says where to put them
        #expect(message.contains(DataRoot.maps(in: root).path))
    }

    @Test func anEmptyMapsFolderGivesItToo() throws {
        let root = try temporaryRoot()
        defer { try? FileManager.default.removeItem(at: root) }
        try FileManager.default.createDirectory(at: DataRoot.maps(in: root), withIntermediateDirectories: true)
        #expect(DataRoot.missingMapsMessage(root: root) != nil)
    }

    @Test func theStockMapsAreEnough() throws {
        let root = try temporaryRoot()
        defer { try? FileManager.default.removeItem(at: root) }
        let maps = DataRoot.maps(in: root)
        try FileManager.default.createDirectory(at: maps, withIntermediateDirectories: true)
        for name in ["ui.map", "bloodgulch.map"] {
            FileManager.default.createFile(atPath: maps.appendingPathComponent(name).path, contents: Data([0]))
        }
        #expect(DataRoot.missingMapsMessage(root: root) == nil)
    }
}
