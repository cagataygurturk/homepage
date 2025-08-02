use crate::get_node_name;
use crate::metrics::{read_cpu_temperature, read_fan_speed};
use axum::{
    extract::ws::{WebSocket, WebSocketUpgrade},
    response::Response,
};
use chrono::{DateTime, Utc};
use serde_json::json;
use std::collections::VecDeque;
use std::sync::{Arc, Mutex};
use std::time::Duration;
use sysinfo::System;
use tokio::time::sleep;

#[derive(Clone)]
struct MetricsDataPoint {
    timestamp: DateTime<Utc>,
    cpu_usage: f32,
    temperature: f32,
    fan_speed: u32,
}

type MetricsHistory = Arc<Mutex<VecDeque<MetricsDataPoint>>>;

lazy_static::lazy_static! {
    static ref METRICS_HISTORY: MetricsHistory = Arc::new(Mutex::new(VecDeque::new()));
}

pub async fn websocket_handler(ws: WebSocketUpgrade) -> Response {
    ws.on_upgrade(handle_websocket)
}

fn store_metrics_data(cpu_usage: f32, temperature: f32, fan_speed: u32) {
    let now = Utc::now();
    let fifteen_minutes_ago = now - chrono::Duration::minutes(15);

    if let Ok(mut history) = METRICS_HISTORY.lock() {
        history.push_back(MetricsDataPoint {
            timestamp: now,
            cpu_usage,
            temperature,
            fan_speed,
        });

        while let Some(front) = history.front() {
            if front.timestamp < fifteen_minutes_ago {
                history.pop_front();
            } else {
                break;
            }
        }
    }
}

fn get_metrics_history() -> Vec<MetricsDataPoint> {
    if let Ok(history) = METRICS_HISTORY.lock() {
        history.iter().cloned().collect()
    } else {
        Vec::new()
    }
}

async fn handle_websocket(mut socket: WebSocket) {
    let mut sys = System::new_all();
    let node_name = get_node_name();

    loop {
        sys.refresh_all();

        let cpu_usage = sys.global_cpu_info().cpu_usage();
        let cpu_temp = read_cpu_temperature().await;
        let fan_speed = read_fan_speed().await;

        store_metrics_data(cpu_usage, cpu_temp, fan_speed);
        let history = get_metrics_history();

        let data = json!({
            "cpuUsage": cpu_usage.round() as u32,
            "cpuTemperature": cpu_temp.round() as u32,
            "fanSpeed": fan_speed,
            "nodeName": node_name,
            "metricsHistory": history.iter().map(|point| json!({
                "timestamp": point.timestamp.timestamp_millis(),
                "cpuUsage": point.cpu_usage,
                "temperature": point.temperature,
                "fanSpeed": point.fan_speed
            })).collect::<Vec<_>>()
        });

        if socket
            .send(axum::extract::ws::Message::Text(data.to_string()))
            .await
            .is_err()
        {
            break;
        }

        sleep(Duration::from_secs(2)).await;
    }
}
