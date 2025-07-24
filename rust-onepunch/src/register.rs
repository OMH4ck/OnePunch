use crate::{RustMemory, RustRegister, RustValue};

impl RustRegister {
    pub fn contain_range(&self, range: (i64, i64)) -> bool {
        if let Some(ref memory) = self.memory {
            memory.contain_range(range)
        } else {
            false
        }
    }

    pub fn remove_range(&mut self, range: (i64, i64)) -> bool {
        if let Some(ref mut memory) = self.memory {
            memory.remove_range(range)
        } else {
            false
        }
    }

    pub fn set_content(&mut self, offset: i64, val: RustValue) {
        if let Some(ref mut memory) = self.memory {
            memory.set_content(offset, val);
        }
    }

    pub fn get_memory(&self) -> Option<&RustMemory> {
        self.memory.as_ref().map(|m| m.as_ref())
    }

    pub fn get_memory_mut(&mut self) -> Option<&mut RustMemory> {
        self.memory.as_mut().map(|m| m.as_mut())
    }
}