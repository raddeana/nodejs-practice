// 利用v8新建一个函数
auto process_template = FunctionTemplate::New(isolate());

// 设置函数名
process_template->SetClassName(FIXED_ONE_BYTE_STRING(isolate(), 'process'));

// 利用函数 new 一个对象
auto process_object = process_template->GetFunction()->NewInstance(context()).ToLocalChecked();

// 设置 env 的一个属性，val 是process_object
set_process_object(process_object);

// 设置 process 对象的属性
SetupProcessObject(this, argc, argv, exec_argc, exec_argv);