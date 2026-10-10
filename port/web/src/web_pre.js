// WEB_PRE.JS: runs first on the page and in each of the game's threads
// (tools/web_build.py --pre-js). An error on one of the threads reaches the
// page with its stack (the functions' names: --profiling-funcs), for the log.
if (typeof WorkerGlobalScope != 'undefined' && self instanceof WorkerGlobalScope) {
  self.addEventListener('error', (event) => {
    var error = event.error;
    var text = (error && error.stack) ? String(error.stack) : String(event.message);
    console.error('halo worker: ' + text);
  });
  self.addEventListener('unhandledrejection', (event) => {
    var reason = event.reason;
    console.error('halo worker: ' + ((reason && reason.stack) ? reason.stack : String(reason)));
  });
}
