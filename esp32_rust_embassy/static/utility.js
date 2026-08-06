export function format_bluetooth_address(addr) {
  return addr
    .map(v => v.toString())
    .map(s => s.padStart(3, "0"))
    .join(".");
}

export function format_object_to_string(obj) {
  return Object.entries(obj)
    .map(([k,v]) => `${k}=${v}`)
    .join(",");
}

// doesn't forward the return value since it skips and delay calls
// always runs the function with the arguments from the most recent invocation
export function debounce_timeout(func, timeout_ms) {
  if (timeout_ms === undefined) {
    throw Error("timeout_ms argument is missing");
  }

  let running_timeout_id = null;
  let pending_args = null;

  function wrapper(...args) {
    if (running_timeout_id !== null) {
      pending_args = args;
      return;
    }
    func(...args);
    running_timeout_id = setTimeout(() => {
      running_timeout_id = null;
      // attempt to run pending calls
      if (pending_args !== null) {
        let args = pending_args;
        pending_args = null;
        wrapper(args);
      }
    }, timeout_ms);
  }

  return wrapper;
}

export function random_u32() {
  const U32_MAX = 0x100000000;
  return Math.floor(Math.random() * U32_MAX);
}

export function seconds_to_dhms(time_s) {
  const minutes_s = 60;
  const hours_s = minutes_s*60;
  const days_s = hours_s*24;

  const days = Math.floor(time_s / days_s);
  time_s -= days*days_s;
  const hours = Math.floor(time_s / hours_s);
  time_s -= hours*hours_s;
  const minutes = Math.floor(time_s / minutes_s);
  time_s -= minutes*minutes_s;
  const seconds = time_s;
  return { days, hours, minutes, seconds };
}
