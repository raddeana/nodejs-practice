static void SetImmediateCallback (const FunctionCallbackInfo<Value>& args) {
    CHECK(args[0]->IsFunction());
    
    auto env = Environment::GetCurrent(args);
    env->set_immediate_callback_function(args[0].As<Function>());
    
    auto toggle_ref_cb = [] (const FunctionCallbackInfo<Value>& args) {
        Environment::GetCurrent(args)->ToggleImmediateRef(args[0]->IsTrue());
    };

    auto toggle_ref_function = 
        env->NewFunctionTemplate(toggle_ref_cb)->GetFunction(env->context())
        .ToLocalChecked();

    auto result = Array::New(env->isolate(), 2);

    result->Set(env->context(), 0, env->immediate_info()->fields().GetJSArray()).FromJust();
    result->Set(env->context(), 1, toggle_ref_function).FromJust();
    
    args.GetReturnValue().Set(result);
}