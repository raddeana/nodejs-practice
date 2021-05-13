uv__run_check(loop);

// 在每一轮循环中执行该函数，执行时机见uv_run
void uv__run_##name(uv_loop_t* loop) {
  uv_##name##_t* h;
  QUEUE queue;
  QUEUE* q;

  // 把该类型对应的队列中所有节点摘下来挂载到queue变量
  QUEUE_MOVE(&loop->name##_handles, &queue);

  // 遍历队列，执行每个节点里面的函数
  while (!QUEUE_EMPTY(&queue)) {

    // 取下当前待处理的节点
    q = QUEUE_HEAD(&queue);

    // 取得该节点对应的整个结构体的基地址
    h = QUEUE_DATA(q, uv_##name##_t, queue);

    // 把该节点移出当前队列
    QUEUE_REMOVE(q);

   // 重新插入原来的队列
    QUEUE_INSERT_TAIL(&loop->name##_handles, q);

   // 执行回调函数
    h->name##_cb(h);
  }
}