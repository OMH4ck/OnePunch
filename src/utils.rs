//! Utility functions

use std::time::Instant;

/// Split a string by a delimiter
pub fn str_split(s: &str, delimiter: &str) -> Vec<String> {
    if s.is_empty() {
        return vec![s.to_string()];
    }
    s.split(delimiter).map(|x| x.to_string()).collect()
}

/// Trim whitespace from a string
pub fn str_trim(s: &str) -> &str {
    s.trim()
}

/// Get current time in seconds (for profiling)
pub fn get_cur_time() -> f64 {
    static START: std::sync::OnceLock<Instant> = std::sync::OnceLock::new();
    let start = START.get_or_init(Instant::now);
    start.elapsed().as_secs_f64()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_str_split_basic() {
        let result = str_split("a,b,c", ",");
        assert_eq!(result, vec!["a", "b", "c"]);
    }

    #[test]
    fn test_str_split_no_delimiter() {
        let result = str_split("abc", ",");
        assert_eq!(result, vec!["abc"]);
    }

    #[test]
    fn test_str_split_empty() {
        let result = str_split("", ",");
        assert_eq!(result, vec![""]);
    }

    #[test]
    fn test_str_split_multiple_chars() {
        let result = str_split("a::b::c", "::");
        assert_eq!(result, vec!["a", "b", "c"]);
    }

    #[test]
    fn test_str_trim_basic() {
        assert_eq!(str_trim("  hello  "), "hello");
    }

    #[test]
    fn test_str_trim_empty() {
        assert_eq!(str_trim(""), "");
    }

    #[test]
    fn test_str_trim_no_whitespace() {
        assert_eq!(str_trim("hello"), "hello");
    }

    #[test]
    fn test_get_cur_time() {
        let t1 = get_cur_time();
        std::thread::sleep(std::time::Duration::from_millis(10));
        let t2 = get_cur_time();
        assert!(t2 > t1);
    }
}
