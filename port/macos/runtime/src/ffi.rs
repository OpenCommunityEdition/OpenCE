//! The C ABI the macOS host (port/macos/host) calls; cbindgen writes its
//! header (port/macos/runtime/cbindgen.toml, build/macos/runtime/opence_runtime.h).
//!
//! An [`OpenceGuest`] is the guest's memory with every region of the
//! standard plan mapped, its pool allocator, and, once an image is loaded,
//! the image with its page tables and the system registers the vCPUs start
//! with. Functions return an [`OpenceStatus`]; after a failure,
//! `opence_guest_last_error` gives the reason.
//!
//! `opence_guest_host_pointer`, `opence_guest_host_base` and
//! `opence_render_check` may be called from any thread at any time. The others change the guest and must not run
//! at the same time as each other: the host serialises them.

use std::ffi::{c_char, c_void, CStr, CString};
use std::ptr;
use std::sync::Mutex;

use crate::image::{check_imports, GuestHeader, GuestImage};
use std::sync::Arc;

use crate::memory::{GuestMemory, GuestPtr, GuestRange};
use crate::page_tables::{PageTables, SystemRegisters};
use crate::plan::{AddressSpacePlan, PoolAllocator, RegionKind, IMAGE_BASE, RUNTIME_BASE};
use crate::render::{copy_batch, RenderValidator};

/// Where the host puts the guest's EL1 exception vectors (VBAR_EL1).
pub const OPENCE_VECTORS: u32 = 0x0020_0000;
/// Where the page tables go, after the vectors' 2 KB, in the runtime's range.
pub const OPENCE_TABLES: u32 = 0x0020_1000;

/// The guest: opaque to C.
pub struct OpenceGuest {
    // (shared with the Swift app's view of it, src/swift.rs)
    memory: Arc<GuestMemory>,
    plan: AddressSpacePlan,
    pools: PoolAllocator,
    image: Option<GuestImage>,
    last_error: CString,
    // the render batches' objects, for a host with no app to draw them
    // (opence_render_check)
    render: Mutex<RenderValidator>,
}

#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum OpenceStatus {
    Ok = 0,
    /// a null pointer, or an argument out of range
    Argument = 1,
    /// guest memory could not be mapped or used
    Memory = 2,
    /// the image, its header or its imports are not as this host needs
    Image = 3,
    /// the page tables could not be built
    Tables = 4,
    /// the pools have no room
    NoRoom = 5,
    /// a render batch was refused whole
    Refused = 6,
}

/// What a region is for (crate::plan::RegionKind).
#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum OpenceRegionKind {
    Runtime = 0,
    Window = 1,
    Image = 2,
    Pool = 3,
}

/// A region of the guest's address space, all of it mapped: the host maps
/// it into the VM at `start`, from opence_guest_host_pointer(start, size).
#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct OpenceRegion {
    pub kind: OpenceRegionKind,
    /// a pool's number; 0 for the others
    pub index: u32,
    pub start: u32,
    pub size: u64,
}

#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Debug, Default)]
pub struct OpenceSystemRegisters {
    pub ttbr0_el1: u64,
    pub tcr_el1: u64,
    pub mair_el1: u64,
    pub sctlr_el1: u64,
    pub cpacr_el1: u64,
    pub vbar_el1: u64,
}

/// halo_android_abi.h's struct halo_guest_header, as loaded.
#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Debug, Default)]
pub struct OpenceGuestHeader {
    pub magic: u32,
    pub abi_version: u32,
    pub image_end: u32,
    pub import_table: u32,
    pub import_names: u32,
    pub import_count: u32,
    pub start: u32,
    pub thread_start: u32,
    pub thread_attach: u32,
    pub thread_detach: u32,
    pub init_array_start: u32,
    pub init_array_end: u32,
}

/// What opence_guest_load_image found.
#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Debug, Default)]
pub struct OpenceImageInfo {
    pub header: OpenceGuestHeader,
    /// the image's loaded pages
    pub image_start: u32,
    pub image_end: u64,
    pub import_count: u32,
    /// the page tables' size, at registers.ttbr0_el1
    pub table_bytes: u64,
    pub registers: OpenceSystemRegisters,
}

impl From<GuestHeader> for OpenceGuestHeader {
    fn from(h: GuestHeader) -> OpenceGuestHeader {
        OpenceGuestHeader {
            magic: h.magic,
            abi_version: h.abi_version,
            image_end: h.image_end,
            import_table: h.import_table,
            import_names: h.import_names,
            import_count: h.import_count,
            start: h.start,
            thread_start: h.thread_start,
            thread_attach: h.thread_attach,
            thread_detach: h.thread_detach,
            init_array_start: h.init_array_start,
            init_array_end: h.init_array_end,
        }
    }
}

impl From<SystemRegisters> for OpenceSystemRegisters {
    fn from(r: SystemRegisters) -> OpenceSystemRegisters {
        OpenceSystemRegisters {
            ttbr0_el1: r.ttbr0_el1,
            tcr_el1: r.tcr_el1,
            mair_el1: r.mair_el1,
            sctlr_el1: r.sctlr_el1,
            cpacr_el1: r.cpacr_el1,
            vbar_el1: r.vbar_el1,
        }
    }
}

impl OpenceGuest {
    fn new() -> Result<OpenceGuest, String> {
        let plan = AddressSpacePlan::standard();
        let mut memory = GuestMemory::reserve().map_err(|error| error.to_string())?;
        for region in plan.regions() {
            memory
                .map(region.range)
                .map_err(|error| format!("{}: {error}", region.kind))?;
        }
        let memory = Arc::new(memory);
        crate::swift::set_running(&memory);
        let pools = PoolAllocator::new(&plan);
        Ok(OpenceGuest {
            memory,
            plan,
            pools,
            image: None,
            last_error: CString::default(),
            render: Mutex::new(RenderValidator::new()),
        })
    }

    /// Records `message` as the last error and returns `status`.
    fn fail(&mut self, status: OpenceStatus, message: impl std::fmt::Display) -> OpenceStatus {
        // (a message with a NUL in it loses what follows)
        let text = message.to_string();
        let text = text.split('\0').next().unwrap_or_default();
        self.last_error = CString::new(text).unwrap_or_default();
        status
    }

    fn load_image(&mut self, file: &[u8]) -> Result<OpenceImageInfo, (OpenceStatus, String)> {
        let image_range = self
            .plan
            .region(RegionKind::Image)
            .map(|region| region.range)
            .ok_or((
                OpenceStatus::Image,
                "the plan has no image range".to_string(),
            ))?;
        let image = GuestImage::parse(file, image_range)
            .map_err(|e| (OpenceStatus::Image, e.to_string()))?;
        image
            .load(file, &self.memory)
            .map_err(|e| (OpenceStatus::Image, e.to_string()))?;
        let header = image
            .header(&self.memory)
            .map_err(|e| (OpenceStatus::Image, e.to_string()))?;
        let names = image
            .import_names(&self.memory, &header)
            .map_err(|e| (OpenceStatus::Image, e.to_string()))?;
        let tables = PageTables::build(&self.plan, &image.access(), GuestPtr(OPENCE_TABLES))
            .map_err(|e| (OpenceStatus::Tables, e.to_string()))?;
        let bytes = tables.to_bytes();
        self.memory
            .write(tables.base(), &bytes)
            .map_err(|e| (OpenceStatus::Memory, e.to_string()))?;
        let registers = SystemRegisters::new(&tables, GuestPtr(OPENCE_VECTORS))
            .map_err(|e| (OpenceStatus::Tables, e.to_string()))?;
        let info = OpenceImageInfo {
            header: header.into(),
            image_start: image.range().start().0,
            image_end: image.range().end(),
            import_count: names.len() as u32,
            table_bytes: bytes.len() as u64,
            registers: registers.into(),
        };
        self.image = Some(image);
        Ok(info)
    }
}

/// Reserves and maps the guest's memory. NULL if the host's address space
/// has no room for it.
#[no_mangle]
pub extern "C" fn opence_guest_create() -> *mut OpenceGuest {
    match OpenceGuest::new() {
        Ok(guest) => Box::into_raw(Box::new(guest)),
        Err(_) => ptr::null_mut(),
    }
}

/// # Safety
/// `guest` is NULL or was returned by opence_guest_create and not yet
/// destroyed; nothing uses it, or a pointer from it, afterwards.
#[no_mangle]
pub unsafe extern "C" fn opence_guest_destroy(guest: *mut OpenceGuest) {
    if !guest.is_null() {
        // SAFETY: the caller passes a pointer Box::into_raw made, once.
        drop(unsafe { Box::from_raw(guest) });
    }
}

/// The reason for the last failure, as text; valid until the next call that
/// changes the guest. "" if none.
///
/// # Safety
/// `guest` is NULL or a live guest.
#[no_mangle]
pub unsafe extern "C" fn opence_guest_last_error(guest: *const OpenceGuest) -> *const c_char {
    // SAFETY: NULL or a live guest (the caller's promise).
    match unsafe { guest.as_ref() } {
        Some(guest) => guest.last_error.as_ptr(),
        None => c"no guest".as_ptr(),
    }
}

/// The host address of guest address 0. NULL for no guest.
///
/// # Safety
/// `guest` is NULL or a live guest.
#[no_mangle]
pub unsafe extern "C" fn opence_guest_host_base(guest: *const OpenceGuest) -> *mut u8 {
    // SAFETY: NULL or a live guest (the caller's promise).
    unsafe { guest.as_ref() }.map_or(ptr::null_mut(), |guest| guest.memory.host_base())
}

/// The host address of `size` mapped bytes at guest `address`, or NULL if
/// any of them is not mapped.
///
/// # Safety
/// `guest` is NULL or a live guest.
#[no_mangle]
pub unsafe extern "C" fn opence_guest_host_pointer(
    guest: *const OpenceGuest,
    address: u32,
    size: u64,
) -> *mut u8 {
    // SAFETY: NULL or a live guest (the caller's promise).
    let Some(guest) = (unsafe { guest.as_ref() }) else {
        return ptr::null_mut();
    };
    guest
        .memory
        .host_pointer(GuestPtr(address), size)
        .unwrap_or(ptr::null_mut())
}

/// The number of regions (opence_guest_region).
///
/// # Safety
/// `guest` is NULL or a live guest.
#[no_mangle]
pub unsafe extern "C" fn opence_guest_region_count(guest: *const OpenceGuest) -> u32 {
    // SAFETY: NULL or a live guest (the caller's promise).
    unsafe { guest.as_ref() }.map_or(0, |guest| guest.plan.regions().len() as u32)
}

/// Region `index` (in address order) into `*region`.
///
/// # Safety
/// `guest` is NULL or a live guest; `region` is NULL or points to an
/// OpenceRegion the function may write.
#[no_mangle]
pub unsafe extern "C" fn opence_guest_region(
    guest: *const OpenceGuest,
    index: u32,
    region: *mut OpenceRegion,
) -> OpenceStatus {
    // SAFETY: NULL or a live guest (the caller's promise).
    let Some(guest) = (unsafe { guest.as_ref() }) else {
        return OpenceStatus::Argument;
    };
    let Some(found) = guest.plan.regions().get(index as usize) else {
        return OpenceStatus::Argument;
    };
    if region.is_null() {
        return OpenceStatus::Argument;
    }
    let (kind, pool) = match found.kind {
        RegionKind::Runtime => (OpenceRegionKind::Runtime, 0),
        RegionKind::Window => (OpenceRegionKind::Window, 0),
        RegionKind::Image => (OpenceRegionKind::Image, 0),
        RegionKind::Pool(number) => (OpenceRegionKind::Pool, u32::from(number)),
    };
    // SAFETY: region is not NULL and may be written (the caller's promise).
    unsafe {
        region.write(OpenceRegion {
            kind,
            index: pool,
            start: found.range.start().0,
            size: found.range.size(),
        });
    }
    OpenceStatus::Ok
}

/// Checks the guest image in `file` (`size` bytes), copies it to guest
/// memory, writes its page tables (at `info->registers.ttbr0_el1`), and
/// describes it in `*info`. The host then writes the EL1 vectors at
/// `info->registers.vbar_el1`.
///
/// # Safety
/// `guest` is NULL or a live guest; `file` is NULL or points to `size`
/// readable bytes; `info` is NULL or points to an OpenceImageInfo the
/// function may write.
#[no_mangle]
pub unsafe extern "C" fn opence_guest_load_image(
    guest: *mut OpenceGuest,
    file: *const u8,
    size: usize,
    info: *mut OpenceImageInfo,
) -> OpenceStatus {
    // SAFETY: NULL or a live guest, used by this call alone (the caller's
    // promise).
    let Some(guest) = (unsafe { guest.as_mut() }) else {
        return OpenceStatus::Argument;
    };
    if file.is_null() || info.is_null() {
        return guest.fail(
            OpenceStatus::Argument,
            "no image or no place for its description",
        );
    }
    // SAFETY: file points to size readable bytes (the caller's promise).
    let file = unsafe { std::slice::from_raw_parts(file, size) };
    match guest.load_image(file) {
        Ok(found) => {
            // SAFETY: info is not NULL and may be written (the caller's promise).
            unsafe { info.write(found) };
            OpenceStatus::Ok
        }
        Err((status, message)) => guest.fail(status, message),
    }
}

/// Checks the loaded image's imports against the host's `count` names, in
/// the order of the host's svc stubs.
///
/// # Safety
/// `guest` is NULL or a live guest; `names` is NULL or points to `count`
/// pointers to NUL-terminated strings.
#[no_mangle]
pub unsafe extern "C" fn opence_guest_check_imports(
    guest: *mut OpenceGuest,
    names: *const *const c_char,
    count: u32,
) -> OpenceStatus {
    // SAFETY: NULL or a live guest, used by this call alone (the caller's
    // promise).
    let Some(guest) = (unsafe { guest.as_mut() }) else {
        return OpenceStatus::Argument;
    };
    if names.is_null() && count != 0 {
        return guest.fail(OpenceStatus::Argument, "no names");
    }
    let mut host: Vec<String> = Vec::with_capacity(count as usize);
    for index in 0..count as usize {
        // SAFETY: names points to count pointers (the caller's promise).
        let name = unsafe { *names.add(index) };
        if name.is_null() {
            return guest.fail(
                OpenceStatus::Argument,
                format!("host import name {index} is NULL"),
            );
        }
        // SAFETY: each pointer is to a NUL-terminated string (the caller's
        // promise).
        host.push(
            unsafe { CStr::from_ptr(name) }
                .to_string_lossy()
                .into_owned(),
        );
    }
    let Some(image) = guest.image.as_ref() else {
        return guest.fail(OpenceStatus::Argument, "no image is loaded");
    };
    let result = image
        .header(&guest.memory)
        .and_then(|header| image.import_names(&guest.memory, &header))
        .and_then(|found| {
            check_imports(&found, &host.iter().map(String::as_str).collect::<Vec<_>>())
        });
    match result {
        Ok(()) => OpenceStatus::Ok,
        Err(error) => guest.fail(OpenceStatus::Image, error),
    }
}

/// `size` bytes of pool memory (whole pages, zero the first time) into
/// `*address`.
///
/// # Safety
/// `guest` is NULL or a live guest; `address` is NULL or points to a u32
/// the function may write.
#[no_mangle]
pub unsafe extern "C" fn opence_guest_allocate(
    guest: *mut OpenceGuest,
    size: u64,
    address: *mut u32,
) -> OpenceStatus {
    // SAFETY: NULL or a live guest, used by this call alone (the caller's
    // promise).
    let Some(guest) = (unsafe { guest.as_mut() }) else {
        return OpenceStatus::Argument;
    };
    if address.is_null() {
        return guest.fail(OpenceStatus::Argument, "no place for the address");
    }
    match guest.pools.allocate(size) {
        Ok(range) => {
            // SAFETY: address is not NULL and may be written (the caller's
            // promise).
            unsafe { address.write(range.start().0) };
            OpenceStatus::Ok
        }
        Err(error) => guest.fail(OpenceStatus::NoRoom, error),
    }
}

/// Gives back pool memory opence_guest_allocate handed out.
///
/// # Safety
/// `guest` is NULL or a live guest.
#[no_mangle]
pub unsafe extern "C" fn opence_guest_free(
    guest: *mut OpenceGuest,
    address: u32,
    size: u64,
) -> OpenceStatus {
    // SAFETY: NULL or a live guest, used by this call alone (the caller's
    // promise).
    let Some(guest) = (unsafe { guest.as_mut() }) else {
        return OpenceStatus::Argument;
    };
    let result = GuestRange::new(GuestPtr(address), size)
        .map_err(|error| error.to_string())
        .and_then(|range| guest.pools.free(range).map_err(|error| error.to_string()));
    match result {
        Ok(()) => OpenceStatus::Ok,
        Err(message) => guest.fail(OpenceStatus::Argument, message),
    }
}

/// What opence_render_check found in a batch.
#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Debug, Default)]
pub struct OpenceRenderCheck {
    /// the commands that passed, and those refused
    pub commands: u32,
    pub refused: u32,
    /// the batch's bytes as the host copied them, hashed (FNV-1a), for tests
    /// that compare them with what the guest wrote
    pub fnv: u32,
}

/// Told of each command refused: its place in the batch (bytes), its op
/// and why, as text valid for the call.
pub type OpenceRenderRefusal =
    Option<unsafe extern "C" fn(context: *mut c_void, offset: u32, op: u32, reason: *const c_char)>;

/// Copies out the `size` bytes of render batch at guest `address` and
/// checks it (src/render.rs) against the objects the guest's batches before
/// it made, for a host with no app to draw them. Each command refused is
/// given to `refusal` (if not NULL) and the counts are written to `*check`.
/// OPENCE_STATUS_REFUSED if the batch is refused whole, with the reason in
/// the `reason_size` bytes at `reason` (if not NULL).
///
/// # Safety
/// `guest` is NULL or a live guest; `check` is NULL or points to an
/// OpenceRenderCheck the function may write; `reason` is NULL or points to
/// `reason_size` bytes it may write; `refusal` is safe to call with
/// `context`.
#[no_mangle]
pub unsafe extern "C" fn opence_render_check(
    guest: *const OpenceGuest,
    address: u32,
    size: u32,
    check: *mut OpenceRenderCheck,
    reason: *mut c_char,
    reason_size: usize,
    refusal: OpenceRenderRefusal,
    context: *mut c_void,
) -> OpenceStatus {
    // SAFETY: NULL or a live guest (the caller's promise).
    let Some(guest) = (unsafe { guest.as_ref() }) else {
        return OpenceStatus::Argument;
    };
    if check.is_null() {
        return OpenceStatus::Argument;
    }
    let decoded = copy_batch(&guest.memory, address, size).and_then(|bytes| {
        let mut validator = guest
            .render
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        validator
            .decode(&bytes, &*guest.memory)
            .map(|decoded| (decoded, fnv(&bytes)))
    });
    let (decoded, hash) = match decoded {
        Ok(found) => found,
        Err(error) => {
            if !reason.is_null() && reason_size > 0 {
                let text = error.to_string().replace('\0', " ");
                let length = text.len().min(reason_size - 1);
                // SAFETY: reason has reason_size writable bytes (the
                // caller's promise); length + 1 of them are written.
                unsafe {
                    ptr::copy_nonoverlapping(text.as_ptr().cast(), reason, length);
                    reason.add(length).write(0);
                }
            }
            return OpenceStatus::Refused;
        }
    };
    if let Some(refusal) = refusal {
        for rejection in &decoded.rejected {
            let text = CString::new(rejection.reason.replace('\0', " ")).unwrap_or_default();
            // SAFETY: refusal may be called with context (the caller's
            // promise); text lives for the call.
            unsafe { refusal(context, rejection.offset, rejection.op, text.as_ptr()) };
        }
    }
    // SAFETY: check is not NULL and may be written (the caller's promise).
    unsafe {
        check.write(OpenceRenderCheck {
            commands: decoded.commands.len() as u32,
            refused: decoded.rejected.len() as u32,
            fnv: hash,
        })
    };
    OpenceStatus::Ok
}

fn fnv(bytes: &[u8]) -> u32 {
    bytes.iter().fold(0x811c_9dc5, |hash: u32, &byte| {
        (hash ^ u32::from(byte)).wrapping_mul(0x0100_0193)
    })
}

// (the constants the header gives C are the plan's)
const _: () = assert!(OPENCE_VECTORS == RUNTIME_BASE && OPENCE_TABLES == RUNTIME_BASE + 0x1000);
const _: () = assert!(IMAGE_BASE == 0x8800_0000);

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_images::{elf, header_bytes, segment, PF_W, PF_X};
    use std::ffi::CString;
    use std::path::Path;

    struct Guest(*mut OpenceGuest);

    impl Drop for Guest {
        fn drop(&mut self) {
            // SAFETY: made by opence_guest_create, destroyed once here.
            unsafe { opence_guest_destroy(self.0) };
        }
    }

    fn guest() -> Guest {
        let _running = crate::swift::RUNNING_TESTS
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let guest = opence_guest_create();
        assert!(!guest.is_null());
        Guest(guest)
    }

    fn last_error(guest: &Guest) -> String {
        // SAFETY: a live guest; the message is NUL-terminated.
        unsafe { CStr::from_ptr(opence_guest_last_error(guest.0)) }
            .to_string_lossy()
            .into_owned()
    }

    fn sample_image() -> Vec<u8> {
        elf(&[
            segment(
                0x8800_0000,
                4 | PF_X,
                &header_bytes(&["host_syscall", "host_log"]),
                0x2000,
            ),
            segment(0x8801_0000, 4 | PF_W, b"data", 0x3000),
        ])
    }

    fn load(guest: &Guest, file: &[u8]) -> (OpenceStatus, OpenceImageInfo) {
        let mut info = OpenceImageInfo::default();
        // SAFETY: a live guest; file and info are valid for the call.
        let status =
            unsafe { opence_guest_load_image(guest.0, file.as_ptr(), file.len(), &mut info) };
        (status, info)
    }

    fn check(guest: &Guest, names: &[&str]) -> OpenceStatus {
        let owned: Vec<CString> = names
            .iter()
            .map(|name| CString::new(*name).expect("no NUL"))
            .collect();
        let pointers: Vec<*const c_char> = owned.iter().map(|name| name.as_ptr()).collect();
        // SAFETY: a live guest; pointers holds names.len() valid strings.
        unsafe { opence_guest_check_imports(guest.0, pointers.as_ptr(), pointers.len() as u32) }
    }

    #[test]
    fn every_region_is_mapped_and_listed() {
        let guest = guest();
        // SAFETY: a live guest.
        let count = unsafe { opence_guest_region_count(guest.0) };
        assert_eq!(count, 15);
        let mut pools = 0;
        for index in 0..count {
            let mut region = OpenceRegion {
                kind: OpenceRegionKind::Runtime,
                index: 0,
                start: 0,
                size: 0,
            };
            // SAFETY: a live guest; region may be written.
            assert_eq!(
                unsafe { opence_guest_region(guest.0, index, &mut region) },
                OpenceStatus::Ok
            );
            // SAFETY: a live guest.
            let pointer = unsafe { opence_guest_host_pointer(guest.0, region.start, region.size) };
            // SAFETY: a live guest.
            let base = unsafe { opence_guest_host_base(guest.0) };
            assert_eq!(
                pointer as usize,
                base as usize + region.start as usize,
                "{region:?}"
            );
            if region.kind == OpenceRegionKind::Pool {
                assert_eq!(region.index, pools);
                pools += 1;
            }
        }
        assert_eq!(pools, 12);
        // SAFETY: a live guest; a null output.
        assert_eq!(
            unsafe { opence_guest_region(guest.0, 0, ptr::null_mut()) },
            OpenceStatus::Argument
        );
        // SAFETY: a live guest.
        assert_eq!(
            unsafe { opence_guest_region(guest.0, count, ptr::null_mut()) },
            OpenceStatus::Argument
        );
    }

    #[test]
    fn what_is_not_mapped_has_no_host_address() {
        let guest = guest();
        for (address, size) in [
            (0u32, 1u64),
            (0x40_0000, 4),
            (0x8bff_fffc, 8),
            (0xe000_0000, 1),
        ] {
            // SAFETY: a live guest.
            assert!(
                unsafe { opence_guest_host_pointer(guest.0, address, size) }.is_null(),
                "{address:#x}"
            );
        }
    }

    #[test]
    fn an_image_loads_with_its_tables_and_registers() {
        let guest = guest();
        let (status, info) = load(&guest, &sample_image());
        assert_eq!(status, OpenceStatus::Ok, "{}", last_error(&guest));
        assert_eq!(info.header.magic, crate::image::GUEST_MAGIC);
        assert_eq!(
            (info.image_start, info.image_end),
            (IMAGE_BASE, 0x8801_3000)
        );
        assert_eq!(info.import_count, 2);
        assert_eq!(info.registers.ttbr0_el1, u64::from(OPENCE_TABLES));
        assert_eq!(info.registers.vbar_el1, u64::from(OPENCE_VECTORS));
        // the tables are in guest memory: the first level 1 entry points at
        // the first level 2 table
        // SAFETY: a live guest.
        let tables = unsafe { opence_guest_host_pointer(guest.0, OPENCE_TABLES, info.table_bytes) };
        assert!(!tables.is_null());
        // SAFETY: tables points to table_bytes mapped bytes, written by the load.
        let first = unsafe { tables.cast::<u64>().read_unaligned() };
        assert_eq!(first & 0xffff_ffff_f000, u64::from(OPENCE_TABLES) + 0x1000);
        assert_eq!(
            check(&guest, &["host_syscall", "host_log"]),
            OpenceStatus::Ok
        );
        assert_eq!(
            check(&guest, &["host_syscall", "host_abort"]),
            OpenceStatus::Image
        );
        assert!(
            last_error(&guest).contains("host_abort"),
            "{}",
            last_error(&guest)
        );
    }

    #[test]
    fn a_bad_image_is_refused_with_its_reason() {
        let guest = guest();
        let (status, _) = load(&guest, b"not an image");
        assert_eq!(status, OpenceStatus::Image);
        assert!(last_error(&guest).contains("not a 64-bit little-endian AArch64 executable"));
        // and imports cannot be checked with no image loaded
        assert_eq!(check(&guest, &[]), OpenceStatus::Argument);
        // SAFETY: a live guest; a null file is refused.
        let status = unsafe { opence_guest_load_image(guest.0, ptr::null(), 0, ptr::null_mut()) };
        assert_eq!(status, OpenceStatus::Argument);
    }

    /// The batch of `commands` (each its op and fields), as bytes.
    fn render_batch(commands: &[(u32, &[u32])]) -> Vec<u8> {
        let mut words = vec![crate::render::MAGIC, crate::render::VERSION, 0, 0];
        for &(op, fields) in commands {
            words.push(op | (fields.len() as u32 + 1) << 16);
            words.extend_from_slice(fields);
        }
        words[2] = words.len() as u32 * 4;
        words[3] = commands.len() as u32;
        words.iter().flat_map(|word| word.to_le_bytes()).collect()
    }

    unsafe extern "C" fn note_refusal(
        context: *mut c_void,
        offset: u32,
        op: u32,
        reason: *const c_char,
    ) {
        // SAFETY: the test's context is its Vec, and reason a C string.
        let refusals = unsafe { &mut *context.cast::<Vec<(u32, u32, String)>>() };
        let reason = unsafe { CStr::from_ptr(reason) }
            .to_string_lossy()
            .into_owned();
        refusals.push((offset, op, reason));
    }

    #[test]
    fn render_batches_are_checked_in_guest_memory() {
        use crate::render::op;
        let guest = guest();
        let mut address = 0u32;
        // SAFETY: a live guest; address may be written.
        assert_eq!(
            unsafe { opence_guest_allocate(guest.0, 0x4000, &mut address) },
            OpenceStatus::Ok
        );
        let batch = render_batch(&[
            (op::CREATE_BUFFER, &[1, 64]),
            (op::UPLOAD_BUFFER, &[1, 0, address, 64]),
            (op::UPLOAD_BUFFER, &[2, 0, address, 64]),
        ]);
        // SAFETY: a live guest; the batch fits the memory just allocated.
        unsafe {
            let at = opence_guest_host_pointer(guest.0, address, batch.len() as u64);
            ptr::copy_nonoverlapping(batch.as_ptr(), at, batch.len());
        }
        let mut check = OpenceRenderCheck::default();
        let mut refusals: Vec<(u32, u32, String)> = Vec::new();
        let mut reason = [0 as c_char; 128];
        // SAFETY: a live guest; check, reason and refusals live for the call.
        let status = unsafe {
            opence_render_check(
                guest.0,
                address,
                batch.len() as u32,
                &mut check,
                reason.as_mut_ptr(),
                reason.len(),
                Some(note_refusal),
                (&mut refusals as *mut Vec<(u32, u32, String)>).cast(),
            )
        };
        assert_eq!(status, OpenceStatus::Ok);
        assert_eq!(
            check,
            OpenceRenderCheck {
                commands: 2,
                refused: 1,
                fnv: fnv(&batch),
            }
        );
        // (the third command, after the header and 3 + 5 words)
        assert_eq!(refusals.len(), 1);
        assert_eq!((refusals[0].0, refusals[0].1), (48, op::UPLOAD_BUFFER));
        assert!(!refusals[0].2.is_empty());

        // the objects last: buffer 1 is the batch before's
        let next = render_batch(&[(op::UPLOAD_BUFFER, &[1, 0, address, 64])]);
        // SAFETY: as above.
        let status = unsafe {
            let at = opence_guest_host_pointer(guest.0, address, next.len() as u64);
            ptr::copy_nonoverlapping(next.as_ptr(), at, next.len());
            opence_render_check(
                guest.0,
                address,
                next.len() as u32,
                &mut check,
                ptr::null_mut(),
                0,
                None,
                ptr::null_mut(),
            )
        };
        assert_eq!(status, OpenceStatus::Ok);
        assert_eq!((check.commands, check.refused), (1, 0));

        // a batch not mapped, or not a batch, is refused whole with its reason
        for (at, size) in [(0xffff_f000, 16), (address, 6)] {
            // SAFETY: a live guest; check and reason live for the call.
            let status = unsafe {
                opence_render_check(
                    guest.0,
                    at,
                    size,
                    &mut check,
                    reason.as_mut_ptr(),
                    reason.len(),
                    None,
                    ptr::null_mut(),
                )
            };
            assert_eq!(status, OpenceStatus::Refused);
            // SAFETY: the reason is NUL-terminated.
            assert!(!unsafe { CStr::from_ptr(reason.as_ptr()) }.is_empty());
        }
        // SAFETY: no guest, and no place for the counts.
        assert_eq!(
            unsafe {
                opence_render_check(
                    ptr::null(),
                    address,
                    16,
                    &mut check,
                    ptr::null_mut(),
                    0,
                    None,
                    ptr::null_mut(),
                )
            },
            OpenceStatus::Argument
        );
    }

    #[test]
    fn pool_memory_is_handed_out_and_taken_back() {
        let guest = guest();
        let mut address = 0u32;
        // SAFETY: a live guest; address may be written.
        assert_eq!(
            unsafe { opence_guest_allocate(guest.0, 0x3000, &mut address) },
            OpenceStatus::Ok
        );
        assert_eq!(address, 0x1000_0000);
        // SAFETY: a live guest.
        assert!(!unsafe { opence_guest_host_pointer(guest.0, address, 0x3000) }.is_null());
        // SAFETY: a live guest.
        assert_eq!(
            unsafe { opence_guest_free(guest.0, address, 0x3000) },
            OpenceStatus::Ok
        );
        // SAFETY: a live guest.
        assert_eq!(
            unsafe { opence_guest_free(guest.0, 0x8000_0000, 0x1000) },
            OpenceStatus::Argument
        );
        // SAFETY: a live guest; address may be written.
        assert_eq!(
            unsafe { opence_guest_allocate(guest.0, 1 << 40, &mut address) },
            OpenceStatus::NoRoom
        );
        // SAFETY: a live guest; a null output.
        assert_eq!(
            unsafe { opence_guest_allocate(guest.0, 1, ptr::null_mut()) },
            OpenceStatus::Argument
        );
    }

    #[test]
    fn no_guest_is_an_argument_error() {
        // SAFETY: NULL is allowed everywhere.
        unsafe {
            assert!(opence_guest_host_base(ptr::null()).is_null());
            assert!(opence_guest_host_pointer(ptr::null(), 0x8000_0000, 1).is_null());
            assert_eq!(opence_guest_region_count(ptr::null()), 0);
            assert_eq!(
                opence_guest_free(ptr::null_mut(), 0, 1),
                OpenceStatus::Argument
            );
            assert_eq!(
                opence_guest_check_imports(ptr::null_mut(), ptr::null(), 0),
                OpenceStatus::Argument
            );
            assert!(!CStr::from_ptr(opence_guest_last_error(ptr::null()))
                .to_bytes()
                .is_empty());
            opence_guest_destroy(ptr::null_mut());
        }
    }

    /// The built image against the host's generated list of its imports
    /// (build/macos/host/host_import_names.c): the two agree, index for index.
    #[test]
    fn the_built_image_matches_the_hosts_import_names() {
        let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../..");
        let (Ok(file), Ok(listing)) = (
            std::fs::read(root.join("build/macos/halo_guest.elf")),
            std::fs::read_to_string(root.join("build/macos/host/host_import_names.c")),
        ) else {
            eprintln!("skipped: `ninja macos` has not run");
            return;
        };
        let names: Vec<&str> = listing
            .lines()
            .filter_map(|line| line.trim().strip_prefix('"')?.strip_suffix("\","))
            .collect();
        let guest = guest();
        let (status, info) = load(&guest, &file);
        assert_eq!(status, OpenceStatus::Ok, "{}", last_error(&guest));
        assert_eq!(info.import_count as usize, names.len());
        assert_eq!(
            check(&guest, &names),
            OpenceStatus::Ok,
            "{}",
            last_error(&guest)
        );
    }
}
