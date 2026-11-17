"""The engine's own vector and frame maths, extracted for the fixtures that compile riding code against it."""

import re

from harness import function, inline, read, structure


def without_includes(text):
    return re.sub(r"^#include .*$", "", text, flags=re.M)


NL = chr(10)
REAL_MATH_INLINES = ["square_root", "arctangent", "set_real_point3d", "vector_from_points3d", "scale_vector3d",
                     "magnitude_squared3d", "magnitude3d", "normalize3d", "distance_squared3d", "distance3d",
                     "dot_product3d", "cross_product3d", "add_vectors3d", "subtract_vectors3d", "negate_vector3d",
                     "scale_vector2d", "magnitude_squared2d", "magnitude2d", "normalize2d", "point_from_line3d",
                      "valid_real", "valid_real_point3d", "valid_real_vector3d", "reciprocal_square_root", "set_real_quaternion"]
MATRIX_FUNCTIONS = ["matrix4x3_inverse", "matrix4x3_multiply", "matrix4x3_rotation_to_quaternion",
                    "matrix4x3_rotation_from_vectors", "matrix4x3_from_point_and_vectors", "matrix4x3_transform_point",
                    "matrix4x3_inverse_transform_point", "vector_from_matrices4x3", "matrix4x3_rotation_from_quaternion"]


def engine():
    """The engine's own vector and frame maths, extracted: the code under test calls it and does not copy it."""
    real_math = read("source/math/real_math.h")
    matrix_math = read("source/math/matrix_math.c")
    text = "".join(re.search(r"^#define " + name + r".*$", real_math, re.M).group(0) + NL
                   for name in ("_real_epsilon", "_pi"))
    for name in ("real_quaternion", "real_matrix4x3", "real_plane3d"):
        text += structure(real_math, name) + NL + f"typedef struct {name} {name};" + NL
    text += "".join(inline(real_math, name) + NL for name in REAL_MATH_INLINES)
    text += function(read("source/math/real_math.c"), "quaternion_to_angle_and_vector") + NL
    text += function(read("source/math/real_math.c"), "quaternion_normalize") + NL
    # (the 4x3 next-index table: one blob in the original, matrix_math.c)
    text += re.search(r"^static short data_0030790c.*$", matrix_math, re.M).group(0) + NL
    text += "#define matrix4x3_next (data_0030790c)" + NL
    text += "".join(function(matrix_math, name) + NL for name in MATRIX_FUNCTIONS)
    return text
