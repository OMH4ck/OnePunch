use std::os::raw::{c_char, c_double, c_uchar, c_ulong, c_uint};
use std::ffi::{CStr};
use std::collections::hash_map::DefaultHasher;
use std::hash::{Hash, Hasher};
use std::time::{SystemTime, UNIX_EPOCH};

/// Transfer operation length to string representation
pub fn transfer_operation_len_to_str(dtype: c_uint) -> &'static [u8] {
    match dtype {
        0 => b"NONE\0",
        1 => b"BYTE\0", 
        2 => b"WORD\0",
        4 => b"DWORD\0",
        8 => b"QWORD\0",
        _ => b"UNKNOWN\0",
    }
}

/// Hash function for strings (equivalent to fuck_hash in C++)
pub fn string_hash(input: &str) -> c_ulong {
    let mut hasher = DefaultHasher::new();
    input.hash(&mut hasher);
    hasher.finish() as c_ulong
}

/// Generate a unique ID (simple counter-based approach)
pub fn gen_id() -> c_ulong {
    static mut COUNTER: c_ulong = 0;
    unsafe {
        COUNTER += 1;
        COUNTER
    }
}

/// Check if a string represents an immediate value
pub fn is_imm(input: &str) -> bool {
    let trimmed = input.trim();
    
    // Check for hex format (0x prefix)
    if trimmed.starts_with("0x") || trimmed.starts_with("0X") {
        return trimmed.len() > 2 && trimmed[2..].chars().all(|c| c.is_ascii_hexdigit());
    }
    
    // Check for decimal format (optional negative sign followed by digits)
    if trimmed.starts_with('-') {
        return trimmed.len() > 1 && trimmed[1..].chars().all(|c| c.is_ascii_digit());
    }
    
    // Check for positive decimal
    trimmed.chars().all(|c| c.is_ascii_digit()) && !trimmed.is_empty()
}

/// Split string by delimiter
pub fn str_split(input: &str, delimiter: &str) -> Vec<String> {
    if delimiter.is_empty() {
        return vec![input.to_string()];
    }
    
    input.split(delimiter).map(|s| s.to_string()).collect()
}

/// Trim whitespace from string
pub fn str_trim(input: &str) -> String {
    input.trim().to_string()
}

/// Get current time as double (seconds since epoch)
pub fn get_cur_time() -> c_double {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs_f64()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_operation_len_to_str() {
        assert_eq!(transfer_operation_len_to_str(0), b"NONE\0");
        assert_eq!(transfer_operation_len_to_str(1), b"BYTE\0");
        assert_eq!(transfer_operation_len_to_str(4), b"DWORD\0");
        assert_eq!(transfer_operation_len_to_str(8), b"QWORD\0");
    }

    #[test] 
    fn test_string_hash() {
        let hash1 = string_hash("test");
        let hash2 = string_hash("test");
        let hash3 = string_hash("different");
        
        assert_eq!(hash1, hash2); // Same input should produce same hash
        assert_ne!(hash1, hash3); // Different inputs should produce different hashes
    }

    #[test]
    fn test_is_imm() {
        assert!(is_imm("123"));
        assert!(is_imm("-456"));
        assert!(is_imm("0x1a2b"));
        assert!(is_imm("0X1A2B"));
        assert!(!is_imm("abc"));
        assert!(!is_imm("0x"));
        assert!(!is_imm(""));
        assert!(!is_imm("12.34"));
    }

    #[test]
    fn test_str_split() {
        assert_eq!(str_split("a,b,c", ","), vec!["a", "b", "c"]);
        assert_eq!(str_split("hello world", " "), vec!["hello", "world"]);
        assert_eq!(str_split("no-delimiter", ","), vec!["no-delimiter"]);
        assert_eq!(str_split("", ","), vec![""]);
    }

    #[test]
    fn test_str_trim() {
        assert_eq!(str_trim("  hello  "), "hello");
        assert_eq!(str_trim("\t\nworld\t\n"), "world");
        assert_eq!(str_trim("notrim"), "notrim");
    }

    #[test]
    fn test_gen_id() {
        let id1 = gen_id();
        let id2 = gen_id();
        assert_ne!(id1, id2);
        assert!(id2 > id1);
    }

    #[test]
    fn test_get_cur_time() {
        let time = get_cur_time();
        assert!(time > 0.0);
    }
}