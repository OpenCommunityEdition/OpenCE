// The render ring's decoder, through its UniFFI bindings
// (port/macos/runtime/src/swift.rs, render.rs).
import Foundation
import Testing

import OpenCERuntime

// opence_render.h
private let magic = UInt32(0x4252_434f)
private let createBuffer = UInt32(1)
private let uploadBuffer = UInt32(2)

/// A batch of `commands`, each its op and fields.
private func batch(_ commands: [(UInt32, [UInt32])]) -> Data {
    var words: [UInt32] = [magic, 1, 0, UInt32(commands.count)]
    for (op, fields) in commands {
        words.append(op | UInt32(fields.count + 1) << 16)
        words.append(contentsOf: fields)
    }
    words[2] = UInt32(words.count * 4)
    var data = Data()
    for word in words {
        withUnsafeBytes(of: word.littleEndian) { data.append(contentsOf: $0) }
    }
    return data
}

@Test func commandsTheDecoderAcceptsComeBackAsSwiftValues() throws {
    let memory = try GuestMemoryView.withRanges(ranges: [
        GuestRangeSpec(address: 0x10_0000, size: 0x4000)
    ])
    let decoder = RenderDecoder(memory: memory)
    let decoded = try decoder.decodeBytes(
        bytes: batch([
            (createBuffer, [1, 64]),
            (uploadBuffer, [1, 0, 0x10_0000, 64]),
            (uploadBuffer, [1, 0, 0x30_0000, 64]),
        ]))
    #expect(decoded.commands.first == .createBuffer(id: 1, length: 64))
    #expect(decoded.commands.count == 2)
    // the upload from memory that is not mapped is refused, with its place
    #expect(decoded.rejected.count == 1)
    #expect(decoded.rejected.first?.error == .guest(address: 0x30_0000, length: 64))
}

@Test func aBatchThatIsNotOneThrows() throws {
    let memory = try GuestMemoryView.withRanges(ranges: [
        GuestRangeSpec(address: 0x10_0000, size: 0x4000)
    ])
    let decoder = RenderDecoder(memory: memory)
    #expect(throws: BatchError.Magic(magic: 0)) {
        try decoder.decodeBytes(bytes: Data(count: 16))
    }
    let made = batch([(createBuffer, [2, 16])])
    try memory.write(address: 0x10_0000, bytes: made)
    let decoded = try decoder.decodeGuest(address: 0x10_0000, size: UInt32(made.count))
    #expect(decoded.commands == [.createBuffer(id: 2, length: 16)])
}
