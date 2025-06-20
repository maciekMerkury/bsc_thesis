// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

mod buffer_pool;
mod demibuffer;
mod memory_pool;

//======================================================================================================================
// Imports
//======================================================================================================================

use crate::runtime::{
    fail::Fail,
    types::{demi_sgarray_t, demi_sgaseg_t},
};
use ::libc::c_void;
use ::std::{
    mem,
    ptr::{self, NonNull},
};

//======================================================================================================================
// Exports
//======================================================================================================================

pub use self::{buffer_pool::*, demibuffer::*};

//======================================================================================================================
// Traits
//======================================================================================================================

pub trait DemiMemoryAllocator {
    fn allocate_demi_buffer(&self, size: usize) -> Result<DemiBuffer, Fail> {
        Ok(DemiBuffer::new(size as u16))
    }
}

/// Converts a buffer into a scatter-gather array.
pub fn into_sgarray(buf: DemiBuffer) -> Result<demi_sgarray_t, Fail> {
    // Create a scatter-gather segment to expose the DemiBuffer to the user.
    let data: *const u8 = buf.as_ptr();
    let sga_seg: demi_sgaseg_t = demi_sgaseg_t {
        sgaseg_buf: data as *mut c_void,
        sgaseg_len: buf.len() as u32,
    };

    // Create and return a new scatter-gather array (which inherits the DemiBuffer's reference).
    Ok(demi_sgarray_t {
        sga_buf: buf.into_raw().as_ptr() as *mut c_void,
        sga_numsegs: 1,
        sga_segs: [sga_seg],
        sga_addr: unsafe { mem::zeroed() },
    })
}

/// Allocates a scatter-gather array.
pub fn sgaalloc<M: DemiMemoryAllocator>(size: usize, mem_alloc: &M) -> Result<demi_sgarray_t, Fail> {
    // TODO: Allocate an array of buffers if requested size is too large for a single buffer.

    // We can't allocate a zero-sized buffer.
    if size == 0 {
        let cause: String = format!("cannot allocate a zero-sized buffer");
        error!("sgaalloc(): {}", cause);
        return Err(Fail::new(libc::EINVAL, &cause));
    }

    // We can't allocate more than a single buffer.
    if size > u16::MAX as usize {
        return Err(Fail::new(libc::EINVAL, "size too large for a single demi_sgaseg_t"));
    }

    // First allocate the underlying DemiBuffer.
    let buf: DemiBuffer = mem_alloc.allocate_demi_buffer(size)?;
    debug_assert_eq!(buf.len(), size);

    into_sgarray(buf)
}

/// Releases a scatter-gather array.
pub fn sgafree(sga: demi_sgarray_t) -> Result<(), Fail> {
    // Check arguments.
    // TODO: Drop this check once we support scatter-gather arrays with multiple segments.
    if sga.sga_numsegs != 1 {
        return Err(Fail::new(libc::EINVAL, "demi_sgarray_t has invalid segment count"));
    }

    if sga.sga_buf == ptr::null_mut() {
        return Err(Fail::new(libc::EINVAL, "demi_sgarray_t has invalid DemiBuffer token"));
    }

    // Convert back to a DemiBuffer and drop it.
    // Safety: The `NonNull::new_unchecked()` call is safe, as we verified `sga.sga_buf` is not null above.
    let token: NonNull<u8> = unsafe { NonNull::new_unchecked(sga.sga_buf as *mut u8) };
    // Safety: The `DemiBuffer::from_raw()` call *should* be safe, as the `sga_buf` field in the `demi_sgarray_t`
    // contained a valid `DemiBuffer` token when we provided it to the user (and the user shouldn't change it).
    let buf: DemiBuffer = unsafe { DemiBuffer::from_raw(token) };
    drop(buf);

    Ok(())
}

/// Clones a scatter-gather array.
pub fn clone_sgarray(sga: &demi_sgarray_t) -> Result<DemiBuffer, Fail> {
    // Check arguments.
    // TODO: Drop this check once we support scatter-gather arrays with multiple segments.
    if sga.sga_numsegs != 1 {
        return Err(Fail::new(libc::EINVAL, "demi_sgarray_t has invalid segment count"));
    }

    if sga.sga_buf == ptr::null_mut() {
        return Err(Fail::new(libc::EINVAL, "demi_sgarray_t has invalid DemiBuffer token"));
    }

    // Convert back to a DemiBuffer.
    // Safety: The `NonNull::new_unchecked()` call is safe, as we verified `sga.sga_buf` is not null above.
    let token: NonNull<u8> = unsafe { NonNull::new_unchecked(sga.sga_buf as *mut u8) };
    // Safety: The `DemiBuffer::from_raw()` call *should* be safe, as the `sga_buf` field in the `demi_sgarray_t`
    // contained a valid `DemiBuffer` token when we provided it to the user (and the user shouldn't change it).
    let buf: DemiBuffer = unsafe { DemiBuffer::from_raw(token) };
    let mut clone: DemiBuffer = buf.clone();

    // Don't drop buf, as it holds the same reference to the data as the sgarray (which should keep it).
    mem::forget(buf);

    // Check whether the limits of the buffer have changed.
    check_demi_buf_limits(&sga, &mut clone)?;

    // Return the clone.
    Ok(clone)
}

/// Check to see if the user has reduced the size of the buffer described by the sgarray segment since we provided it to
/// them.  They could have increased the starting address of the buffer (`sgaseg_buf`), decreased the ending address of
/// the buffer (`sgaseg_buf + sgaseg_len`), or both.
fn check_demi_buf_limits(sga: &demi_sgarray_t, clone: &mut DemiBuffer) -> Result<(), Fail> {
    let sga_data: *const u8 = sga.sga_segs[0].sgaseg_buf as *const u8;
    let sga_len: usize = sga.sga_segs[0].sgaseg_len as usize;
    let clone_data: *const u8 = clone.as_ptr();
    let mut clone_len: usize = clone.len();
    if sga_data != clone_data || sga_len != clone_len {
        // We need to adjust the DemiBuffer to match the user's changes.

        // First check that the user didn't do something non-sensical, like change the buffer description to
        // reference address space outside of the DemiBuffer's allocated memory area.
        if sga_data < clone_data || sga_data.addr() + sga_len > clone_data.addr() + clone_len {
            return Err(Fail::new(
                libc::EINVAL,
                "demi_sgarray_t describes data outside backing buffer's allocated region",
            ));
        }

        // Calculate the amount the new starting address is ahead of the old.  And then adjust `clone` to match.
        let adjustment_amount: usize = sga_data.addr() - clone_data.addr();
        clone.adjust(adjustment_amount)?;

        // An adjustment above would have reduced clone.len() by the adjustment amount.
        clone_len -= adjustment_amount;
        debug_assert_eq!(clone_len, clone.len());

        // Trim the clone down to size.
        let trim_amount: usize = clone_len - sga_len;
        clone.trim(trim_amount)?;
    }
    Ok(())
}
