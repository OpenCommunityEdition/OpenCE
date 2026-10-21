//! Little-endian reads that check every range.

use std::fmt;

/// Why a read failed.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Fault {
    /// The range lies outside the bytes read from.
    OutOfRange {
        /// Its start.
        at: u64,
        /// Its size.
        size: u64,
        /// How many bytes there are.
        length: u64,
    },
    /// A string has no terminator within its longest length.
    Unterminated,
    /// A string holds a byte that is not printable ASCII.
    NotText,
}

impl fmt::Display for Fault {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Self::OutOfRange { at, size, length } => write!(
                f,
                "({size:#x} bytes at {at:#x}) lies outside the {length:#x} bytes it is in"
            ),
            Self::Unterminated => f.write_str("is not terminated"),
            Self::NotText => f.write_str("is not text"),
        }
    }
}

/// `size` bytes at `at`.
pub fn slice(bytes: &[u8], at: usize, size: usize) -> Result<&[u8], Fault> {
    at.checked_add(size)
        .and_then(|end| bytes.get(at..end))
        .ok_or(Fault::OutOfRange {
            at: at as u64,
            size: size as u64,
            length: bytes.len() as u64,
        })
}

/// `N` bytes at `at`.
pub fn array<const N: usize>(bytes: &[u8], at: usize) -> Result<[u8; N], Fault> {
    let mut array = [0; N];
    array.copy_from_slice(slice(bytes, at, N)?);
    Ok(array)
}

/// A `u16` at `at`.
pub fn u16(bytes: &[u8], at: usize) -> Result<u16, Fault> {
    array(bytes, at).map(u16::from_le_bytes)
}

/// An `i16` at `at`.
pub fn i16(bytes: &[u8], at: usize) -> Result<i16, Fault> {
    array(bytes, at).map(i16::from_le_bytes)
}

/// A `u32` at `at`.
pub fn u32(bytes: &[u8], at: usize) -> Result<u32, Fault> {
    array(bytes, at).map(u32::from_le_bytes)
}

/// An `i32` at `at`.
pub fn i32(bytes: &[u8], at: usize) -> Result<i32, Fault> {
    array(bytes, at).map(i32::from_le_bytes)
}

/// An `f32` at `at`.
pub fn f32(bytes: &[u8], at: usize) -> Result<f32, Fault> {
    array(bytes, at).map(f32::from_le_bytes)
}

/// A string at `at` of at most `longest` bytes before its terminator,
/// which must be within the bytes.
pub fn text(bytes: &[u8], at: usize, longest: usize) -> Result<&str, Fault> {
    let rest = bytes.get(at..).ok_or(Fault::OutOfRange {
        at: at as u64,
        size: 1,
        length: bytes.len() as u64,
    })?;
    let window = &rest[..rest.len().min(longest.saturating_add(1))];
    let end = window
        .iter()
        .position(|&byte| byte == 0)
        .ok_or(Fault::Unterminated)?;
    let text = &window[..end];
    if !text.iter().all(|&byte| (0x20..0x7f).contains(&byte)) {
        return Err(Fault::NotText);
    }
    // (printable ASCII is UTF-8)
    std::str::from_utf8(text).map_err(|_| Fault::NotText)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn reads_are_little_endian() {
        let bytes = [0x78, 0x56, 0x34, 0x12, 0x00, 0x00, 0x80, 0x3f];
        assert_eq!(u32(&bytes, 0), Ok(0x1234_5678));
        assert_eq!(u16(&bytes, 2), Ok(0x1234));
        assert_eq!(f32(&bytes, 4), Ok(1.0));
    }

    #[test]
    fn ranges_past_the_end_are_refused() {
        let bytes = [0u8; 8];
        assert!(u32(&bytes, 5).is_err());
        assert!(u32(&bytes, usize::MAX).is_err());
        assert!(slice(&bytes, 4, usize::MAX).is_err());
        assert_eq!(slice(&bytes, 8, 0), Ok(&[][..]));
        assert_eq!(
            u32(&bytes, 6)
                .err()
                .map(|fault| fault.to_string())
                .as_deref(),
            Some("(0x4 bytes at 0x6) lies outside the 0x8 bytes it is in")
        );
    }

    #[test]
    fn text_needs_its_terminator() {
        assert_eq!(text(b"abc\0def", 0, 255), Ok("abc"));
        assert_eq!(text(b"abc\0", 0, 3), Ok("abc"));
        assert_eq!(text(b"abcd\0", 0, 3), Err(Fault::Unterminated));
        assert_eq!(text(b"abc", 0, 255), Err(Fault::Unterminated));
        assert_eq!(text(b"a\x01\0", 0, 255), Err(Fault::NotText));
        assert!(text(b"abc", 9, 255).is_err());
    }
}
