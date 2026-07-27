
const API_VERSION = "api/v1"
const HOSTNAME = document.location.host;
// const HOSTNAME = "192.168.1.124";
export const WEBSOCKET_URL = `ws://${HOSTNAME}/${API_VERSION}/websocket`;
const API_URL = `${document.location.protocol}//${HOSTNAME}/${API_VERSION}`;

export async function get_bluetooth_devices() {
  const url = `${API_URL}/bluetooth_devices`;
  const response = await fetch(url);
  if (!response.ok) {
    throw new Error(`Response status: ${response.status}`);
  }
  const devices = await response.json();
  return devices;
}

export async function get_heap_stats() {
  const url = `${API_URL}/heap_stats`;
  const response = await fetch(url);
  if (!response.ok) {
    throw new Error(`Response status: ${response.status}`);
  }
  const devices = await response.json();
  return devices;
}
