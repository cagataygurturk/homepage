use crate::get_node_name;
use crate::metrics::{read_cpu_temperature, read_fan_speed};
use axum::{
    extract::ws::{WebSocket, WebSocketUpgrade},
    response::Response,
};
use serde_json::json;
use std::time::Duration;
use sysinfo::System;
use tokio::time::sleep;

pub async fn websocket_handler(ws: WebSocketUpgrade) -> Response {
    ws.on_upgrade(handle_websocket)
}

async fn handle_websocket(mut socket: WebSocket) {
    let mut sys = System::new_all();
    let node_name = get_node_name();

    loop {
        sys.refresh_all();

        let cpu_usage = sys.global_cpu_info().cpu_usage();
        let cpu_temp = read_cpu_temperature().await;
        let fan_speed = read_fan_speed().await;

        let data = json!({
            "cpuUsage": cpu_usage.round() as u32,
            "cpuTemperature": cpu_temp.round() as u32,
            "fanSpeed": fan_speed,
            "nodeName": node_name
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
