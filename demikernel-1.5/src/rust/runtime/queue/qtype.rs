// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

use ::std::convert::TryFrom;

//==============================================================================
// Structures
//==============================================================================

/// IO Queue Type
#[repr(u32)]
#[derive(Debug, Eq, PartialEq, Hash, Copy, Clone)]
pub enum QType {
    UdpSocket = 0x0001,
    TcpSocket = 0x0002,
    MemoryQueue = 0x003,
    TestQueue = 0x004,
}

//==============================================================================
// Trait Implementations
//==============================================================================

/// From Trait Implementation for IO Queue Types
impl From<QType> for u32 {
    fn from(value: QType) -> Self {
        match value {
            QType::UdpSocket => 0x0001,
            QType::TcpSocket => 0x0002,
            QType::MemoryQueue => 0x0003,
            QType::TestQueue => 0x0004,
        }
    }
}

/// From Trait Implementation for IO Queue Types
impl TryFrom<u32> for QType {
    type Error = &'static str;

    fn try_from(value: u32) -> Result<Self, Self::Error> {
        match value {
            0x0001 => Ok(QType::UdpSocket),
            0x0002 => Ok(QType::TcpSocket),
            0x0003 => Ok(QType::MemoryQueue),
            0x0004 => Ok(QType::TestQueue),
            _ => Err("invalid qtype"),
        }
    }
}
