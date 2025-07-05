use axum::{http::StatusCode, response::Json, routing::get, Router};
use serde_json::json;

pub fn create_health_router() -> Router {
    Router::new()
        .route("/healthz/ready", get(readiness_check))
        .route("/healthz/live", get(liveness_check))
}

async fn readiness_check() -> Result<Json<serde_json::Value>, StatusCode> {
    Ok(Json(json!({
        "status": "ready",
        "timestamp": chrono::Utc::now().to_rfc3339()
    })))
}

async fn liveness_check() -> Result<Json<serde_json::Value>, StatusCode> {
    Ok(Json(json!({
        "status": "alive",
        "timestamp": chrono::Utc::now().to_rfc3339()
    })))
}
