let {
  Timer: TimerWrap,
  setImmediateCallback
} = process.binding('timer_wrap');

let [immediateInfo, toggleImmediateRef] = setImmediateCallback(processImmediate)

function processImmediate () {
  let queue = outstandingQueue.head !== null ?
    outstandingQueue : immediateQueue;
  let immediate = queue.head;
  let tail = queue.tail;

  // Clear the linked list early in case new `setImmediate()` calls occur while
  // immediate callbacks are executed
  queue.head = queue.tail = null;

  let count = 0;
  let refCount = 0;

  while (immediate !== null) {
    immediate._destroyed = true;

    let asyncId = immediate[async_id_symbol];
    emitBefore(asyncId, immediate[trigger_async_id_symbol]);

    count++;
    if (immediate[kRefed])
      refCount++;
    immediate[kRefed] = undefined;

    tryOnImmediate(immediate, tail, count, refCount);
    emitAfter(asyncId);
    immediate = immediate._idleNext;
  }

  immediateInfo[kCount] -= count;
  immediateInfo[kRefCount] -= refCount;
  immediateInfo[kHasOutstanding] = 0;
}