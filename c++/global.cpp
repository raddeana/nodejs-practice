void say () {}

Handle<FunctionTemplate> a_template = FunctionTemplate::New(callbackWhenNewObject);
a_template ->SetClassName(String::New("a_template"));

Handle<ObjectTemplate> a_template _proto = a_template->PrototypeTemplate();
a_template _proto->Set(String::New("say"), FunctionTemplate::New(say));

// 挂载到全局变量，我们在js里就可以直接访问a_template 
global->Set(String::New("a_template "), a_template );