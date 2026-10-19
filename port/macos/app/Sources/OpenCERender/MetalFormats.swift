// The render commands' values (OpenCERuntime's, from opence_render.h) as
// Metal's.

import Metal
import OpenCERuntime

extension PixelFormat {
    var metal: MTLPixelFormat {
        switch self {
        case .rgba8: .rgba8Unorm
        case .bgra8: .bgra8Unorm
        case .r8: .r8Unorm
        case .rg8: .rg8Unorm
        case .b5g6r5: .b5g6r5Unorm
        case .bgr5a1: .bgr5A1Unorm
        case .abgr4: .abgr4Unorm
        case .bc1: .bc1_rgba
        case .bc2: .bc2_rgba
        case .bc3: .bc3_rgba
        case .rgba16f: .rgba16Float
        case .depth32f: .depth32Float
        case .depth32fStencil8: .depth32Float_stencil8
        }
    }

    var hasStencil: Bool { self == .depth32fStencil8 }

    /// Whether Metal can make its mipmaps (not the compressed formats', nor
    /// depth's).
    var generatesMipmaps: Bool {
        switch self {
        case .bc1, .bc2, .bc3, .depth32f, .depth32fStencil8: false
        default: true
        }
    }
}

extension VertexFormat {
    var metal: MTLVertexFormat {
        switch self {
        case .float: .float
        case .float2: .float2
        case .float3: .float3
        case .float4: .float4
        case .uchar4Normalized: .uchar4Normalized
        case .uchar4NormalizedBgra: .uchar4Normalized_bgra
        case .short2: .short2
        case .short4: .short4
        case .short2Normalized: .short2Normalized
        case .short4Normalized: .short4Normalized
        case .uchar4: .uchar4
        case .uint: .uint
        }
    }
}

extension BlendFactor {
    var metal: MTLBlendFactor {
        switch self {
        case .zero: .zero
        case .one: .one
        case .sourceColor: .sourceColor
        case .oneMinusSourceColor: .oneMinusSourceColor
        case .sourceAlpha: .sourceAlpha
        case .oneMinusSourceAlpha: .oneMinusSourceAlpha
        case .destinationColor: .destinationColor
        case .oneMinusDestinationColor: .oneMinusDestinationColor
        case .destinationAlpha: .destinationAlpha
        case .oneMinusDestinationAlpha: .oneMinusDestinationAlpha
        case .sourceAlphaSaturated: .sourceAlphaSaturated
        case .blendColor: .blendColor
        case .oneMinusBlendColor: .oneMinusBlendColor
        case .blendAlpha: .blendAlpha
        case .oneMinusBlendAlpha: .oneMinusBlendAlpha
        }
    }
}

extension BlendOperation {
    var metal: MTLBlendOperation {
        switch self {
        case .add: .add
        case .subtract: .subtract
        case .reverseSubtract: .reverseSubtract
        case .min: .min
        case .max: .max
        }
    }
}

/// OPENCE_RENDER_WRITE_*: red 1, green 2, blue 4, alpha 8 (Metal's own bits
/// are the other way round).
func colorWriteMask(_ mask: UInt32) -> MTLColorWriteMask {
    var metal: MTLColorWriteMask = []
    if mask & 1 != 0 { metal.insert(.red) }
    if mask & 2 != 0 { metal.insert(.green) }
    if mask & 4 != 0 { metal.insert(.blue) }
    if mask & 8 != 0 { metal.insert(.alpha) }
    return metal
}

extension Compare {
    var metal: MTLCompareFunction {
        switch self {
        case .never: .never
        case .less: .less
        case .equal: .equal
        case .lessEqual: .lessEqual
        case .greater: .greater
        case .notEqual: .notEqual
        case .greaterEqual: .greaterEqual
        case .always: .always
        }
    }
}

extension StencilOperation {
    var metal: MTLStencilOperation {
        switch self {
        case .keep: .keep
        case .zero: .zero
        case .replace: .replace
        case .incrementClamp: .incrementClamp
        case .decrementClamp: .decrementClamp
        case .invert: .invert
        case .incrementWrap: .incrementWrap
        case .decrementWrap: .decrementWrap
        }
    }
}

extension StencilFace {
    var metal: MTLStencilDescriptor {
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilFailureOperation = fail.metal
        descriptor.depthFailureOperation = depthFail.metal
        descriptor.depthStencilPassOperation = pass.metal
        descriptor.stencilCompareFunction = compare.metal
        descriptor.readMask = readMask
        descriptor.writeMask = writeMask
        return descriptor
    }
}

extension Filter {
    /// (none is for mipmaps only; the validator allows it nowhere else)
    var metal: MTLSamplerMinMagFilter { self == .linear ? .linear : .nearest }

    var metalMip: MTLSamplerMipFilter {
        switch self {
        case .none: .notMipmapped
        case .nearest: .nearest
        case .linear: .linear
        }
    }
}

extension Address {
    var metal: MTLSamplerAddressMode {
        switch self {
        case .repeat: .repeat
        case .mirror: .mirrorRepeat
        case .clampToEdge: .clampToEdge
        case .clampToBorder: .clampToBorderColor
        }
    }
}

extension Border {
    var metal: MTLSamplerBorderColor {
        switch self {
        case .transparentBlack: .transparentBlack
        case .opaqueBlack: .opaqueBlack
        case .opaqueWhite: .opaqueWhite
        }
    }
}

extension Load {
    var metal: MTLLoadAction {
        switch self {
        case .dontCare: .dontCare
        case .load: .load
        case .clear: .clear
        }
    }
}

extension Cull {
    var metal: MTLCullMode {
        switch self {
        case .none: .none
        case .front: .front
        case .back: .back
        }
    }
}

extension Winding {
    var metal: MTLWinding { self == .clockwise ? .clockwise : .counterClockwise }
}

extension Fill {
    var metal: MTLTriangleFillMode { self == .solid ? .fill : .lines }
}

extension Primitive {
    var metal: MTLPrimitiveType {
        switch self {
        case .point: .point
        case .line: .line
        case .lineStrip: .lineStrip
        case .triangle: .triangle
        case .triangleStrip: .triangleStrip
        }
    }
}

extension IndexType {
    var metal: MTLIndexType { self == .uint16 ? .uint16 : .uint32 }
}

extension Visibility {
    var metal: MTLVisibilityResultMode {
        switch self {
        case .disabled: .disabled
        case .boolean: .boolean
        case .counting: .counting
        }
    }
}
