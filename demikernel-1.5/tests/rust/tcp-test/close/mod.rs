// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

mod async_close;
mod wait;
//======================================================================================================================
// Imports
//======================================================================================================================

use anyhow::Result;
use demikernel::{
    LibOS,
    QDesc,
};
use std::net::SocketAddrV4;

//======================================================================================================================
// Constants
//======================================================================================================================

#[cfg(target_os = "windows")]
pub const AF_INET: i32 = windows::Win32::Networking::WinSock::AF_INET.0 as i32;

#[cfg(target_os = "windows")]
pub const SOCK_STREAM: i32 = windows::Win32::Networking::WinSock::SOCK_STREAM as i32;

#[cfg(target_os = "linux")]
pub const AF_INET: i32 = libc::AF_INET;

#[cfg(target_os = "linux")]
pub const SOCK_STREAM: i32 = libc::SOCK_STREAM;

//======================================================================================================================
// Standalone Functions
//======================================================================================================================

/// Drives integration tests for close() on TCP sockets.
pub fn run(libos: &mut LibOS, addr: &SocketAddrV4) -> Vec<(String, String, Result<(), anyhow::Error>)> {
    let mut result: Vec<(String, String, Result<(), anyhow::Error>)> = Vec::new();

    crate::collect!(result, crate::test!(close_invalid_queue_descriptor(libos)));
    crate::collect!(result, crate::test!(close_socket_twice(libos)));
    crate::collect!(result, crate::test!(close_unbound_socket(libos)));
    crate::collect!(result, crate::test!(close_bound_socket(libos, addr)));
    crate::collect!(result, crate::test!(close_listening_socket(libos, addr)));

    // Run asynchronous close tests.
    crate::collect!(result, async_close::run(libos, addr));
    crate::collect!(result, wait::run(libos, addr));

    result
}

/// Attempts to close an invalid queue descriptor.
fn close_invalid_queue_descriptor(libos: &mut LibOS) -> Result<()> {
    println!("{}", stringify!(close_invalid_queue_descriptor));

    // Fail to close socket.
    match libos.close(QDesc::from(0)) {
        Err(e) if e.errno == libc::EBADF => Ok(()),
        Err(e) => anyhow::bail!("close() failed with {}", e),
        Ok(()) => anyhow::bail!("close() an invalid socket should fail"),
    }
}

/// Attempts to close a TCP socket multiple times.
fn close_socket_twice(libos: &mut LibOS) -> Result<()> {
    println!("{}", stringify!(close_socket_twice));

    // Create an unbound socket.
    let sockqd: QDesc = libos.socket(AF_INET, SOCK_STREAM, 0)?;

    // Succeed to close socket.
    libos.close(sockqd)?;

    // Fail to close socket.
    match libos.close(sockqd) {
        Err(e) if e.errno == libc::EBADF => Ok(()),
        Err(e) => anyhow::bail!("close() failed with {}", e),
        Ok(()) => anyhow::bail!("close() a socket twice should fail"),
    }
}

/// Attempts to close a TCP socket that is not bound.
fn close_unbound_socket(libos: &mut LibOS) -> Result<()> {
    println!("{}", stringify!(close_unbound_socket));

    // Create an unbound socket.
    let sockqd: QDesc = libos.socket(AF_INET, SOCK_STREAM, 0)?;

    // Succeed to close socket.
    libos.close(sockqd)?;

    Ok(())
}

/// Attempts to close a TCP socket that is bound.
fn close_bound_socket(libos: &mut LibOS, local: &SocketAddrV4) -> Result<()> {
    println!("{}", stringify!(close_bound_socket));

    // Create a bound socket.
    let sockqd: QDesc = libos.socket(AF_INET, SOCK_STREAM, 0)?;
    libos.bind(sockqd, *local)?;

    // Succeed to close socket.
    libos.close(sockqd)?;

    Ok(())
}

/// Attempts to close a TCP socket that is listening.
fn close_listening_socket(libos: &mut LibOS, local: &SocketAddrV4) -> Result<()> {
    println!("{}", stringify!(close_listening_socket));

    // Create a listening socket.
    let sockqd: QDesc = libos.socket(AF_INET, SOCK_STREAM, 0)?;
    libos.bind(sockqd, *local)?;
    libos.listen(sockqd, 16)?;

    // Succeed to close socket.
    libos.close(sockqd)?;

    Ok(())
}
