import OpenCEEngineCore
import OpenCEEngineMenu
import OpenCEEngineRender
import XCTest

/// The dev panel's model: the object clicked inspected, and the timings
/// and sounds read four times a second.
@MainActor
final class DevPanelTests: XCTestCase {
    /// The lines inspected, the reads, and what the sources give.
    private final class Fake {
        var inspected: [String] = []
        var reads = 0
        var gone: Set<UInt32> = []
        var sounds = [SoundClassCount(class: "weapon_fire", playing: 2, culled: 1, failed: 0)]

        func sources() -> DevPanelModel.Sources {
            DevPanelModel.Sources(
                inspect: { id in
                    self.inspected.append("inspect object=\(id)")
                    return self.gone.contains(id)
                        ? ShellOutput(ok: false, text: "no object \(id)", json: "null", action: .none)
                        : ShellOutput(ok: true, text: "object \(id): warthog", json: "null", action: .none)
                },
                gpu: { [PassTime(name: "level", milliseconds: 1.5), PassTime(name: "screen", milliseconds: 0.25)] },
                cpu: {
                    self.reads += 1
                    return [ScopeTime(name: "advance", depth: 0, count: 1, milliseconds: 2, ownMilliseconds: 0.5)]
                },
                hitches: { [] },
                sounds: { self.sounds })
        }
    }

    func testAPickIsInspectedAndFollowed() {
        let fake = Fake()
        let model = DevPanelModel(sources: fake.sources())
        XCTAssertEqual(model.inspection, DevPanelModel.unpicked)
        model.pick(7)
        XCTAssertEqual(model.inspected, 7)
        XCTAssertEqual(model.inspection, "object 7: warthog")
        XCTAssertEqual(fake.inspected, ["inspect object=7"])
        // (read again as it plays)
        model.update(now: 0)
        XCTAssertEqual(fake.inspected.count, 2)
        // a click on no object
        model.pick(nil)
        XCTAssertNil(model.inspected)
        XCTAssertEqual(model.inspection, DevPanelModel.nothing)
        // one gone is shown so, and no longer read
        fake.gone = [9]
        model.pick(9)
        XCTAssertEqual(model.inspection, "no object 9")
        XCTAssertNil(model.inspected)
        model.update(now: 1)
        XCTAssertEqual(fake.inspected.count, 3)
        // a new level forgets it
        model.pick(7)
        model.reset()
        XCTAssertNil(model.inspected)
        XCTAssertEqual(model.inspection, DevPanelModel.unpicked)
    }

    func testTheTimingsAndSoundsAreReadFourTimesASecond() {
        let fake = Fake()
        let model = DevPanelModel(sources: fake.sources())
        model.update(now: 10)
        XCTAssertEqual(fake.reads, 1)
        XCTAssertEqual(model.gpuMilliseconds, 1.75, accuracy: 1e-9)
        XCTAssertEqual(model.cpu.map(\.name), ["advance"])
        XCTAssertEqual(model.sounds.first?.playing, 2)
        model.update(now: 10.1)
        XCTAssertEqual(fake.reads, 1, "read again within a quarter second")
        fake.sounds = []
        model.update(now: 10.25)
        XCTAssertEqual(fake.reads, 2)
        XCTAssertTrue(model.sounds.isEmpty)
    }
}
