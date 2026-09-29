#!/usr/bin/env node
// Compare observable draw inputs under immediate GL and deferred stream batching.
// Run from any directory: node tools/test_stream_batch.mjs
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import vm from "node:vm";

const source = readFileSync(new URL("../port/web/stream-batch.js", import.meta.url), "utf8");
const vertexCapacity = 16 * 1024 * 1024;
const indexCapacity = 2 * 1024 * 1024;

class MockGL {
    constructor() {
        Object.assign(this, { ARRAY_BUFFER: 34962, ELEMENT_ARRAY_BUFFER: 34963, STREAM_DRAW: 35040,
            DYNAMIC_DRAW: 35048, FLOAT: 5126, UNSIGNED_BYTE: 5121, UNSIGNED_SHORT: 5123,
            TRIANGLES: 4, COLOR: 6144, COPY_READ_BUFFER: 36662, COPY_WRITE_BUFFER: 36663, FRAMEBUFFER: 36160, DRAW_FRAMEBUFFER: 36009, READ_FRAMEBUFFER: 36008, TRANSFORM_FEEDBACK_BUFFER: 35982, COLOR_BUFFER_BIT: 16384, NEAREST: 9728 });
        this.bindings = new Map();
        this.arrays = new Map([[null, { element: null, attributes: new Map() }]]);
        this.vao = null;
        this.draws = [];
        this.uploads = 0;
        this.color = [];
        this.trace = [];
        this.drawFramebuffer = null;
        this.presented = [];
        this.feedback = null;
        this.feedbackActive = false;
    }
    createBuffer() { return { bytes: new Uint8Array(0) }; }
    createFramebuffer() { return {}; }
    bindFramebuffer(target, framebuffer) { if (target === this.FRAMEBUFFER || target === this.DRAW_FRAMEBUFFER) this.drawFramebuffer = framebuffer; }
    deleteFramebuffer(framebuffer) { if (framebuffer && this.drawFramebuffer === framebuffer) this.drawFramebuffer = null; }
    blitFramebuffer() { if (this.drawFramebuffer === null) this.presented.push(this.draws.slice()); }
    bindBufferRange(target, index, buffer, offset, size) {
        this.bindings.set(target, buffer);
        if (target === this.TRANSFORM_FEEDBACK_BUFFER) this.feedback = { buffer, offset };
    }
    beginTransformFeedback() { this.feedbackActive = true; }
    endTransformFeedback() { this.feedbackActive = false; }
    createVertexArray() { const value = {}; this.arrays.set(value, { element: null, attributes: new Map() }); return value; }
    bindVertexArray(value) { this.vao = value; }
    bindBuffer(target, buffer) {
        if (target === this.ELEMENT_ARRAY_BUFFER) this.arrays.get(this.vao).element = buffer;
        else this.bindings.set(target, buffer);
    }
    bufferData(target, value) {
        const buffer = target === this.ELEMENT_ARRAY_BUFFER ? this.arrays.get(this.vao).element : this.bindings.get(target);
        buffer.bytes = typeof value === "number" ? new Uint8Array(value) : new Uint8Array(value.buffer, value.byteOffset, value.byteLength).slice();
    }
    bufferSubData(target, offset, value, sourceOffset = 0, length = 0) {
        const buffer = target === this.ELEMENT_ARRAY_BUFFER ? this.arrays.get(this.vao).element : this.bindings.get(target);
        const unit = value.BYTES_PER_ELEMENT || 1;
        buffer.bytes.set(new Uint8Array(value.buffer, value.byteOffset + sourceOffset * unit, (length || value.byteLength / unit - sourceOffset) * unit), offset);
        this.uploads++;
    }
    enableVertexAttribArray(index) { this.arrays.get(this.vao).attributes.get(index).enabled = true; }
    vertexAttribPointer(index, size, type, normalized, stride, offset) {
        this.arrays.get(this.vao).attributes.set(index, { buffer: this.bindings.get(this.ARRAY_BUFFER), size, type, stride: stride || size, offset, enabled: true });
    }
    uniform4fv(location, data, offset = 0, length = 0) { if (location !== null) this.color = Array.from(data).slice(offset, offset + (length || data.length - offset)); }
    uniform1f(location, value) { if (location !== null) this.color = [value]; }
    drawArrays(mode, first, count) { this.capture(Array.from({ length: count }, (_, i) => first + i)); }
    drawElements(mode, count, type, offset) {
        const bytes = this.arrays.get(this.vao).element.bytes;
        const indices = Array.from({ length: count }, (_, i) => type === this.UNSIGNED_BYTE ? bytes[offset + i] : new DataView(bytes.buffer).getUint16(offset + i * 2, true));
        this.capture(indices);
    }
    capture(indices) {
        const attribute = this.arrays.get(this.vao).attributes.get(0);
        this.draws.push({ vertices: indices.map(i => attribute.buffer.bytes[attribute.offset + i * attribute.stride]), color: this.color.slice() });
        this.trace.push("draw");
        if (this.feedbackActive) this.feedback.buffer.bytes[this.feedback.offset] = 88;
    }
    getBufferSubData(target, offset, destination) {
        const buffer = target === this.ELEMENT_ARRAY_BUFFER ? this.arrays.get(this.vao).element : this.bindings.get(target);
        destination.set(buffer.bytes.subarray(offset, offset + destination.byteLength));
        this.trace.push("read");
    }
    copyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size) {
        this.bindings.get(writeTarget).bytes.set(this.bindings.get(readTarget).bytes.slice(readOffset, readOffset + size), writeOffset);
    }
    getError() { this.trace.push("error-check"); return 0; }
    deleteBuffer(buffer) {
        for (const [target, bound] of this.bindings) if (bound === buffer) this.bindings.set(target, null);
        if (this.arrays.get(this.vao).element === buffer) this.arrays.get(this.vao).element = null;
    }
    deleteVertexArray(array) { if (!array) return; if (this.vao === array) this.vao = null; this.arrays.delete(array); }
}

function environment(batched, search = "?batch_streams") {
    const gl = new MockGL();
    const canvas = { getContext: () => gl };
    const window = { requestAnimationFrame: () => 1 };
    if (batched) {
        vm.runInNewContext(source, { URLSearchParams, Uint8Array, ArrayBuffer,
            location: { search }, document: { getElementById: id => id === "canvas" ? canvas : null }, window,
            WebGLRenderingContext: { prototype: MockGL.prototype }, WebGL2RenderingContext: { prototype: {} }, setInterval() {} });
        canvas.getContext("webgl2");
    }
    return { gl, frame: () => window.requestAnimationFrame(() => {}) };
}

function vertexBuffer(gl, size = vertexCapacity, usage = gl.STREAM_DRAW) {
    const buffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
    gl.bufferData(gl.ARRAY_BUFFER, size, usage);
    gl.vertexAttribPointer(0, 1, gl.UNSIGNED_BYTE, false, 1, 0);
    gl.enableVertexAttribArray(0);
    return buffer;
}

function compare(name, sequence, expectation) {
    const original = environment(false), batched = environment(true);
    sequence(original.gl, original.frame);
    sequence(batched.gl, batched.frame);
    original.frame(); batched.frame();
    assert.deepEqual(batched.gl.draws, original.gl.draws, `${name}: changed geometry or uniforms`);
    expectation?.(original.gl, batched.gl);
    process.stdout.write(`PASS ${name}\n`);
}

compare("persistent appends retain per-draw vertices and uniform snapshots", (gl, frame) => {
    vertexBuffer(gl); frame(); // Allocation in an earlier flush, as in the real triple ring.
    const color = new Float32Array([1, 0, 0, 1]);
    gl.uniform4fv({}, color);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([10, 20, 30]));
    gl.drawArrays(gl.TRIANGLES, 0, 3);
    color.set([0, 1, 0, 1]); gl.uniform4fv({}, color);
    gl.bufferSubData(gl.ARRAY_BUFFER, 4, new Uint8Array([40, 50, 60]));
    gl.vertexAttribPointer(0, 1, gl.UNSIGNED_BYTE, false, 1, 4);
    gl.drawArrays(gl.TRIANGLES, 0, 3);
    color.fill(9);
}, (original, batched) => assert.equal(batched.uploads, original.uploads - 1));

compare("overwriting an already drawn range preserves old and new draws", gl => {
    vertexBuffer(gl);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([1, 2, 3])); gl.drawArrays(gl.TRIANGLES, 0, 3);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([7, 8, 9])); gl.drawArrays(gl.TRIANGLES, 0, 3);
}, (original, batched) => assert.equal(batched.uploads, original.uploads));

compare("readbacks observe all prior writes before later mutations", gl => {
    vertexBuffer(gl);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([1, 2, 3]));
    const read = new Uint8Array(3); gl.getBufferSubData(gl.ARRAY_BUFFER, 0, read); assert.deepEqual(Array.from(read), [1, 2, 3]);
    gl.bufferSubData(gl.ARRAY_BUFFER, 4, new Uint8Array([4, 5, 6])); gl.drawArrays(gl.TRIANGLES, 0, 3);
});

compare("existing bytes between later appends survive frame boundaries", (gl, frame) => {
    vertexBuffer(gl);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([10, 20, 30, 40, 50, 60, 70])); frame();
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([1])); gl.drawArrays(gl.TRIANGLES, 0, 3);
    gl.bufferSubData(gl.ARRAY_BUFFER, 6, new Uint8Array([9])); gl.drawArrays(gl.TRIANGLES, 4, 3);
});

compare("two VAOs retain independent index bindings and offsets", gl => {
    const first = gl.createVertexArray(), second = gl.createVertexArray();
    for (const [array, vertices] of [[first, [10, 20, 30]], [second, [40, 50, 60]]]) {
        gl.bindVertexArray(array); vertexBuffer(gl);
        gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array(vertices));
        const index = gl.createBuffer(); gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, index); gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, indexCapacity, gl.STREAM_DRAW);
        gl.bufferSubData(gl.ELEMENT_ARRAY_BUFFER, 0, new Uint16Array([2, 0, 1]));
    }
    gl.bindVertexArray(first); gl.drawElements(gl.TRIANGLES, 3, gl.UNSIGNED_SHORT, 0);
    gl.bindVertexArray(second); gl.drawElements(gl.TRIANGLES, 3, gl.UNSIGNED_SHORT, 0);
});

compare("unrelated dynamic buffers keep upload order", gl => {
    vertexBuffer(gl, 65536, gl.DYNAMIC_DRAW);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([1, 2, 3])); gl.drawArrays(gl.TRIANGLES, 0, 3);
    gl.bufferSubData(gl.ARRAY_BUFFER, 4, new Uint8Array([4, 5, 6])); gl.drawArrays(gl.TRIANGLES, 0, 3);
}, (original, batched) => assert.equal(batched.uploads, original.uploads));

compare("typed buffer replacement and heap-offset writes stay ordered", gl => {
    vertexBuffer(gl);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([0, 1, 2, 3, 4]), 1, 3); gl.drawArrays(gl.TRIANGLES, 0, 3);
    gl.bufferData(gl.ARRAY_BUFFER, new Uint8Array([7, 8, 9]), gl.STREAM_DRAW); gl.drawArrays(gl.TRIANGLES, 0, 3);
});

compare("deleting a ring before allocating its replacement releases bindings", gl => {
    const old = vertexBuffer(gl); gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([1, 2, 3])); gl.drawArrays(gl.TRIANGLES, 0, 3);
    gl.deleteBuffer(old); vertexBuffer(gl); gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([4, 5, 6])); gl.drawArrays(gl.TRIANGLES, 0, 3);
});

{
    const { gl, frame } = environment(true);
    let copied = 0;
    gl.uniform4fv(null, { length: 4, slice() { copied++; return [1, 2, 3, 4]; } });
    gl.uniform1f(null, 9); frame();
    assert.equal(copied, 0, "null uniform must not copy source data");
    assert.deepEqual(gl.color, []);
    process.stdout.write("PASS null uniforms skip unused copies and native writes\n");
}

compare("GPU buffer copies invalidate stale CPU snapshots", (gl, frame) => {
    const vertices = vertexBuffer(gl);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([10, 20, 30, 40, 50, 60, 70])); frame();
    const source = gl.createBuffer(); gl.bindBuffer(gl.COPY_READ_BUFFER, source);
    gl.bufferData(gl.COPY_READ_BUFFER, new Uint8Array([90, 91]), gl.STREAM_DRAW);
    gl.bindBuffer(gl.COPY_WRITE_BUFFER, vertices);
    gl.copyBufferSubData(gl.COPY_READ_BUFFER, gl.COPY_WRITE_BUFFER, 0, 2, 2);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([1])); gl.drawArrays(gl.TRIANGLES, 0, 3);
    gl.bufferSubData(gl.ARRAY_BUFFER, 6, new Uint8Array([9])); gl.drawArrays(gl.TRIANGLES, 4, 3);
});

for (const [query, expectedUploads] of [["", 1], ["?batch_streams=0", 2]]) {
    const { gl, frame } = environment(true, query);
    vertexBuffer(gl); frame();
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([1, 2, 3])); gl.drawArrays(gl.TRIANGLES, 0, 3);
    gl.bufferSubData(gl.ARRAY_BUFFER, 4, new Uint8Array([4, 5, 6])); frame();
    assert.equal(gl.uploads, expectedUploads);
}
process.stdout.write("PASS default fast path and explicit opt-out\n");

compare("transform-feedback binding separates pending uploads from GPU writes", (gl, frame) => {
    const vertices = vertexBuffer(gl);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120])); frame();
    const source = gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER, source);
    gl.bufferData(gl.ARRAY_BUFFER, new Uint8Array([5, 6, 7]), gl.STREAM_DRAW);
    gl.bindBuffer(gl.ARRAY_BUFFER, vertices);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([1]));
    gl.bindBufferRange(gl.TRANSFORM_FEEDBACK_BUFFER, 0, vertices, 8, 4);
    gl.bindBuffer(gl.ARRAY_BUFFER, source); gl.vertexAttribPointer(0, 1, gl.UNSIGNED_BYTE, false, 1, 0);
    gl.beginTransformFeedback(gl.TRIANGLES); gl.drawArrays(gl.TRIANGLES, 0, 3); gl.endTransformFeedback();
    gl.bindBuffer(gl.ARRAY_BUFFER, vertices);
    gl.bufferSubData(gl.ARRAY_BUFFER, 8, new Uint8Array([9]));
    gl.vertexAttribPointer(0, 1, gl.UNSIGNED_BYTE, false, 1, 6); gl.drawArrays(gl.TRIANGLES, 0, 3);
});

{
    const { gl } = environment(true);
    const offscreen = gl.createFramebuffer();
    vertexBuffer(gl);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Uint8Array([1, 2, 3])); gl.drawArrays(gl.TRIANGLES, 0, 3);
    gl.bindFramebuffer(gl.DRAW_FRAMEBUFFER, offscreen);
    gl.bindFramebuffer(gl.READ_FRAMEBUFFER, null);
    gl.blitFramebuffer(0, 0, 4, 4, 0, 0, 4, 4, gl.COLOR_BUFFER_BIT, gl.NEAREST);
    assert.equal(gl.draws.length, 0, "offscreen blits must retain batching even with default read framebuffer");
    gl.bindFramebuffer(gl.FRAMEBUFFER, null); gl.blitFramebuffer(0, 0, 4, 4, 0, 0, 4, 4, gl.COLOR_BUFFER_BIT, gl.NEAREST);
    assert.equal(gl.presented.length, 1, "canvas presentation must flush without requestAnimationFrame");
    assert.deepEqual(gl.presented[0][0].vertices, [1, 2, 3]);
    process.stdout.write("PASS presentation without RAF and offscreen blit batching\n");
}
