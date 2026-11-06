package com.halo.decomp;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Arrays;
import java.util.Base64;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public final class MccPackageImporterTest {
    private static int checks;
    interface Checked { void run() throws Exception; }
    static void check(boolean value) { checks++; if (!value) throw new AssertionError("check " + checks); }
    static void refuses(Checked action) throws Exception {
        checks++;
        try { action.run(); } catch (IOException expected) { return; }
        throw new AssertionError("Expected refusal at " + checks);
    }
    static byte[] bytes(String text) { return text.getBytes(StandardCharsets.UTF_8); }
    static byte[] map(int type) {
        byte[] bytes = new byte[2088];
        ByteBuffer h = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
        h.putInt(0, 0x68656164); h.putInt(4, 13); h.putInt(8, bytes.length);
        h.putInt(16, 2048); h.putInt(20, 40); h.putShort(0x60, (short)type); h.putInt(0x7fc, 0x666f6f74);
        return bytes;
    }
    static Map<String,byte[]> campaign() {
        Map<String,byte[]> entries = new LinkedHashMap<>();
        entries.put("2915693904/ModInfo.json", bytes("{\"Engine\":\"Halo1\",\"Title\":{\"Neutral\":\"Example\"},\"ModIdentifier\":{\"HostedModIds\":{\"SteamWorkshopId\":2915693904}},\"GameModContents\":{\"HasCampaign\":true}}"));
        entries.put("2915693904/CampaignInfo.json", bytes("{\"CampaignMaps\":[\"Campaign/Z10.json\",\"campaign/a10.json\"]}"));
        entries.put("2915693904/campaign/z10.json", bytes("{\"ScenarioFile\":\"levels/test/z10\",\"CampaignMapKind\":\"Default\"}"));
        entries.put("2915693904/campaign/a10.json", bytes("{\"ScenarioFile\":\"a10\",\"Images\":{\"Large\":\"Assets/Level Large.PNG\",\"Thumbnail\":\"Thumb.PNG\"}}"));
        entries.put("2915693904/Assets/Level Large.PNG", preview());
        entries.put("2915693904/Thumb.PNG", preview());
        entries.put("2915693904/Maps/z10.map", map(0));
        entries.put("2915693904/Maps/a10.map", map(0));
        return entries;
    }
    static byte[] preview() {
        // Generated one-pixel PNG; no game or Workshop artwork in the tests.
        return Base64.getDecoder().decode("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR4nGPgNgitAgAB5QELSSf/aQAAAABJRU5ErkJggg==");
    }
    static MccPackageImporter.Source source(Map<String,byte[]> entries) {
        return receiver -> { for (Map.Entry<String,byte[]> entry : entries.entrySet()) receiver.file(entry.getKey(), new ByteArrayInputStream(entry.getValue())); };
    }
    static MccPackageImporter.Source zip(Map<String,byte[]> entries) throws IOException {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        try (ZipOutputStream zip = new ZipOutputStream(bytes)) {
            for (Map.Entry<String,byte[]> entry : entries.entrySet()) {
                zip.putNextEntry(new ZipEntry(entry.getKey())); zip.write(entry.getValue()); zip.closeEntry();
            }
        }
        return MccPackageImporter.zip(new ByteArrayInputStream(bytes.toByteArray()));
    }
    static void noStaging(Path root) throws IOException {
        try (java.util.stream.Stream<Path> children = Files.list(root)) { check(children.noneMatch(p -> p.getFileName().toString().startsWith(".mcc-import-"))); }
    }
    public static void main(String[] args) throws Exception {
        Path root = Files.createTempDirectory(Path.of(args[0]), "importer-");
        try {
            Files.createDirectories(root.resolve("maps")); Files.writeString(root.resolve("maps/ui.map"), "Xbox stays");
            Files.createDirectories(root.resolve("custom_maps")); Files.writeString(root.resolve("custom_maps/example.map"), "CE stays");
            Files.createDirectories(root.resolve("saves")); Files.writeString(root.resolve("saves/savegame.bin"), "save stays");
            try (MccPackageImporter.Prepared prepared = MccPackageImporter.prepare(root.toFile(), source(campaign()), "", 0)) {
                check(prepared.maps == 2 && prepared.destination.equals("2915693904"));
                check(!prepared.exists(root.toFile()));
                refuses(() -> MccPackageImporter.prepare(root.toFile(), source(campaign()), "Another", 0));
                prepared.install(root.toFile(), false);
            }
            Path installed = root.resolve("mcc_maps/2915693904");
            check(Files.readString(installed.resolve("workshop_id.txt")).startsWith("2915693904\nExample\n"));
            check(Files.exists(installed.resolve("campaign/z10.json")) && Files.exists(installed.resolve("z10.map")));
            check(Arrays.equals(preview(), Files.readAllBytes(installed.resolve("assets/level large.png"))));
            check(Arrays.equals(preview(), Files.readAllBytes(installed.resolve("thumb.png"))));
            check(Files.readString(installed.resolve("campaign/a10.json")).contains("Assets/Level Large.PNG"));
            noStaging(root);
            try (MccPackageImporter.Prepared prepared = MccPackageImporter.prepare(root.toFile(), zip(campaign()), "Mobile copy", 0)) {
                prepared.install(root.toFile(), false);
            }
            check(Files.exists(root.resolve("mcc_maps/Mobile copy/a10.map")));
            check(Arrays.equals(preview(), Files.readAllBytes(root.resolve("mcc_maps/Mobile copy/assets/level large.png"))));
            check(Arrays.equals(preview(), Files.readAllBytes(root.resolve("mcc_maps/Mobile copy/thumb.png"))));
            Files.writeString(installed.resolve("old-marker"), "previous");
            try (MccPackageImporter.Prepared prepared = MccPackageImporter.prepare(root.toFile(), source(campaign()), "", 0)) {
                refuses(() -> prepared.install(root.toFile(), false));
                check(Files.exists(installed.resolve("old-marker")));
                prepared.install(root.toFile(), true);
            }
            check(!Files.exists(installed.resolve("old-marker")));
            // Cancel after preparation leaves installed content unchanged.
            try (MccPackageImporter.Prepared prepared = MccPackageImporter.prepare(root.toFile(), source(campaign()), "", 0)) { check(prepared.exists(root.toFile())); }
            noStaging(root);
            // A failed publish restores the previous directory.
            Files.writeString(installed.resolve("old-marker"), "previous");
            try (MccPackageImporter.Prepared prepared = MccPackageImporter.prepare(root.toFile(), source(campaign()), "", 0)) {
                delete(prepared.content);
                refuses(() -> prepared.install(root.toFile(), true));
                check(Files.readString(installed.resolve("old-marker")).equals("previous"));
            }
            noStaging(root);
            // Recover an interrupted replacement before accepting another import.
            try (MccPackageImporter.Prepared prepared = MccPackageImporter.prepare(root.toFile(), source(campaign()), "", 0)) {
                Files.writeString(prepared.staging.resolve("destination"), prepared.destination);
                Files.move(installed, prepared.staging.resolve("previous"));
            }
            check(!Files.exists(installed));
            try (MccPackageImporter.Prepared prepared = MccPackageImporter.prepare(root.toFile(), source(campaign()), "", 0)) {
                check(prepared.exists(root.toFile()));
                check(Files.readString(installed.resolve("old-marker")).equals("previous"));
            }
            noStaging(root);
            for (String bad : new String[]{"../escape.map", "/escape.map", "C:/escape.map", "a/../escape.map", "a\\..\\escape.map", "a//file.map", "a/file.map:stream"}) {
                Map<String,byte[]> input = campaign(); input.put(bad, map(0));
                refuses(() -> MccPackageImporter.prepare(root.toFile(), zip(input), "", 0)); noStaging(root);
            }
            for (String fault : new String[]{"duplicate", "missing", "type", "kind", "null-kind", "malformed", "oversized", "engine", "header", "duplicate-mission"}) {
                Map<String,byte[]> input = campaign();
                if (fault.equals("duplicate")) input.put("2915693904/maps/A10.MAP", map(0));
                if (fault.equals("missing")) input.remove("2915693904/Maps/a10.map");
                if (fault.equals("type")) input.put("2915693904/Maps/a10.map", map(1));
                if (fault.equals("kind")) input.put("2915693904/campaign/a10.json", bytes("{\"ScenarioFile\":\"a10\",\"CampaignMapKind\":\"Hub\"}"));
                if (fault.equals("null-kind")) input.put("2915693904/campaign/a10.json", bytes("{\"ScenarioFile\":\"a10\",\"CampaignMapKind\":null}"));
                if (fault.equals("malformed")) input.put("2915693904/ModInfo.json", bytes("{broken}"));
                if (fault.equals("oversized")) input.put("2915693904/ModInfo.json", new byte[262145]);
                if (fault.equals("engine")) input.put("2915693904/ModInfo.json", bytes("{\"Engine\":\"Halo2\"}"));
                if (fault.equals("header")) input.put("2915693904/Maps/a10.map", new byte[2088]);
                if (fault.equals("duplicate-mission")) input.put("2915693904/campaign/a10.json", bytes("{\"ScenarioFile\":\"z10\"}"));
                refuses(() -> MccPackageImporter.prepare(root.toFile(), source(input), "", 0)); noStaging(root);
            }
            refuses(() -> MccPackageImporter.prepare(root.toFile(), receiver -> { receiver.file("partial.map", new ByteArrayInputStream(map(0))); throw new IOException("provider disconnected"); }, "", 0));
            noStaging(root);
            Map<String,byte[]> resources = new LinkedHashMap<>(); resources.put("bitmaps.map", new byte[32]);
            try (MccPackageImporter.Prepared prepared = MccPackageImporter.prepare(root.toFile(), source(resources), "", 1)) { prepared.install(root.toFile(), false); }
            check(Files.exists(root.resolve("mcc_maps/_shared/singleplayer/bitmaps.map")));
            refuses(() -> MccPackageImporter.prepare(root.toFile(), source(resources), "", 2));
            resources.put("sounds.map", new byte[32]);
            try (MccPackageImporter.Prepared prepared = MccPackageImporter.prepare(root.toFile(), source(resources), "", 2)) { prepared.install(root.toFile(), false); }
            check(Files.exists(root.resolve("mcc_maps/_shared/multiplayer/sounds.map")));
            check(Files.readString(root.resolve("maps/ui.map")).equals("Xbox stays"));
            check(Files.readString(root.resolve("custom_maps/example.map")).equals("CE stays"));
            check(Files.readString(root.resolve("saves/savegame.bin")).equals("save stays"));
            for (String bad : new String[]{"{} trailing", "{\"x\":1,\"x\":2}", "[1,]", "[01]", "\"\\u0000\"", "[".repeat(18) + "0" + "]".repeat(18)}) refuses(() -> MccJson.parse(bad));
            Object largeId = MccJson.member(MccJson.parse("{\"id\":18446744073709551615}"), "id");
            check(((MccJson.Number)largeId).text.equals("18446744073709551615"));
            noStaging(root);
            System.out.println("MCC importer: " + checks + " checks passed");
        } finally { delete(root); }
    }
    static void delete(Path path) throws IOException {
        if (!Files.exists(path)) return;
        if (Files.isDirectory(path)) try (java.nio.file.DirectoryStream<Path> entries = Files.newDirectoryStream(path)) { for (Path entry : entries) delete(entry); }
        Files.delete(path);
    }
}
