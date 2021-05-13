int main (int argc, char *argv[]) {

#if defined (__linux__)
  char** envp = environ;
  
  while (*envp++ != nullptr) {}
  Elf_auxv_t* auxv = reinterpret_cast<Elf_auxv_t*>(envp);
  
  for (; auxv->a_type != AT_NULL; auxv++) {
    if (auxv->a_type == AT_SECURE) {
      node::linux_at_secure = auxv->a_un.a_val;
      break;
    }
  }
#endif
  // Disable stdio buffering, it interacts poorly with printf()
  // calls elsewhere in the program (e.g., any logging from V8.)
  // 设置流的缓冲方式
  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);
  return node::Start(argc, argv);
}
#endif

int Start (int argc, char** argv) {
  // 注册进程退出时的回调
  atexit([] () { uv_tty_reset_mode(); });
  // 文件打开数和信号处理
  PlatformInit();
  // 当前时间
  node::performance::performance_node_start = PERFORMANCE_NOW();
  CHECK_GT(argc, 0);
  // Hack around with the argv pointer. Used for process.title = "blah".
  argv = uv_setup_args(argc, argv);
  // This needs to run *before* V8::Initialize().  The const_cast is not
  // optional, in case you're wondering.
  int exec_argc;
  const char** exec_argv;
  // 见该函数
  Init(&argc, const_cast<const char**>(argv), &exec_argc, &exec_argv);

#if HAVE_OPENSSL
  {
    std::string extra_ca_certs;
    if (SafeGetenv("NODE_EXTRA_CA_CERTS", &extra_ca_certs))
      crypto::UseExtraCaCerts(extra_ca_certs);
  }
#ifdef NODE_FIPS_MODE
  OPENSSL_init();
#endif  // NODE_FIPS_MODE
  // V8 on Windows doesn't have a good source of entropy. Seed it from
  // OpenSSL's pool.
  V8::SetEntropySource(crypto::EntropySource);
#endif  // HAVE_OPENSSL

  v8_platform.Initialize(v8_thread_pool_size);
  // Enable tracing when argv has --trace-events-enabled.
  if (trace_enabled) {=
    v8_platform.StartTracingAgent();
  }
  V8::Initialize();
  node::performance::performance_v8_start = PERFORMANCE_NOW();
  v8_initialized = true;
  // 继续初始化
  const int exit_code =
      Start(uv_default_loop(), argc, argv, exec_argc, exec_argv);
  if (trace_enabled) {
    v8_platform.StopTracingAgent();
  }
  v8_initialized = false;
  V8::Dispose();

  v8_platform.Dispose();

  delete[] exec_argv;
  exec_argv = nullptr;

  return exit_code;
}
inline void PlatformInit() {
#ifdef __POSIX__
#if HAVE_INSPECTOR
  sigset_t sigmask;
  // 清空信号
  sigemptyset(&sigmask);
  // 设置信号
  sigaddset(&sigmask, SIGUSR1);
  // 子线程继承了主线程的信号，这里设置sigmask里的信号只有主线程能收到
  const int err = pthread_sigmask(SIG_SETMASK, &sigmask, nullptr);
#endif  // HAVE_INSPECTOR

  // Make sure file descriptors 0-2 are valid before we start logging anything.
  // 保证进程打开标准输入、输出、错误流三个文件描述符
  for (int fd = STDIN_FILENO; fd <= STDERR_FILENO; fd += 1) {
    struct stat ignored;
    // 返回0说明对应的文件描述符已经打开，则继续判断下一个
    if (fstat(fd, &ignored) == 0)
      continue;
    // Anything but EBADF means something is seriously wrong.  We don't
    // have to special-case EINTR, fstat() is not interruptible.
    /*
      EBADF说明fd是无效的文件描述符，说明还没打开，则接下来打开就行，
      如果是其他的错误则说明出错了，EINTR在阻塞的系统调用被信号中断时返回，
      fstate不是阻塞的调用，所以不需要处理
    */
    if (errno != EBADF)
      ABORT();
    if (fd != open("/dev/null", O_RDWR))
      ABORT();
  }

#if HAVE_INSPECTOR
  CHECK_EQ(err, 0);
#endif  // HAVE_INSPECTOR

#ifndef NODE_SHARED_MODE
  // Restore signal dispositions, the parent process may have changed them.
  struct sigaction act;
  memset(&act, 0, sizeof(act));
  // 设置系统所有信号的处理函数，linux上最多32
  for (unsigned nr = 1; nr < kMaxSignal; nr += 1) {
    // 进程不能忽略这两个信号 
    if (nr == SIGKILL || nr == SIGSTOP)
      continue;
    // 忽略或者默认处理
    act.sa_handler = (nr == SIGPIPE) ? SIG_IGN : SIG_DFL;
    CHECK_EQ(0, sigaction(nr, &act, nullptr));
  }
#endif  // !NODE_SHARED_MODE
  // 注册这两个信号的处理函数
  RegisterSignalHandler(SIGINT, SignalExit, true);
  RegisterSignalHandler(SIGTERM, SignalExit, true);

  // Raise the open file descriptor limit.
  // 设置进程能打开的最大文件数，算出最大值和最小值
  struct rlimit lim;
  if (getrlimit(RLIMIT_NOFILE, &lim) == 0 && lim.rlim_cur != lim.rlim_max) {
    // Do a binary search for the limit.
    rlim_t min = lim.rlim_cur;
    rlim_t max = 1 << 20;
    // But if there's a defined upper bound, don't search, just set it.
    if (lim.rlim_max != RLIM_INFINITY) {
      min = lim.rlim_max;
      max = lim.rlim_max;
    }
    do {
      lim.rlim_cur = min + (max - min) / 2;
      if (setrlimit(RLIMIT_NOFILE, &lim)) {
        max = lim.rlim_cur;
      } else {
        min = lim.rlim_cur;
      }
    } while (min + 1 < max);
  }
#endif  // __POSIX__
...
}
void Init(int* argc,
          const char** argv,
          int* exec_argc,
          const char*** exec_argv) {
  // Initialize prog_start_time to get relative uptime.
  // 当前时间
  prog_start_time = static_cast<double>(uv_now(uv_default_loop()));

  // Register built-in modules
  // 注册内置模块
  node::RegisterBuiltinModules();

  // Make inherited handles noninheritable.
  // 设置文件描述符的cloexec标记，进程执行fork和exec后关闭有这个标记的文件
  uv_disable_stdio_inheritance();
// 获取一大堆环境变量
#if defined(NODE_V8_OPTIONS)
  // Should come before the call to V8::SetFlagsFromCommandLine()
  // so the user can disable a flag --foo at run-time by passing
  // --no_foo from the command line.
  V8::SetFlagsFromString(NODE_V8_OPTIONS, sizeof(NODE_V8_OPTIONS) - 1);
#endif

  {
    std::string text;
    config_pending_deprecation =
        SafeGetenv("NODE_PENDING_DEPRECATION", &text) && text[0] == '1';
  }

  // Allow for environment set preserving symlinks.
  {
    std::string text;
    config_preserve_symlinks =
        SafeGetenv("NODE_PRESERVE_SYMLINKS", &text) && text[0] == '1';
  }

  if (config_warning_file.empty())
    SafeGetenv("NODE_REDIRECT_WARNINGS", &config_warning_file);

#if HAVE_OPENSSL
  if (openssl_config.empty())
    SafeGetenv("OPENSSL_CONF", &openssl_config);
#endif

#if !defined(NODE_WITHOUT_NODE_OPTIONS)
  std::string node_options;
  if (SafeGetenv("NODE_OPTIONS", &node_options)) {
    // Smallest tokens are 2-chars (a not space and a space), plus 2 extra
    // pointers, for the prepended executable name, and appended NULL pointer.
    size_t max_len = 2 + (node_options.length() + 1) / 2;
    const char** argv_from_env = new const char*[max_len];
    int argc_from_env = 0;
    // [0] is expected to be the program name, fill it in from the real argv.
    argv_from_env[argc_from_env++] = argv[0];

    char* cstr = strdup(node_options.c_str());
    char* initptr = cstr;
    char* token;
    while ((token = strtok(initptr, " "))) {  // NOLINT(runtime/threadsafe_fn)
      initptr = nullptr;
      argv_from_env[argc_from_env++] = token;
    }
    argv_from_env[argc_from_env] = nullptr;
    int exec_argc_;
    const char** exec_argv_ = nullptr;
    ProcessArgv(&argc_from_env, argv_from_env, &exec_argc_, &exec_argv_, true);
    delete[] exec_argv_;
    delete[] argv_from_env;
    free(cstr);
  }
#endif
  // 命令行参数处理
  ProcessArgv(argc, argv, exec_argc, exec_argv);

#if defined(NODE_HAVE_I18N_SUPPORT)
  // If the parameter isn't given, use the env variable.
  if (icu_data_dir.empty())
    SafeGetenv("NODE_ICU_DATA", &icu_data_dir);
  // Initialize ICU.
  // If icu_data_dir is empty here, it will load the 'minimal' data.
  if (!i18n::InitializeICUDirectory(icu_data_dir)) {
    fprintf(stderr,
            "%s: could not initialize ICU "
            "(check NODE_ICU_DATA or --icu-data-dir parameters)\n",
            argv[0]);
    exit(9);
  }
#endif

  // Needed for access to V8 intrinsics.  Disabled again during bootstrapping,
  // see lib/internal/bootstrap_node.js.
  const char allow_natives_syntax[] = "--allow_natives_syntax";
  V8::SetFlagsFromString(allow_natives_syntax,
                         sizeof(allow_natives_syntax) - 1);
  node_is_initialized = true;
}

inline int Start(uv_loop_t* event_loop,
                 int argc, const char* const* argv,
                 int exec_argc, const char* const* exec_argv) {
  Isolate::CreateParams params;
  ArrayBufferAllocator allocator;
  params.array_buffer_allocator = &allocator;
#ifdef NODE_ENABLE_VTUNE_PROFILING
  params.code_event_handler = vTune::GetVtuneCodeEventHandler();
#endif

  Isolate* const isolate = Isolate::New(params);

  if (isolate == nullptr) {
    return 12;  // Signal internal error.
  }

  isolate->AddMessageListener(OnMessage);
  isolate->SetAbortOnUncaughtExceptionCallback(ShouldAbortOnUncaughtException);
  isolate->SetAutorunMicrotasks(false);
  isolate->SetFatalErrorHandler(OnFatalError);

  {
    Mutex::ScopedLock scoped_lock(node_isolate_mutex);
    CHECK_EQ(node_isolate, nullptr);
    node_isolate = isolate;
  }

  int exit_code;
  {
    Locker locker(isolate);
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    IsolateData isolate_data(
        isolate,
        event_loop,
        v8_platform.Platform(),
        allocator.zero_fill_field());
    if (track_heap_objects) {
      isolate->GetHeapProfiler()->StartTrackingHeapObjects(true);
    }
    // 继续
    exit_code = Start(isolate, &isolate_data, argc, argv, exec_argc, exec_argv);
  }

  {
    Mutex::ScopedLock scoped_lock(node_isolate_mutex);
    CHECK_EQ(node_isolate, isolate);
    node_isolate = nullptr;
  }

  isolate->Dispose();
  return exit_code;
}

inline int Start (Isolate* isolate, IsolateData* isolate_data, int argc, const char* const* argv, int exec_argc, const char* const* exec_argv) {
  // 见v8文档
  HandleScope handle_scope(isolate);
  Local<Context> context = NewContext(isolate);
  Context::Scope context_scope(context);
  Environment env(isolate_data, context);
  // 创建线程私有化数据的键，每个线程通过该键读写数据，都是对应的内容是独立的
  CHECK_EQ(0, uv_key_create(&thread_local_env));
  uv_key_set(&thread_local_env, &env);
  // 见env.cc的Start
  env.Start(argc, argv, exec_argc, exec_argv, v8_is_profiling);

  const char* path = argc > 1 ? argv[1] : nullptr;
  StartInspector(&env, path, debug_options);

  if (debug_options.inspector_enabled() && !v8_platform.InspectorStarted(&env))
    return 12;  // Signal internal error.

  env.set_abort_on_uncaught_exception(abort_on_uncaught_exception);

  if (no_force_async_hooks_checks) {
    env.async_hooks()->no_force_checks();
  }

  {
    Environment::AsyncCallbackScope callback_scope(&env);
    env.async_hooks()->push_async_ids(1, 0);
    // 执行js脚步初始化
    LoadEnvironment(&env);
    env.async_hooks()->pop_async_id(1);
  }
  env.set_trace_sync_io(trace_sync_io);
  {
    SealHandleScope seal(isolate);
    bool more;
    PERFORMANCE_MARK(&env, LOOP_START);
    // 开启事件循环
    do {
      uv_run(env.event_loop(), UV_RUN_DEFAULT);
      v8_platform.DrainVMTasks(isolate);
      more = uv_loop_alive(env.event_loop());
      if (more)
        continue;
      RunBeforeExit(&env);
      // Emit `beforeExit` if the loop became alive either after emitting
      // event, or after running some callbacks.
      more = uv_loop_alive(env.event_loop());
    } while (more == true);
    PERFORMANCE_MARK(&env, LOOP_EXIT);
  }
  env.set_trace_sync_io(false);
  const int exit_code = EmitExit(&env);
  RunAtExit(&env);
  uv_key_delete(&thread_local_env);
  v8_platform.DrainVMTasks(isolate);
  v8_platform.CancelVMTasks(isolate);
  WaitForInspectorDisconnect(&env);
#if defined(LEAK_SANITIZER)
  __lsan_do_leak_check();
#endif

  return exit_code;
}

void Environment::Start (int argc, const char* const* argv, int exec_argc, const char* const* exec_argv, bool start_profiler_idle_notifier) {
  HandleScope handle_scope(isolate());
  Context::Scope context_scope(context());
  // 初始化uv_check_t结构体，对应check phase
  uv_check_init(event_loop(), immediate_check_handle());
  // 清除UV__HANDLE_REF标记
  uv_unref(reinterpret_cast<uv_handle_t*>(immediate_check_handle()));
  // 类似，对应idle phase
  uv_idle_init(event_loop(), immediate_idle_handle());
  // 把u）check_t结构体加到loop里，并设置actived标记，uv_run的时候执行CheckImmediate回调
  uv_check_start(immediate_check_handle(), CheckImmediate);

  uv_prepare_init(event_loop(), &idle_prepare_handle_);
  uv_check_init(event_loop(), &idle_check_handle_);
  uv_unref(reinterpret_cast<uv_handle_t*>(&idle_prepare_handle_));
  uv_unref(reinterpret_cast<uv_handle_t*>(&idle_check_handle_));

  auto close_and_finish = [](Environment* env, uv_handle_t* handle, void* arg) {
    handle->data = env;

    uv_close(handle, [](uv_handle_t* handle) {
      static_cast<Environment*>(handle->data)->FinishHandleCleanup(handle);
    });
  };

  // 注册进程退出时的回调
  RegisterHandleCleanup(
      reinterpret_cast<uv_handle_t*>(immediate_check_handle()),
      close_and_finish,
      nullptr);

  RegisterHandleCleanup(
      reinterpret_cast<uv_handle_t*>(immediate_idle_handle()),
      close_and_finish,
      nullptr);

  RegisterHandleCleanup(
      reinterpret_cast<uv_handle_t*>(&idle_prepare_handle_),
      close_and_finish,
      nullptr);

  RegisterHandleCleanup(
      reinterpret_cast<uv_handle_t*>(&idle_check_handle_),
      close_and_finish,
      nullptr);

  if (start_profiler_idle_notifier) {
    StartProfilerIdleNotifier();
  }
  // 利用v8新建一个函数
  auto process_template = FunctionTemplate::New(isolate());
  // 设置函数名
  process_template->SetClassName(FIXED_ONE_BYTE_STRING(isolate(), "process"));
  // 利用函数new一个对象
  auto process_object =
      process_template->GetFunction()->NewInstance(context()).ToLocalChecked();
  /*
    V(process_object, v8::Object)

    env.h
    成员属性宏定义
    #define V(PropertyName, TypeName)                                             \
      v8::Persistent<TypeName> PropertyName ## _;
      ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)
    #undef V
  */
  /*
    函数宏定义
    定义
    inline void Environment::set_ ## PropertyName(v8::Local<TypeName> value) {  \
      PropertyName ## _.Reset(isolate(), value);                                \
    }
    template <class T>
    template <class S>
    void PersistentBase<T>::Reset(Isolate* isolate, const Local<S>& other) {
      TYPE_CHECK(T, S);
      Reset();
      if (other.IsEmpty()) return;
      this->val_ = New(isolate, other.val_);
    } 
  */
  // 设置env的一个属性，类似是Object，val是process_object
  set_process_object(process_object);
  // 设置process对象的属性
  SetupProcessObject(this, argc, argv, exec_argc, exec_argv);
  LoadAsyncWrapperInfo(this);
}

void LoadEnvironment(Environment* env) {
  HandleScope handle_scope(env->isolate());
  TryCatch try_catch(env->isolate());
  // Disable verbose mode to stop FatalException() handler from trying
  // to handle the exception. Errors this early in the start-up phase
  // are not safe to ignore.
  try_catch.SetVerbose(false);
  // Execute the lib/internal/bootstrap_node.js file which was included as a
  // static C string in node_natives.h by node_js2c.
  // 'internal_bootstrap_node_native' is the string containing that source code.
  Local<String> script_name = FIXED_ONE_BYTE_STRING(env->isolate(),
                                                    "bootstrap_node.js");
  // 执行bootstrap_node.js导出的函数
  Local<Value> f_value = ExecuteString(env, MainSource(env), script_name);
  if (try_catch.HasCaught())  {
    ReportException(env, try_catch);
    exit(10);
  }
  // The bootstrap_node.js file returns a function 'f'
  CHECK(f_value->IsFunction());

  Local<Function> f = Local<Function>::Cast(f_value);

  // Add a reference to the global object
  Local<Object> global = env->context()->Global();

#if defined HAVE_DTRACE || defined HAVE_ETW
  InitDTrace(env, global);
#endif

#if defined HAVE_LTTNG
  InitLTTNG(env, global);
#endif

#if defined HAVE_PERFCTR
  InitPerfCounters(env, global);
#endif

  try_catch.SetVerbose(true);

  env->SetMethod(env->process_object(), "_rawDebug", RawDebug);

  // Expose the global object as a property on itself
  // (Allows you to set stuff on `global` from anywhere in JavaScript.)
  global->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "global"), global);

  Local<Value> arg = env->process_object();
  // 执行bootstrap_node.js 
  auto ret = f->Call(env->context(), Null(env->isolate()), 1, &arg);
  if (ret.IsEmpty())
    env->async_hooks()->clear_async_id_stack();
}