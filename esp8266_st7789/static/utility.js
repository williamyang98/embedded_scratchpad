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
