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

export function milliseconds_to_dhms(time_ms) {
  const seconds_ms = 1000;
  const minutes_ms = seconds_ms*60;
  const hours_ms = minutes_ms*60;
  const days_ms = hours_ms*24;

  const days = Math.floor(time_ms / days_ms);
  time_ms -= days*days_ms;
  const hours = Math.floor(time_ms / hours_ms);
  time_ms -= hours*hours_ms;
  const minutes = Math.floor(time_ms / minutes_ms);
  time_ms -= minutes*minutes_ms;
  const seconds = Math.floor(time_ms / seconds_ms);
  time_ms -= seconds*seconds_ms;
  const milliseconds = time_ms;
  return { days, hours, minutes, seconds, milliseconds };
}
