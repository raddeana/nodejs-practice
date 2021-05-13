Local<String> script_name = FIXED_ONE_BYTE_STRING(env->isolate(), "bootstrap_node.js");

// 执行bootstrap_node.js
Local<Value> f_value = ExecuteString(env, MainSource(env), script_name);
Local<Function> f = Local<Function>::Cast(f_value);

// 全局变量，我们访问全局变量的时候都是global的属性
Local<Object> global = env->context()->Global()

// js层的全局变量，类似浏览器的window
global->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "global"), global);
Local<Value> arg = env->process_object();

// 执行bootstrap_node.js 
auto ret = f->Call(env->context(), Null(env->isolate()), 1, &arg);