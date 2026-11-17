package com.halo.decomp;

import java.io.IOException;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/* Small bounded metadata reader. Numbers retain their spelling so Workshop
 * IDs never pass through floating point. No Android dependency: JVM tests
 * exercise the same parser used by the document importer. */
final class MccJson {
    static final class Number {
        final String text;
        Number(String text) { this.text = text; }
    }
    private final String text;
    private int at, tokens;
    private MccJson(String text) { this.text = text; }
    static Object parse(String text) throws IOException {
        if (text.length() > 262144) throw new IOException("Metadata exceeds 256 KiB");
        MccJson parser = new MccJson(text);
        Object value = parser.value(0);
        parser.space();
        if (parser.at != text.length()) throw new IOException("Trailing JSON data");
        return value;
    }
    private void space() {
        while (at < text.length() && " \r\n\t".indexOf(text.charAt(at)) >= 0) at++;
    }
    private boolean take(char c) {
        space();
        if (at < text.length() && text.charAt(at) == c) { at++; return true; }
        return false;
    }
    private void need(char c) throws IOException {
        if (!take(c)) throw new IOException("Invalid JSON near character " + at);
    }
    private Object value(int depth) throws IOException {
        if (depth > 16 || ++tokens > 32768) throw new IOException("Metadata nesting or token limit exceeded");
        space();
        if (at == text.length()) throw new IOException("Incomplete JSON");
        char c = text.charAt(at);
        if (c == '"') return string();
        if (take('{')) {
            Map<String,Object> result = new LinkedHashMap<>();
            if (take('}')) return result;
            do {
                space();
                String key = string();
                need(':');
                if (result.containsKey(key)) throw new IOException("Duplicate metadata key: " + key);
                result.put(key, value(depth + 1));
                if (take('}')) return result;
                need(',');
            } while (true);
        }
        if (take('[')) {
            List<Object> result = new ArrayList<>();
            if (take(']')) return result;
            do {
                result.add(value(depth + 1));
                if (take(']')) return result;
                need(',');
            } while (true);
        }
        int start = at;
        while (at < text.length() && ",]} \r\n\t".indexOf(text.charAt(at)) < 0) at++;
        String word = text.substring(start, at);
        if (word.equals("true")) return Boolean.TRUE;
        if (word.equals("false")) return Boolean.FALSE;
        if (word.equals("null")) return null;
        if (word.matches("-?(0|[1-9][0-9]*)(\\.[0-9]+)?([eE][+-]?[0-9]+)?")) return new Number(word);
        throw new IOException("Invalid JSON value");
    }
    private String string() throws IOException {
        need('"');
        StringBuilder result = new StringBuilder();
        while (at < text.length()) {
            char c = text.charAt(at++);
            if (c == '"') return result.toString();
            if (c < 32) throw new IOException("Control character in JSON string");
            if (c == '\\') {
                if (at == text.length()) break;
                c = text.charAt(at++);
                switch (c) {
                case '"': case '\\': case '/': break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case 'u':
                    if (at + 4 > text.length()) throw new IOException("Incomplete Unicode escape");
                    int code = 0;
                    for (int i = 0; i < 4; i++) {
                        int digit = Character.digit(text.charAt(at++), 16);
                        if (digit < 0) throw new IOException("Invalid Unicode escape");
                        code = code * 16 + digit;
                    }
                    c = (char) code;
                    if (c == 0) throw new IOException("NUL in metadata");
                    break;
                default: throw new IOException("Invalid JSON escape");
                }
            }
            result.append(c);
        }
        throw new IOException("Unterminated JSON string");
    }
    static Object member(Object object, String key) {
        return object instanceof Map ? ((Map<?,?>)object).get(key) : null;
    }
    static String string(Object object) { return object instanceof String ? (String)object : ""; }
}
