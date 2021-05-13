int uv_##name##_start(uv_##name##_t* handle, uv_##name##_cb cb) {           
  if (uv__is_active(handle)) {
    return 0;
  }

  if (cb == NULL) {
    return UV_EINVAL;
  }
  
  QUEUE_INSERT_HEAD(&handle->loop->name##_handles, &handle->queue);
  handle->name##_cb = cb;
  uv__handle_start(handle);

  return 0;                                                                 
}