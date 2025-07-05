use axum::{response::Html, routing::get, Router};
use minify_html::{minify, Cfg};
use std::sync::Arc;
use sysinfo::System;
use tera::{Context, Tera};
use tokio::signal;

mod health;
mod metrics;
mod tls;
mod websocket;

use health::create_health_router;
use metrics::{read_cpu_temperature, read_fan_speed};
use tls::create_tls_config;
use websocket::websocket_handler;

#[tokio::main]
async fn main() {
    // Initialize Tera template engine with embedded template
    let mut tera = Tera::default();
    tera.add_raw_template("index.html", include_str!("../templates/index.html"))
        .expect("Failed to add embedded template");
    tera.autoescape_on(vec![]);
    let tera = Arc::new(tera);

    let app = Router::new()
        .route(
            "/",
            get({
                let tera = tera.clone();
                move || index_handler(tera.clone())
            }),
        )
        .route("/ws", get(websocket_handler));

    // Start health check server on port 8081 (always HTTP, no TLS)
    let health_app = create_health_router();
    let health_server = tokio::spawn(async move {
        let listener = tokio::net::TcpListener::bind("[::]:8081").await.unwrap();
        println!("Health check server running on http://[::]:8081");
        axum::serve(listener, health_app).await.unwrap();
    });

    // Check if HTTPS-only mode is enabled
    let https_only = std::env::var("HTTPS_ONLY")
        .unwrap_or_else(|_| "false".to_string())
        .to_lowercase()
        == "true";

    if https_only {
        // HTTPS-only mode
        if let Ok(tls_config) = create_tls_config().await {
            println!("HTTPS server running on https://[::]:6443");
            let server = axum_server::bind_rustls("[::]:6443".parse().unwrap(), tls_config)
                .serve(app.into_make_service());

            tokio::select! {
                _ = server => {},
                _ = health_server => {},
                _ = shutdown_signal() => {
                    println!("Received shutdown signal, stopping server...");
                }
            }
        } else {
            panic!("HTTPS_ONLY=true but TLS certificates not found. Please provide TLS_CERT_FILE and TLS_KEY_FILE.");
        }
    } else {
        // HTTP-only mode (default)
        let listener = tokio::net::TcpListener::bind("[::]:8080").await.unwrap();
        println!("HTTP server running on http://[::]:8080 (dual-stack IPv4/IPv6)");
        let server = axum::serve(listener, app);

        tokio::select! {
            _ = server => {},
            _ = health_server => {},
            _ = shutdown_signal() => {
                println!("Received shutdown signal, stopping server...");
            }
        }
    }
}

async fn shutdown_signal() {
    let ctrl_c = async {
        signal::ctrl_c()
            .await
            .expect("failed to install Ctrl+C handler");
    };

    #[cfg(unix)]
    let terminate = async {
        signal::unix::signal(signal::unix::SignalKind::terminate())
            .expect("failed to install SIGTERM handler")
            .recv()
            .await;
    };

    #[cfg(not(unix))]
    let terminate = std::future::pending::<()>();

    tokio::select! {
        _ = ctrl_c => {},
        _ = terminate => {},
    }
}

fn get_node_name() -> String {
    // First try NODE_NAME environment variable (set by Kubernetes)
    if let Ok(node_name) = std::env::var("NODE_NAME") {
        return node_name;
    }

    // Fallback to hostname
    std::fs::read_to_string("/etc/hostname")
        .unwrap_or_else(|_| "unknown".to_string())
        .trim()
        .to_string()
}

fn compress_html(html: &str) -> String {
    let cfg = Cfg::spec_compliant();
    let minified = minify(html.as_bytes(), &cfg);
    String::from_utf8(minified).unwrap_or_else(|_| html.to_string())
}

async fn index_handler(tera: Arc<Tera>) -> Html<String> {
    // Read initial metrics
    let mut sys = System::new_all();
    sys.refresh_all();

    let cpu_usage = sys.global_cpu_info().cpu_usage().round() as u32;
    let cpu_temp = read_cpu_temperature().await.round() as u32;
    let fan_speed = read_fan_speed().await;
    let node_name = get_node_name();

    // Create template context with metrics data
    let mut context = Context::new();
    context.insert("cpu_usage", &cpu_usage);
    context.insert("cpu_temperature", &cpu_temp);
    context.insert("fan_speed", &fan_speed);
    context.insert("node_name", &node_name);

    // Render template
    let html_content = tera
        .render("index.html", &context)
        .unwrap_or_else(|e| format!("Template error: {}", e));

    // Compress HTML before serving
    let compressed_html = compress_html(&html_content);

    Html(compressed_html)
}
