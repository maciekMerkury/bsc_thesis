// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#![cfg_attr(feature = "strict", deny(warnings))]
#![deny(clippy::all)]
#![feature(drain_filter)]
#![feature(hash_drain_filter)]

//======================================================================================================================
// Imports
//======================================================================================================================

mod args;
mod client;
mod server;

use crate::{
    args::ProgramArguments,
    client::TcpClient,
    server::TcpServer,
};
use anyhow::Result;
use demikernel::{
    LibOS,
    LibOSName,
};

//======================================================================================================================
// main
//======================================================================================================================

fn main() -> Result<()> {
    let args: ProgramArguments = ProgramArguments::new(
        "tcp-close",
        "Pedro Henrique Penna <ppenna@microsoft.com>",
        "Stress test for close() on tcp sockets.",
    )?;

    let libos: LibOS = {
        let libos_name: LibOSName = LibOSName::from_env()?.into();
        LibOS::new(libos_name)?
    };

    match args.who_closes().expect("missing whocloses the socket").as_str() {
        "client" => match args.peer_type().expect("missing peer_type").as_str() {
            "client" => {
                let mut client: TcpClient = TcpClient::new(libos, args.addr(), args.should_async_close())?;
                let nclients: usize = args.nclients().expect("missing number of clients");
                match args.run_mode().as_str() {
                    "sequential" => client.run_sequential(nclients),
                    "concurrent" => client.run_concurrent(nclients),
                    _ => anyhow::bail!("invalid run mode"),
                }
            },
            "server" => {
                let mut server: TcpServer = TcpServer::new(libos, args.addr(), args.should_async_close())?;
                server.run(args.nclients())
            },
            _ => anyhow::bail!("invalid peer type"),
        },
        "server" => match args.peer_type().expect("missing peer_type").as_str() {
            "client" => {
                let mut client: TcpClient = TcpClient::new(libos, args.addr(), args.should_async_close())?;
                let nclients: usize = args.nclients().expect("missing number of clients");
                match args.run_mode().as_str() {
                    "sequential" => client.run_sequential_expecting_server_to_close_sockets(nclients),
                    "concurrent" => client.run_concurrent_expecting_server_to_close_sockets(nclients),
                    _ => anyhow::bail!("invalid run mode"),
                }
            },
            "server" => {
                let mut server: TcpServer = TcpServer::new(libos, args.addr(), args.should_async_close())?;
                server.run_close_sockets_on_accept(args.nclients())
            },
            _ => anyhow::bail!("invalid peer type"),
        },
        _ => anyhow::bail!("invalid whocloses"),
    }
}
