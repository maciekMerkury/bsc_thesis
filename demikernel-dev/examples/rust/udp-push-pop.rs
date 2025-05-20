// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

//======================================================================================================================
// Imports
//======================================================================================================================

use ::anyhow::Result;
use ::demikernel::{demi_sgarray_t, runtime::types::demi_opcode_t, LibOS, LibOSName, QDesc, QToken};
use ::std::{env, net::SocketAddr, slice, str::FromStr, time::Duration};
use log::{error, warn};

#[cfg(target_os = "windows")]
pub const AF_INET: i32 = windows::Win32::Networking::WinSock::AF_INET.0 as i32;

#[cfg(target_os = "windows")]
pub const SOCK_DGRAM: i32 = windows::Win32::Networking::WinSock::SOCK_DGRAM.0 as i32;

#[cfg(target_os = "linux")]
pub const AF_INET: i32 = libc::AF_INET;

#[cfg(target_os = "linux")]
pub const SOCK_DGRAM: i32 = libc::SOCK_DGRAM;

//======================================================================================================================
// Constants
//======================================================================================================================

const BUFSIZE_BYTES: usize = 64;
const FILL_CHAR: u8 = 0x65;
const NUM_SENDS: usize = 1024;
const TIMEOUT_SECONDS: Duration = Duration::from_secs(256);

fn mksga(libos: &mut LibOS, size: usize, value: u8) -> Result<demi_sgarray_t> {
    let sga: demi_sgarray_t = match libos.sgaalloc(size) {
        Ok(sga) => sga,
        Err(e) => panic!("failed to allocate scatter-gather array: {:?}", e),
    };

    // Ensure that allocated array has the requested size.
    if sga.sga_segs[0].sgaseg_len as usize != size {
        freesga(libos, sga);
        let seglen: usize = sga.sga_segs[0].sgaseg_len as usize;
        anyhow::bail!(
            "failed to allocate scatter-gather array: expected size={:?} allocated size={:?}",
            size,
            seglen
        );
    }

    // Fill in the array.
    let ptr: *mut u8 = sga.sga_segs[0].sgaseg_buf as *mut u8;
    let len: usize = sga.sga_segs[0].sgaseg_len as usize;
    let slice: &mut [u8] = unsafe { slice::from_raw_parts_mut(ptr, len) };
    slice.fill(value);

    Ok(sga)
}

fn freesga(libos: &mut LibOS, sga: demi_sgarray_t) {
    if let Err(e) = libos.sgafree(sga) {
        error!("sgafree() failed (error={:?})", e);
        warn!("leaking sga");
    }
}

fn close(libos: &mut LibOS, sockqd: QDesc) {
    if let Err(e) = libos.close(sockqd) {
        error!("close() failed (error={:?})", e);
        warn!("leaking sockqd={:?}", sockqd);
    }
}

pub struct UdpServer {
    libos: LibOS,
    sockqd: QDesc,
    sga: Option<demi_sgarray_t>,
}

impl UdpServer {
    pub fn new(mut libos: LibOS) -> Result<Self> {
        let sockqd: QDesc = match libos.socket(AF_INET, SOCK_DGRAM, 0) {
            Ok(sockqd) => sockqd,
            Err(e) => anyhow::bail!("failed to create socket: {:?}", e),
        };
        return Ok(Self {
            libos,
            sockqd,
            sga: None,
        });
    }

    fn run(&mut self, local: SocketAddr, fill_char: u8, num_sends: usize) -> Result<()> {
        // Wait for half the packets that we sent. We chose this number randomly to account for lost packets.
        let num_receives: usize = num_sends / 2;

        match self.libos.bind(self.sockqd, local) {
            Ok(()) => (),
            Err(e) => anyhow::bail!("bind failed: {:?}", e),
        };

        for i in 0..num_receives {
            let qt: QToken = match self.libos.pop(self.sockqd, None) {
                Ok(qt) => qt,
                Err(e) => anyhow::bail!("pop failed: {:?}", e),
            };
            self.sga = match self.libos.wait(qt, Some(TIMEOUT_SECONDS)) {
                Ok(qr) if qr.qr_opcode == demi_opcode_t::DEMI_OPC_POP => unsafe { Some(qr.qr_value.sga) },
                Ok(_) => anyhow::bail!("unexpected result"),
                Err(e) => anyhow::bail!("operation failed: {:?}", e),
            };

            if let Some(sga) = self.sga {
                // Sanity check received data.
                let ptr: *mut u8 = sga.sga_segs[0].sgaseg_buf as *mut u8;
                let len: usize = sga.sga_segs[0].sgaseg_len as usize;
                let slice: &mut [u8] = unsafe { slice::from_raw_parts_mut(ptr, len) };
                for x in slice {
                    demikernel::ensure_eq!(*x, fill_char);
                }
                // Free up the scatter-gather array.
                match self.libos.sgafree(sga) {
                    Ok(_) => self.sga = None,
                    Err(e) => anyhow::bail!("failed to release scatter-gather array: {:?}", e),
                }
            } else {
                anyhow::bail!("expected a valid sga");
            }

            println!("pop ({:?})", i);
        }
        Ok(())
    }
}

impl Drop for UdpServer {
    fn drop(&mut self) {
        close(&mut self.libos, self.sockqd);
        if let Some(sga) = self.sga {
            freesga(&mut self.libos, sga);
        }
    }
}

pub struct UdpClient {
    libos: LibOS,
    sockqd: QDesc,
    sga: Option<demi_sgarray_t>,
}

impl UdpClient {
    pub fn new(mut libos: LibOS) -> Result<Self> {
        let sockqd: QDesc = match libos.socket(AF_INET, SOCK_DGRAM, 0) {
            Ok(sockqd) => sockqd,
            Err(e) => anyhow::bail!("failed to create socket: {:?}", e),
        };

        return Ok(Self {
            libos,
            sockqd,
            sga: None,
        });
    }

    fn run(
        &mut self,
        local_socket_addr: SocketAddr,
        remote_socket_addr: SocketAddr,
        fill_char: u8,
        bufsize_bytes: usize,
        num_sends: usize,
    ) -> Result<()> {
        match self.libos.bind(self.sockqd, local_socket_addr) {
            Ok(()) => (),
            Err(e) => anyhow::bail!("bind failed: {:?}", e),
        };

        for i in 0..num_sends {
            self.sga = match mksga(&mut self.libos, bufsize_bytes, fill_char) {
                Ok(sga) => Some(sga),
                Err(e) => anyhow::bail!("failed to allocate scatter-gather array: {:?}", e),
            };

            if let Some(sga) = self.sga {
                let qt: QToken = match self.libos.pushto(self.sockqd, &sga, remote_socket_addr) {
                    Ok(qt) => qt,
                    Err(e) => anyhow::bail!("push failed: {:?}", e),
                };

                match self.libos.wait(qt, Some(TIMEOUT_SECONDS)) {
                    Ok(qr) if qr.qr_opcode == demi_opcode_t::DEMI_OPC_PUSH => (),
                    Err(e) => anyhow::bail!("operation failed: {:?}", e),
                    _ => anyhow::bail!("unexpected result"),
                };

                match self.libos.sgafree(sga) {
                    Ok(_) => self.sga = None,
                    Err(e) => anyhow::bail!("failed to release scatter-gather array: {:?}", e),
                }
            }

            println!("push ({:?})", i);
        }
        Ok(())
    }
}

impl Drop for UdpClient {
    fn drop(&mut self) {
        close(&mut self.libos, self.sockqd);
        if let Some(sga) = self.sga {
            freesga(&mut self.libos, sga);
        }
    }
}

fn usage(program_name: &String) {
    println!("Usage: {} MODE local [remote]\n", program_name);
    println!("Modes:\n");
    println!("  --client    Run program in client mode.");
    println!("  --server    Run program in server mode.");
}

pub fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();

    if args.len() >= 3 {
        let libos_name: LibOSName = match LibOSName::from_env() {
            Ok(libos_name) => libos_name.into(),
            Err(e) => anyhow::bail!("{:?}", e),
        };
        let libos: LibOS = match LibOS::new(libos_name, None) {
            Ok(libos) => libos,
            Err(e) => anyhow::bail!("failed to initialize libos: {:?}", e),
        };
        let local_socket_addr: SocketAddr = SocketAddr::from_str(&args[2])?;

        if args[1] == "--server" {
            let mut server: UdpServer = UdpServer::new(libos)?;
            return server.run(local_socket_addr, FILL_CHAR, NUM_SENDS);
        } else if args[1] == "--client" && args.len() == 4 {
            let remote_socket_addr: SocketAddr = SocketAddr::from_str(&args[3])?;
            let mut client: UdpClient = UdpClient::new(libos)?;
            return client.run(
                local_socket_addr,
                remote_socket_addr,
                FILL_CHAR,
                BUFSIZE_BYTES,
                NUM_SENDS,
            );
        }
    }

    usage(&args[0]);

    Ok(())
}
