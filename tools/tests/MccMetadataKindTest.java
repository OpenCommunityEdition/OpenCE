package com.halo.decomp;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import java.io.ByteArrayOutputStream;

public final class MccMetadataKindTest {
    static byte[] bytes(String text) { return text.getBytes(StandardCharsets.UTF_8); }
    static byte[] cache() {
        byte[] bytes = new byte[2088];
        ByteBuffer h = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
        h.putInt(0, 0x68656164); h.putInt(4, 13); h.putInt(8, bytes.length);
        h.putInt(16, 2048); h.putInt(20, 40); h.putInt(0x7fc, 0x666f6f74);
        return bytes;
    }
    static void check(boolean value) { if (!value) throw new AssertionError(); }
    static void run(Path root, String kind, boolean accepted, boolean zip) throws Exception {
        Files.createDirectories(root);
        String raw = "{\"ScenarioFile\":\"second\"" +
            (kind == null ? "" : ",\"CampaignMapKind\":" + kind) + "}";
        Map<String,byte[]> files = new LinkedHashMap<>();
        files.put("ModInfo.json", bytes("{\"Engine\":\"Halo1\",\"GameModContents\":{\"HasCampaign\":true}}"));
        files.put("CampaignInfo.json", bytes("{\"CampaignMaps\":[\"first.json\",\"second.json\"]}"));
        files.put("first.json", bytes("{\"ScenarioFile\":\"first\",\"CampaignMapKind\":\"Default\"}"));
        files.put("second.json", bytes(raw));
        files.put("maps/first.map", cache()); files.put("maps/second.map", cache());
        MccPackageImporter.Source source = receiver -> {
            for (Map.Entry<String,byte[]> file : files.entrySet())
                receiver.file(file.getKey(), new ByteArrayInputStream(file.getValue()));
        };
        if (zip) {
            ByteArrayOutputStream output = new ByteArrayOutputStream();
            try (ZipOutputStream archive = new ZipOutputStream(output)) {
                for (Map.Entry<String,byte[]> file : files.entrySet()) {
                    archive.putNextEntry(new ZipEntry(file.getKey())); archive.write(file.getValue()); archive.closeEntry();
                }
            }
            source = MccPackageImporter.zip(new ByteArrayInputStream(output.toByteArray()));
        }
        try (MccPackageImporter.Prepared prepared = MccPackageImporter.prepare(root.toFile(), source, "Example", MccPackageImporter.PACKAGE)) {
            check(accepted); check(prepared.maps == 2);
            prepared.install(root.toFile(), false);
            Path installed = root.resolve("mcc_maps").resolve(prepared.destination);
            check(Files.readString(installed.resolve("second.json")).equals(raw));
            check(Files.readString(installed.resolve("campaigninfo.json")).equals(
                new String(files.get("CampaignInfo.json"), StandardCharsets.UTF_8)));
        } catch (IOException failure) {
            if (accepted) throw failure;
            check(failure.getMessage().contains("Unsupported campaign mission kind"));
        }
        try (java.util.stream.Stream<Path> children = Files.list(root)) {
            check(children.noneMatch(p -> p.getFileName().toString().startsWith(".mcc-import-")));
        }
    }
    public static void main(String[] args) throws Exception {
        int index = 0;
        for (boolean zip : new boolean[]{false, true}) {
            for (String kind : new String[]{"\"Default\"", "\"default\"", "\"DEFAULT\"", "\"dEfAuLt\"", null})
                run(Path.of(args[0], "case" + index++), kind, true, zip);
            for (String kind : new String[]{"\"Hub\"", "\"hub\"", "\"Flashback\"", "\"Cutscene\"", "\"\"",
                    "\" Default\"", "\"Default \"", "\"Default/\"", "\"defau\\u0131t\"", "0", "true", "null", "[]", "{}"})
                run(Path.of(args[0], "case" + index++), kind, false, zip);
        }
        System.out.println(index + " folder/ZIP metadata cases passed");
    }
}
