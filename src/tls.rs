use axum_server::tls_rustls::RustlsConfig;
use rustls::{Certificate, PrivateKey, RootCertStore, ServerConfig};
use rustls_pemfile::{certs, pkcs8_private_keys};
use std::fs::File;
use std::io::BufReader;
use std::sync::Arc;

pub async fn create_tls_config() -> Result<RustlsConfig, Box<dyn std::error::Error + Send + Sync>> {
    let cert_file = std::env::var("TLS_CERT_FILE").map_err(|_| "TLS_CERT_FILE not set")?;
    let key_file = std::env::var("TLS_KEY_FILE").map_err(|_| "TLS_KEY_FILE not set")?;
    let client_ca_file = std::env::var("CLIENT_CA_CERT").ok();

    // Load server certificate and private key
    let cert_file = File::open(&cert_file)?;
    let mut cert_reader = BufReader::new(cert_file);
    let cert_chain: Vec<Certificate> = certs(&mut cert_reader)?
        .into_iter()
        .map(Certificate)
        .collect();

    let key_file = File::open(&key_file)?;
    let mut key_reader = BufReader::new(key_file);
    let mut keys = pkcs8_private_keys(&mut key_reader)?;
    let private_key = PrivateKey(keys.remove(0));

    // Create server config
    let config = if let Some(client_ca_path) = client_ca_file {
        // Enable mutual TLS if client CA is provided
        let client_ca_file = File::open(&client_ca_path)?;
        let mut client_ca_reader = BufReader::new(client_ca_file);
        let client_ca_certs: Vec<Certificate> = certs(&mut client_ca_reader)?
            .into_iter()
            .map(Certificate)
            .collect();

        let mut root_cert_store = RootCertStore::empty();
        for cert in client_ca_certs {
            root_cert_store.add(&cert)?;
        }

        let client_cert_verifier =
            rustls::server::AllowAnyAuthenticatedClient::new(root_cert_store);

        ServerConfig::builder()
            .with_safe_defaults()
            .with_client_cert_verifier(Arc::new(client_cert_verifier))
            .with_single_cert(cert_chain, private_key)?
    } else {
        ServerConfig::builder()
            .with_safe_defaults()
            .with_no_client_auth()
            .with_single_cert(cert_chain, private_key)?
    };

    Ok(RustlsConfig::from_config(Arc::new(config)))
}
