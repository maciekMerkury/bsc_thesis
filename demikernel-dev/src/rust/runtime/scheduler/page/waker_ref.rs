// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

//======================================================================================================================
// Imports
//======================================================================================================================

use crate::runtime::scheduler::page::{page::WakerPage, WakerPageRef, WAKER_PAGE_SIZE};
use ::std::{
    mem,
    ptr::NonNull,
    task::{RawWaker, RawWakerVTable},
};

//======================================================================================================================
// Structures
//======================================================================================================================

/// This reference is a representation for the status of a particular task
/// stored in a [WakerPage].
#[repr(transparent)]
pub struct WakerRef(NonNull<u8>);

//======================================================================================================================
// Associate Functions
//======================================================================================================================

impl WakerRef {
    pub fn new(raw_page_ref: NonNull<u8>) -> Self {
        Self(raw_page_ref)
    }

    /// Casts the target [WakerRef] back into reference to a [WakerPage] plus an
    /// offset indicating the target task in the latter structure.
    ///
    /// For more information on this hack see comments on [crate::page::WakerPageRef].
    fn base_ptr(&self) -> (NonNull<WakerPage>, usize) {
        let ptr: *mut u8 = self.0.as_ptr();
        let forward_offset: usize = ptr.align_offset(WAKER_PAGE_SIZE);
        let mut base_ptr: *mut u8 = ptr;
        let mut offset: usize = 0;
        if forward_offset != 0 {
            offset = WAKER_PAGE_SIZE - forward_offset;
            base_ptr = ptr.wrapping_sub(offset);
        }
        unsafe { (NonNull::new_unchecked(base_ptr).cast(), offset) }
    }

    /// Sets the notification flag for the task that associated with the target [WakerRef].
    fn wake_by_ref(&self) {
        let (base_ptr, ix): (NonNull<WakerPage>, usize) = self.base_ptr();
        let base: &WakerPage = unsafe { &*base_ptr.as_ptr() };
        base.notify(ix);
    }

    /// Sets the notification flag for the task that is associated with the target [WakerRef].
    fn wake(self) {
        self.wake_by_ref()
    }

    /// Gets the reference count of the target [WakerRef].
    #[cfg(test)]
    pub fn refcount_get(&self) -> u64 {
        let (base_ptr, _): (NonNull<WakerPage>, _) = self.base_ptr();
        unsafe { base_ptr.as_ref().refcount_get() }
    }
}

//======================================================================================================================
// Trait Implementations
//======================================================================================================================

impl Clone for WakerRef {
    fn clone(&self) -> Self {
        let (base_ptr, _): (NonNull<WakerPage>, _) = self.base_ptr();
        let p: WakerPageRef = WakerPageRef::new(base_ptr);
        // Increment reference count.
        mem::forget(p.clone());
        // This is not a double increment.
        mem::forget(p);
        WakerRef(self.0)
    }
}

impl Drop for WakerRef {
    fn drop(&mut self) {
        let (base_ptr, _) = self.base_ptr();
        // Decrement the refcount.
        drop(WakerPageRef::new(base_ptr));
    }
}

impl Into<RawWaker> for WakerRef {
    fn into(self) -> RawWaker {
        let ptr: *const () = self.0.cast().as_ptr() as *const ();
        let waker: RawWaker = RawWaker::new(ptr, &VTABLE);
        // Increment reference count.
        mem::forget(self);
        waker
    }
}

unsafe fn waker_ref_clone(ptr: *const ()) -> RawWaker {
    let p: WakerRef = WakerRef(NonNull::new_unchecked(ptr as *const u8 as *mut u8));
    let q: WakerRef = p.clone();
    // Increment reference count.
    mem::forget(p);
    q.into()
}

unsafe fn waker_ref_wake(ptr: *const ()) {
    let p = WakerRef(NonNull::new_unchecked(ptr as *const u8 as *mut u8));
    p.wake();
}

unsafe fn waker_ref_wake_by_ref(ptr: *const ()) {
    let p: WakerRef = WakerRef(NonNull::new_unchecked(ptr as *const u8 as *mut u8));
    p.wake_by_ref();
    // Increment reference count.
    mem::forget(p);
}

unsafe fn waker_ref_drop(ptr: *const ()) {
    let p: WakerRef = WakerRef(NonNull::new_unchecked(ptr as *const u8 as *mut u8));
    // Decrement reference count.
    drop(p);
}

/// Raw Waker Trait Implementation for Waker References
pub const VTABLE: RawWakerVTable =
    RawWakerVTable::new(waker_ref_clone, waker_ref_wake, waker_ref_wake_by_ref, waker_ref_drop);

//======================================================================================================================
// Unit Tests
//======================================================================================================================

#[cfg(test)]
mod tests {
    use crate::runtime::scheduler::{
        page::{WakerPageRef, WakerRef},
        waker64::WAKER_BIT_LENGTH,
    };
    use ::anyhow::Result;
    use ::rand::Rng;
    use ::std::ptr::NonNull;
    use ::test::{black_box, Bencher};

    #[test]
    fn test_refcount() -> Result<()> {
        let p: WakerPageRef = WakerPageRef::default();
        crate::ensure_eq!(p.refcount_get(), 1);

        let p_clone: NonNull<u8> = p.into_raw_waker_ref(0);
        let refcount: u64 = p.refcount_get();
        crate::ensure_eq!(refcount, 2);
        let q: WakerRef = WakerRef::new(p_clone);
        let refcount: u64 = q.refcount_get();
        crate::ensure_eq!(refcount, 2);
        let refcount: u64 = p.refcount_get();
        crate::ensure_eq!(refcount, 2);

        let r: WakerRef = WakerRef::new(p.into_raw_waker_ref(31));
        let refcount: u64 = r.refcount_get();
        crate::ensure_eq!(refcount, 3);
        let refcount: u64 = p.refcount_get();
        crate::ensure_eq!(refcount, 3);

        let s: WakerRef = r.clone();
        let refcount: u64 = s.refcount_get();
        crate::ensure_eq!(refcount, 4);
        let refcount: u64 = p.refcount_get();
        crate::ensure_eq!(refcount, 4);

        drop(s);
        let refcount: u64 = p.refcount_get();
        crate::ensure_eq!(refcount, 3);

        drop(r);
        let refcount: u64 = p.refcount_get();
        crate::ensure_eq!(refcount, 2);

        drop(q);
        let refcount: u64 = p.refcount_get();
        crate::ensure_eq!(refcount, 1);

        Ok(())
    }

    #[test]
    fn test_wake() -> Result<()> {
        let p: WakerPageRef = WakerPageRef::default();
        crate::ensure_eq!(p.refcount_get(), 1);

        let q: WakerRef = WakerRef::new(p.into_raw_waker_ref(0));
        crate::ensure_eq!(p.refcount_get(), 2);
        let r: WakerRef = WakerRef::new(p.into_raw_waker_ref(31));
        crate::ensure_eq!(p.refcount_get(), 3);
        let s: WakerRef = WakerRef::new(p.into_raw_waker_ref(15));
        crate::ensure_eq!(p.refcount_get(), 4);

        q.wake();
        crate::ensure_eq!(p.take_notified(), 1 << 0);
        crate::ensure_eq!(p.refcount_get(), 3);

        r.wake();
        s.wake();
        crate::ensure_eq!(p.take_notified(), 1 << 15 | 1 << 31);
        crate::ensure_eq!(p.refcount_get(), 1);

        Ok(())
    }

    #[test]
    fn test_wake_by_ref() -> Result<()> {
        let p: WakerPageRef = WakerPageRef::default();
        crate::ensure_eq!(p.refcount_get(), 1);

        let q: WakerRef = WakerRef::new(p.into_raw_waker_ref(0));
        crate::ensure_eq!(p.refcount_get(), 2);
        let r: WakerRef = WakerRef::new(p.into_raw_waker_ref(31));
        crate::ensure_eq!(p.refcount_get(), 3);
        let s: WakerRef = WakerRef::new(p.into_raw_waker_ref(15));
        crate::ensure_eq!(p.refcount_get(), 4);

        q.wake_by_ref();
        crate::ensure_eq!(p.take_notified(), 1 << 0);
        crate::ensure_eq!(p.refcount_get(), 4);

        r.wake_by_ref();
        s.wake_by_ref();
        crate::ensure_eq!(p.take_notified(), 1 << 15 | 1 << 31);
        crate::ensure_eq!(p.refcount_get(), 4);

        drop(s);
        let refcount: u64 = p.refcount_get();
        crate::ensure_eq!(refcount, 3);

        drop(r);
        let refcount: u64 = p.refcount_get();
        crate::ensure_eq!(refcount, 2);

        drop(q);
        let refcount: u64 = p.refcount_get();
        crate::ensure_eq!(refcount, 1);

        Ok(())
    }

    #[bench]
    fn wake_bench(b: &mut Bencher) {
        let p: WakerPageRef = WakerPageRef::default();
        let ix: usize = rand::thread_rng().gen_range(0..WAKER_BIT_LENGTH);

        b.iter(|| {
            let raw_page_ref: NonNull<u8> = black_box(p.into_raw_waker_ref(ix));
            let q: WakerRef = WakerRef::new(raw_page_ref);
            q.wake();
        });
    }

    #[bench]
    fn wake_by_ref_bench(b: &mut Bencher) {
        let p: WakerPageRef = WakerPageRef::default();
        let ix: usize = rand::thread_rng().gen_range(0..WAKER_BIT_LENGTH);

        b.iter(|| {
            let raw_page_ref: NonNull<u8> = black_box(p.into_raw_waker_ref(ix));
            let q: WakerRef = WakerRef::new(raw_page_ref);
            q.wake_by_ref();
        });
    }
}
