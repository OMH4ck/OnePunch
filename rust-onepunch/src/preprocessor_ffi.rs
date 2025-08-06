use std::os::raw::{c_uchar, c_uint, c_ulong};
use crate::{RustPreprocessor, RustConstraintAnalyzer, RustSegment};

#[no_mangle]
pub extern "C" fn rust_preprocessor_new() -> *mut RustPreprocessor {
    Box::into_raw(Box::new(RustPreprocessor::new()))
}

#[no_mangle]
pub extern "C" fn rust_preprocessor_free(preprocessor: *mut RustPreprocessor) {
    if !preprocessor.is_null() {
        unsafe {
            drop(Box::from_raw(preprocessor));
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_preprocessor_process(
    preprocessor: *mut RustPreprocessor,
    segments_ptr: *const *mut RustSegment,
    segments_len: c_uint,
) {
    if preprocessor.is_null() || segments_ptr.is_null() {
        return;
    }
    
    unsafe {
        let preprocessor_ref = &mut *preprocessor;
        let segments_slice = std::slice::from_raw_parts(segments_ptr, segments_len as usize);
        preprocessor_ref.process(segments_slice);
    }
}

#[no_mangle]
pub extern "C" fn rust_constraint_analyzer_compute_constraint(
    segment: *const RustSegment
) -> c_ulong {
    RustConstraintAnalyzer::compute_constraint(segment)
}

#[no_mangle]
pub extern "C" fn rust_constraint_analyzer_hash_match(
    needed: c_ulong,
    src: c_ulong,
) -> c_uchar {
    if RustConstraintAnalyzer::hash_match(needed, src) { 1 } else { 0 }
}