// The runtime's guest memory view, through its UniFFI bindings
// (port/macos/runtime/src/swift.rs).
import Foundation
import Testing

import OpenCERuntime

private let page = UInt64(0x4000)

private func view() throws -> GuestMemoryView {
    try GuestMemoryView.withRanges(ranges: [
        GuestRangeSpec(address: 0x10_0000, size: 2 * page),
        GuestRangeSpec(address: 0x20_0000, size: page),
    ])
}

@Test func aMappedRangeIsReadAndWrittenThroughTheBindings() throws {
    let memory = try view()
    #expect(memory.pageSize() == page)
    #expect(memory.isMapped(address: 0x10_0000, size: 2 * page))
    try memory.write(address: 0x10_3ffe, bytes: Data([1, 2, 3, 4]))
    #expect(try memory.read(address: 0x10_3ffe, size: 4) == Data([1, 2, 3, 4]))
}

@Test func theHostAddressIsTheGuestsBytesInPlace() throws {
    let memory = try view()
    try memory.write(address: 0x20_0010, bytes: Data([0xab]))
    let address = try memory.hostAddress(address: 0x20_0010, size: 1)
    #expect(UnsafePointer<UInt8>(bitPattern: UInt(address))?.pointee == 0xab)
}

@Test func rangesOutsideWhatIsMappedThrowTheirReason() throws {
    let memory = try view()
    #expect(!memory.isMapped(address: 0x30_0000, size: 1))
    #expect(throws: GuestMemoryError.Unmapped(address: 0x30_0000, size: 4)) {
        try memory.read(address: 0x30_0000, size: 4)
    }
    #expect(throws: GuestMemoryError.OutOfRange(address: 0xffff_fff0, size: 0x20)) {
        try memory.hostAddress(address: 0xffff_fff0, size: 0x20)
    }
    #expect(throws: GuestMemoryError.Empty(address: 0x10_0000)) {
        try memory.read(address: 0x10_0000, size: 0)
    }
}

@Test func overlappingRangesAreNotMapped() {
    #expect(throws: GuestMemoryError.self) {
        try GuestMemoryView.withRanges(ranges: [
            GuestRangeSpec(address: 0x10_0000, size: 2 * page),
            GuestRangeSpec(address: 0x10_4000, size: page),
        ])
    }
}

@Test func noGuestIsRunningInTheTests() {
    #expect(runningGuestMemory() == nil)
}
