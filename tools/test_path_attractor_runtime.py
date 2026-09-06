"""Fixture-free checks for the bounded, explicitly fuzzy Path attractor probe."""
import math

from tools.audit import path_attractor_runtime as oracle


def test_corpus_covers_projection_axes_radius_sign_and_assert():
    cases = oracle.cases()
    assert len(cases) == 103
    assert len({(case['name'], case['entry']) for case in cases}) == len(cases)
    assert sum(case['entry'] == oracle.CLOSEST for case in cases) == 17
    assert {case['radius'] for case in cases} == {-4., 0., 1., 2., 4.}
    assert sum(case['null'] for case in cases) == 1


def test_original_signed_projection_is_not_conventional_closest_point():
    assert oracle.projected((0., 0., 0.), (2., 0., 0.), (-1., 1., 0.)) == (1., 0., 0.)
    assert oracle.projected((0., 0., 0.), (2., 0., 0.), (1., 1., 0.)) == (2., 0., 0.)


def test_parameter_endpoints_and_outside_fallback():
    p0, p1 = (0., 0., 0.), (2., 0., 0.)
    assert oracle.projected(p0, p1, (0., 1., 0.)) == p0
    assert oracle.projected(p0, p1, (-2., 1., 0.)) == p1
    assert oracle.projected(p0, p1, (-3., 1., 0.)) == p1


def test_degenerate_segment_is_not_silently_fixed():
    point = (1., 2., 3.)
    assert all(math.isnan(value) for value in oracle.projected(point, point, (0., 0., 0.)))


def test_translated_oblique_probe_is_exactly_representable():
    assert oracle.projected((1., 2., 3.), (3., 4., 5.), (0., 1., 2.)) == (2., 3., 4.)


def test_literals_bind_the_genuine_assert_contract():
    assert oracle.LITERALS[oracle.ASSERT_SYMBOL][1] == b'distance_reference\0'
    assert oracle.LITERALS[oracle.FILE_SYMBOL][1] == b'c:\\halo\\SOURCE\\ai\\path.c\0'
