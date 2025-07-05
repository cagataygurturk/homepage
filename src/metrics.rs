use std::path::Path;

pub async fn read_cpu_temperature() -> f32 {
    // Try Raspberry Pi thermal zone path
    let rpi_thermal_path = "/sys/class/thermal/thermal_zone0/temp";
    if Path::new(rpi_thermal_path).exists() {
        if let Ok(content) = tokio::fs::read_to_string(rpi_thermal_path).await {
            if let Ok(temp_millidegrees) = content.trim().parse::<u32>() {
                // Convert from millidegrees to degrees Celsius
                return temp_millidegrees as f32 / 1000.0;
            }
        }
    }

    // Try common hwmon temperature paths
    for i in 0..10 {
        let path = format!("/sys/class/hwmon/hwmon{}/temp1_input", i);
        if Path::new(&path).exists() {
            if let Ok(content) = tokio::fs::read_to_string(&path).await {
                if let Ok(temp_millidegrees) = content.trim().parse::<u32>() {
                    return temp_millidegrees as f32 / 1000.0;
                }
            }
        }
    }

    0.0
}

pub async fn read_fan_speed() -> u32 {
    // Try Raspberry Pi fan speed path first
    let rpi_path = "/sys/devices/platform/cooling_fan/hwmon/hwmon3/fan1_input";
    if Path::new(rpi_path).exists() {
        if let Ok(content) = tokio::fs::read_to_string(rpi_path).await {
            if let Ok(speed) = content.trim().parse::<u32>() {
                return speed;
            }
        }
    }

    // Try common hwmon paths
    for i in 0..10 {
        let path = format!("/sys/class/hwmon/hwmon{}/fan1_input", i);
        if Path::new(&path).exists() {
            if let Ok(content) = tokio::fs::read_to_string(&path).await {
                if let Ok(speed) = content.trim().parse::<u32>() {
                    return speed;
                }
            }
        }
    }

    0
}
