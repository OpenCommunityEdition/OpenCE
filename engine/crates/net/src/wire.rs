//! Bounds-checked little-endian reading and size-limited writing.

use crate::{DecodeError, EncodeError, MAX_PACKET_BYTES};

/// Reads a packet's bytes, refusing to read past its end.
pub(crate) struct Reader<'a> {
    bytes: &'a [u8],
    at: usize,
}

impl<'a> Reader<'a> {
    pub(crate) fn new(bytes: &'a [u8]) -> Self {
        Self { bytes, at: 0 }
    }

    pub(crate) fn take(&mut self, count: usize) -> Result<&'a [u8], DecodeError> {
        let end = self.at.checked_add(count).ok_or(DecodeError::Truncated)?;
        let taken = self.bytes.get(self.at..end).ok_or(DecodeError::Truncated)?;
        self.at = end;
        Ok(taken)
    }

    fn array<const N: usize>(&mut self) -> Result<[u8; N], DecodeError> {
        let mut out = [0; N];
        out.copy_from_slice(self.take(N)?);
        Ok(out)
    }

    pub(crate) fn u8(&mut self) -> Result<u8, DecodeError> {
        Ok(self.array::<1>()?[0])
    }

    pub(crate) fn u16(&mut self) -> Result<u16, DecodeError> {
        Ok(u16::from_le_bytes(self.array()?))
    }

    pub(crate) fn i16(&mut self) -> Result<i16, DecodeError> {
        Ok(i16::from_le_bytes(self.array()?))
    }

    pub(crate) fn u32(&mut self) -> Result<u32, DecodeError> {
        Ok(u32::from_le_bytes(self.array()?))
    }

    /// A finite number.
    pub(crate) fn f32(&mut self) -> Result<f32, DecodeError> {
        let value = f32::from_le_bytes(self.array()?);
        if value.is_finite() {
            Ok(value)
        } else {
            Err(DecodeError::BadValue)
        }
    }

    pub(crate) fn f32s<const N: usize>(&mut self) -> Result<[f32; N], DecodeError> {
        let mut out = [0.0; N];
        for value in &mut out {
            *value = self.f32()?;
        }
        Ok(out)
    }

    /// A count, at most `max`.
    pub(crate) fn count(&mut self, max: usize) -> Result<usize, DecodeError> {
        let count = usize::from(self.u16()?);
        if count > max {
            return Err(DecodeError::BadCount);
        }
        Ok(count)
    }

    /// UTF-8 text of at most `max` bytes.
    pub(crate) fn text(&mut self, max: usize) -> Result<String, DecodeError> {
        let count = usize::from(self.u8()?);
        if count > max {
            return Err(DecodeError::BadCount);
        }
        let bytes = self.take(count)?;
        String::from_utf8(bytes.to_vec()).map_err(|_| DecodeError::BadValue)
    }

    pub(crate) fn finish(&self) -> Result<(), DecodeError> {
        if self.at == self.bytes.len() {
            Ok(())
        } else {
            Err(DecodeError::TrailingBytes)
        }
    }
}

/// Writes a packet's bytes.
#[derive(Default)]
pub(crate) struct Writer {
    pub(crate) bytes: Vec<u8>,
}

impl Writer {
    pub(crate) fn put(&mut self, bytes: &[u8]) {
        self.bytes.extend_from_slice(bytes);
    }

    pub(crate) fn u8(&mut self, value: u8) {
        self.bytes.push(value);
    }

    pub(crate) fn u16(&mut self, value: u16) {
        self.put(&value.to_le_bytes());
    }

    pub(crate) fn i16(&mut self, value: i16) {
        self.put(&value.to_le_bytes());
    }

    pub(crate) fn u32(&mut self, value: u32) {
        self.put(&value.to_le_bytes());
    }

    pub(crate) fn f32(&mut self, value: f32) {
        self.put(&value.to_le_bytes());
    }

    pub(crate) fn f32s(&mut self, values: &[f32]) {
        for &value in values {
            self.f32(value);
        }
    }

    pub(crate) fn count(&mut self, count: usize, max: usize) -> Result<(), EncodeError> {
        if count > max {
            return Err(EncodeError::BadCount);
        }
        self.u16(u16::try_from(count).map_err(|_| EncodeError::BadCount)?);
        Ok(())
    }

    pub(crate) fn text(&mut self, text: &str, max: usize) -> Result<(), EncodeError> {
        let count = u8::try_from(text.len()).map_err(|_| EncodeError::BadCount)?;
        if text.len() > max {
            return Err(EncodeError::BadCount);
        }
        self.u8(count);
        self.put(text.as_bytes());
        Ok(())
    }

    pub(crate) fn finish(self) -> Result<Vec<u8>, EncodeError> {
        if self.bytes.len() > MAX_PACKET_BYTES {
            return Err(EncodeError::TooLarge);
        }
        Ok(self.bytes)
    }
}
